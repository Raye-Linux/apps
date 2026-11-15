# Every package declared in debian/control must ship at least one file.
#   cmake -DSRC=<repo root> -P test_control_ships_files.cmake
#
# Why this exists. Measured 2026-11-06 on bb15e0c: debian/control declared
# THIRTEEN packages while only two app folders existed, so ELEVEN .deb were
# built holding nothing but /usr/share/doc -- changelog.gz and copyright,
# 1792 bytes, Installed-Size 9 -- each still declaring its full Depends.
# raye-terminal pulled in konsole-kpart and delivered no terminal. That is
# the defect this repository pays everywhere else: a command that succeeds
# without doing the work. Every one of those builds was green.
#
# The ledger in CMakeLists.txt (cmake/tests/test_app_ledger.cmake) makes an
# unwritten folder be NAMED. It does not stop its package from being
# DECLARED, and a declared package is a package dpkg builds. Naming the
# folder and shipping the empty .deb are two different things, and only this
# file holds the second one.
#
# The rule: a folder may be declared in CMakeLists.txt before it is written
# -- that is a legitimate way to work, and PACKAGE there reserves its name
# and keeps its maintainer scripts meaningful. Its stanza in debian/control
# is what must wait, because that stanza alone is what builds a .deb.
cmake_minimum_required(VERSION 3.22)

if(NOT SRC)
    message(FATAL_ERROR "test_control_ships_files: SRC is not set")
endif()
get_filename_component(SRC "${SRC}" ABSOLUTE)

set(failures 0)
function(bad msg)
    message(SEND_ERROR "${msg}")
    set(failures "1" PARENT_SCOPE)
endfunction()

set(control "${SRC}/debian/control")
set(lists "${SRC}/CMakeLists.txt")
foreach(f "${control}" "${lists}")
    if(NOT EXISTS "${f}")
        message(FATAL_ERROR "test_control_ships_files: ${f} does not exist")
    endif()
endforeach()

# ---- what debian/control declares
file(STRINGS "${control}" package_lines REGEX "^Package:")
set(packages "")
foreach(line IN LISTS package_lines)
    if(line MATCHES "^Package:[ \t]*([a-z0-9][a-z0-9.+-]*)[ \t]*$")
        list(APPEND packages "${CMAKE_MATCH_1}")
    endif()
endforeach()

# ---- what CMakeLists.txt declares: folder, its package, and nothing else
# The folders are read from the file, never written here: a list of names
# in this file would go stale the day an app is added, and the sweep below
# would then pass by looking at nothing.
# Anchored: an unanchored pattern reads "# raye_add_app(foo PACKAGE ...)"
# as a real declaration, so a line someone commented out would still back
# a stanza here while the configure ignores it -- the test and the build
# would disagree, and the test would be the one saying yes.
file(STRINGS "${lists}" app_lines REGEX "^[ \t]*raye_add_app\\(")
set(folders "")
set(folder_packages "")
foreach(line IN LISTS app_lines)
    if(NOT line MATCHES "^[ \t]*raye_add_app\\([ \t]*([A-Za-z0-9_.+-]+)([^)]*)\\)")
        bad("CMakeLists.txt: cannot read this raye_add_app() line, so the package it declares is not checked by anything: ${line}")
        continue()
    endif()
    set(folder "${CMAKE_MATCH_1}")
    set(rest "${CMAKE_MATCH_2}")
    set(package "")
    if(rest MATCHES "PACKAGE[ \t]+([a-z0-9][a-z0-9.+-]*)")
        set(package "${CMAKE_MATCH_1}")
    endif()
    list(APPEND folders "${folder}")
    list(APPEND folder_packages "${folder}|${package}")
endforeach()

# The three floors below are the point of this file, not decoration.
# An empty sweep is a GREEN sweep: `foreach` over an empty list runs no
# body and asserts nothing, exactly as pytest reports an empty set of
# parameters as a SKIP and unittest reports an empty discovery as a pass.
# A regex that stops matching -- debian/control reformatted, raye_add_app
# renamed -- would leave every list below empty, and without these three
# lines this test would say "all backed" having examined nothing.
list(LENGTH packages package_count)
list(LENGTH app_lines app_line_count)
list(LENGTH package_lines stanza_line_count)
# A Package: line that the regex cannot read is a package NOTHING below
# examines, and the floors above would not notice because the list is not
# empty -- only short. A CR left by an editor, or a trailing comment, is
# enough. So the two counts must agree.
if(NOT package_count EQUAL stanza_line_count)
    bad("debian/control has ${stanza_line_count} Package: line(s) but only ${package_count} could be read: a stanza that cannot be read is a package no check below looks at. A carriage return at end of line or a trailing comment does this.")
endif()
if(package_count EQUAL 0)
    bad("no Package: stanza was read from debian/control: the file is empty or the parse is broken, and every check below would pass by examining nothing")
endif()
if(app_line_count EQUAL 0)
    bad("no raye_add_app() line was read from CMakeLists.txt: the parse is broken, and no package below can be backed by a folder")
endif()

# ---- which declared folders are really in the tree
set(present_folders "")
set(present_packages "")
foreach(entry IN LISTS folder_packages)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 folder)
    set(package "")
    list(LENGTH parts n)
    if(n GREATER 1)
        list(GET parts 1 package)
    endif()
    if(EXISTS "${SRC}/${folder}/CMakeLists.txt")
        list(APPEND present_folders "${folder}")
        if(package AND NOT package IN_LIST present_packages)
            list(APPEND present_packages "${package}")
        endif()
    endif()
endforeach()

list(LENGTH present_folders present_count)
if(present_count EQUAL 0)
    bad("not one folder named by raye_add_app() is in the tree: either the tree is empty or the folders are read wrongly, and nothing below is being checked")
endif()

# ---- 1. every stanza must be backed by a folder that exists AND installs
foreach(package IN LISTS packages)
    set(backers "")
    set(backed FALSE)
    foreach(entry IN LISTS folder_packages)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 folder)
        set(p "")
        list(LENGTH parts n)
        if(n GREATER 1)
            list(GET parts 1 p)
        endif()
        if(NOT p STREQUAL "${package}")
            continue()
        endif()
        list(APPEND backers "${folder}")
        if(NOT EXISTS "${SRC}/${folder}/CMakeLists.txt")
            continue()
        endif()
        # The folder is there; does anything in it install a file? A folder
        # that compiles and installs nothing ships the same empty .deb.
        #
        # This sub-check leans toward GREEN on purpose, and the pattern is
        # wide for that reason: any call whose name holds "install" counts,
        # so ecm_install_icons() and qt_*_install*() are read as installing
        # -- they do. A narrow "install(" would call an app that uses only
        # ECM's helpers empty, which is a red for the wrong reason, and a
        # test that cries wolf gets switched off. The case this file alone
        # can catch without a build is the stanza whose folder is NOT in
        # the tree; "present but installs nothing" is caught for real, by
        # counting staged files, in RayeDebianInstall.cmake.
        #
        # What this cannot see either: whether the install() carries the
        # right COMPONENT. A file installed under the wrong component goes
        # to no package, and only RayeDebianInstall.cmake notices -- it
        # walks every component and names what is left over.
        file(GLOB_RECURSE folder_lists "${SRC}/${folder}/CMakeLists.txt"
                                       "${SRC}/${folder}/*/CMakeLists.txt")
        # A folder's tests/ installs nothing a user gets; counting it would
        # call a package backed by its own test harness.
        list(FILTER folder_lists EXCLUDE REGEX "/tests?/CMakeLists\\.txt$")
        foreach(fl IN LISTS folder_lists)
            file(READ "${fl}" ftext)
            # A newline is prepended so a file that BEGINS with install()
            # still matches. Without it the pattern needs a character
            # before the call, and a CMakeLists.txt whose first byte starts
            # the call reads as installing nothing -- a red for the wrong
            # reason, measured on a fixture whose whole content is one
            # install() line. The class also keeps ecm_install_icons() and
            # any other <word>_install( from being counted.
            set(probe "\n${ftext}")
            # Comments go first: "# TODO install (later)" would otherwise
            # count as an installation and call the package backed, which
            # is the exact false green this file exists to refuse.
            # A quoted string holding the word stays a residual false
            # green. It is left: this sub-check only has to avoid crying
            # wolf, and "present but installs nothing" is caught for real
            # by counting staged files in RayeDebianInstall.cmake.
            string(REGEX REPLACE "#[^\n]*" "" probe "${probe}")
            if(probe MATCHES "[^A-Za-z0-9_][A-Za-z0-9_]*install[A-Za-z0-9_]*[ \t]*\\(")
                set(backed TRUE)
                break()
            endif()
        endforeach()
        if(backed)
            break()
        endif()
    endforeach()
    if(backed)
        continue()
    endif()
    if(NOT backers)
        bad("debian/control declares ${package}, which no raye_add_app() line names: dpkg builds a .deb for every stanza, so this one ships as an empty package -- /usr/share/doc and nothing else -- with its full Depends still pulled in.")
    else()
        list(JOIN backers ", " where)
        bad("debian/control declares ${package}, but no folder that would fill it installs anything (${where}): dpkg builds a .deb for every stanza, so this one ships /usr/share/doc and nothing else while still pulling in its Depends. Write the folder, or take the stanza out of debian/control until it is written -- naming it NOT_WRITTEN_YET in CMakeLists.txt does not stop the .deb from being built.")
    endif()
endforeach()

# ---- 2. the other direction: a folder in the tree whose files go nowhere
foreach(package IN LISTS present_packages)
    if(NOT package IN_LIST packages)
        bad("${package} is named by a folder that IS in the tree, and has no stanza in debian/control: everything that folder installs goes into no package at all.")
    endif()
endforeach()

if(failures)
    message(FATAL_ERROR "control-ships-files: some checks failed")
endif()
message(STATUS "control-ships-files: ${package_count} package(s) in debian/control, "
               "${app_line_count} raye_add_app() line(s), ${present_count} folder(s) in the tree, every stanza backed")
