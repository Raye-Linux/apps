# One folder per app, one Debian package per app.
#
#   raye_add_app(<folder> [PACKAGE <package>] [REQUIRES <path>...]
#                [NOT_WRITTEN_YET])
#
# Adds <folder>. Everything the folder installs goes to the CMake
# component named <package>; the Debian build installs each component
# into its own package. A folder without PACKAGE (the shared window
# chrome) must not install anything. REQUIRES names files the package
# cannot ship without (its maintainer scripts use them); relative paths
# start at CMAKE_INSTALL_PREFIX.
#
# NOT_WRITTEN_YET says the folder is declared on purpose before it is
# written: it is skipped and the log says so by name. Without the marker a
# missing folder STOPS the configure.
#
# PACKAGE on a marked folder is allowed, and useful: it reserves the name
# and keeps that package's maintainer scripts in debian/ meaningful. What
# the package must NOT have while the folder is unwritten is a stanza in
# debian/control -- dpkg builds a .deb for every stanza whatever this
# ledger says, and that .deb holds nothing but /usr/share/doc while still
# pulling in its Depends. RayeDebianInstall.cmake refuses that pairing,
# and cmake/tests/test_control_ships_files.cmake refuses it without
# needing a build.
#
# Why the marker exists at all. Until 2026-11-06 a declared folder
# that did not exist was skipped with one STATUS line and nothing else.
# Measured that day: 13 of the 15 folders declared here did not exist,
# so ELEVEN Debian packages were built holding nothing but
# /usr/share/doc -- changelog.gz and copyright, 1.8 kB -- while each
# still declared its full Depends. raye-taskmanager had been in that
# state since the repository began, and the plan of record called it
# "to finish" when its folder had never existed. Every one of those
# builds was green.
#
# Declaring an app before writing it is a legitimate way to work, so the
# answer is not a blunt refusal: it is that an unwritten folder must be
# NAMED. The marker is a ledger that can only shrink -- the folder
# existing while still marked stops the configure too, which is what
# keeps the list from quietly growing to cover apps that are long
# written. cmake/tests/test_app_ledger.cmake holds both reds.
#
# Inside the folder, RAYE_APP_PACKAGE and RAYE_APP_DOMAIN (the gettext
# domain, same as the package) are set.
#
#   raye_write_package_list()
#
# Writes ${CMAKE_BINARY_DIR}/raye-packages.list, read by debian/rules:
#   package <name> present|absent
#   require <name> <absolute path>
#
#   raye_add_qtest(<name> SOURCES <file>... [LIBRARIES <target>...])
#
# A QtTest executable registered with ctest, run without a display.
# Nothing is built when BUILD_TESTING is off (DEB_BUILD_OPTIONS=nocheck).

set(RAYE_INTERNAL_COMPONENT raye-internal)

function(raye_add_app folder)
    cmake_parse_arguments(ARG "NOT_WRITTEN_YET" "PACKAGE" "REQUIRES" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "raye_add_app(${folder}): unknown arguments ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(ARG_REQUIRES AND NOT ARG_PACKAGE)
        message(FATAL_ERROR "raye_add_app(${folder}): REQUIRES needs a PACKAGE")
    endif()

    set(present FALSE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/CMakeLists.txt")
        set(present TRUE)
    endif()

    # ---- the ledger, in both directions
    if(present AND ARG_NOT_WRITTEN_YET)
        message(FATAL_ERROR
            "raye_add_app(${folder}): ${folder}/ exists now, so drop NOT_WRITTEN_YET "
            "from its line in CMakeLists.txt. Left there the marker goes on excusing "
            "this folder the day it disappears again, which is the silence the ledger "
            "exists to break.")
    endif()
    if(NOT present AND NOT ARG_NOT_WRITTEN_YET)
        if(ARG_PACKAGE)
            # set(x "a" "b") makes a LIST, and a list renders with a
            # ";" in the middle of the sentence. string(CONCAT) is the
            # one that makes a string.
            string(CONCAT cost
                "nothing would fill ${ARG_PACKAGE}, and if debian/control "
                "declares it, dpkg builds it anyway: a package holding "
                "nothing but /usr/share/doc, with every one of its Depends")
        else()
            set(cost "nothing would be built from it")
        endif()
        message(FATAL_ERROR
            "raye_add_app(${folder}): ${folder}/CMakeLists.txt does not exist, so "
            "${cost}. Write the folder, or add NOT_WRITTEN_YET to its line to say "
            "that it is unwritten on purpose -- skipping it in silence is what this "
            "refuses.")
    endif()

    if(ARG_PACKAGE)
        get_property(packages GLOBAL PROPERTY RAYE_PACKAGES)
        if(NOT ARG_PACKAGE IN_LIST packages)
            set_property(GLOBAL APPEND PROPERTY RAYE_PACKAGES ${ARG_PACKAGE})
        endif()
        if(present)
            set_property(GLOBAL APPEND PROPERTY RAYE_PACKAGES_PRESENT ${ARG_PACKAGE})
            foreach(path IN LISTS ARG_REQUIRES)
                if(NOT IS_ABSOLUTE "${path}")
                    set(path "${CMAKE_INSTALL_PREFIX}/${path}")
                endif()
                set_property(GLOBAL APPEND PROPERTY RAYE_REQUIRES "${ARG_PACKAGE} ${path}")
            endforeach()
        endif()
    endif()

    # Named, and named with its cost: a reader of the build log must be
    # able to count the empty packages without cross-reading
    # debian/control.
    if(NOT present)
        if(ARG_PACKAGE)
            message(STATUS "Raye apps: ${folder}/ is not written yet, so ${ARG_PACKAGE} is not built; its stanza must stay out of debian/control")
        else()
            message(STATUS "Raye apps: ${folder}/ is not written yet, and has no package of its own")
        endif()
        return()
    endif()

    if(ARG_PACKAGE)
        set(RAYE_APP_PACKAGE ${ARG_PACKAGE})
        set(RAYE_APP_DOMAIN ${ARG_PACKAGE})
        set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${ARG_PACKAGE})
    else()
        unset(RAYE_APP_PACKAGE)
        unset(RAYE_APP_DOMAIN)
        set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${RAYE_INTERNAL_COMPONENT})
    endif()
    message(STATUS "Raye apps: ${folder}/ added")
    # variables set above are seen by the folder: add_subdirectory runs
    # in this function's scope
    add_subdirectory(${folder})
endfunction()

function(raye_write_package_list)
    get_property(packages GLOBAL PROPERTY RAYE_PACKAGES)
    get_property(present GLOBAL PROPERTY RAYE_PACKAGES_PRESENT)
    get_property(requires GLOBAL PROPERTY RAYE_REQUIRES)
    set(text "")
    foreach(package IN LISTS packages)
        if(package IN_LIST present)
            string(APPEND text "package ${package} present\n")
        else()
            string(APPEND text "package ${package} absent\n")
        endif()
    endforeach()
    foreach(line IN LISTS requires)
        string(APPEND text "require ${line}\n")
    endforeach()
    file(WRITE ${CMAKE_BINARY_DIR}/raye-packages.list "${text}")
endfunction()

function(raye_add_qtest name)
    if(DEFINED BUILD_TESTING AND NOT BUILD_TESTING)
        return()
    endif()
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "raye_add_qtest(${name}): SOURCES is empty")
    endif()
    set(prefix "${RAYE_APP_PACKAGE}")
    if(NOT prefix)
        set(prefix "raye")
    endif()
    add_executable(${prefix}-test-${name} ${ARG_SOURCES})
    target_link_libraries(${prefix}-test-${name} PRIVATE Qt6::Test ${ARG_LIBRARIES})
    add_test(NAME ${prefix}/${name} COMMAND ${prefix}-test-${name})
    set_tests_properties(${prefix}/${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=C.UTF-8")
endfunction()
