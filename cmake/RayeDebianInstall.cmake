# Installs every app's CMake component into its Debian package folder,
# then checks the result. Run by debian/rules instead of dh_auto_install:
#   cmake -DBUILD_DIR=<build> -DDEBIAN_DIR=<debian> -DCONTROL=<debian/control>
#         -P RayeDebianInstall.cmake
#
# Fails when debian/control and the raye_add_app() lines disagree -- and the
# agreement is asymmetrical, see the three rules below -- when a file was
# installed but belongs to no package (a typo in a COMPONENT name, or the
# shared chrome installing something), when two packages ship the same
# file, or when a package lacks a file named by REQUIRES in
# raye_add_app().

# Run with "cmake -P", so this file is its own policy scope: nothing above
# it sets one. Without this line CMP0057 stays OLD, the IN_LIST below is
# not an operator, and every check dies with "Unknown arguments specified"
# -- measured 2026-11-06 with cmake 3.31.6, where it made build/packaging
# and build/debian-install red outside a build chroot while both were green
# inside one. The version is the project's own (CMakeLists.txt line 1).
cmake_minimum_required(VERSION 3.22)

foreach(var BUILD_DIR DEBIAN_DIR CONTROL)
    if(NOT ${var})
        message(FATAL_ERROR "RayeDebianInstall: ${var} is not set")
    endif()
    get_filename_component(${var} "${${var}}" ABSOLUTE)
endforeach()

set(list_file "${BUILD_DIR}/raye-packages.list")
if(NOT EXISTS "${list_file}")
    message(FATAL_ERROR "RayeDebianInstall: ${list_file} is missing; configure first")
endif()

file(STRINGS "${list_file}" lines)
set(present "")
set(absent "")
set(requires "")
foreach(line IN LISTS lines)
    if(line MATCHES "^package ([^ ]+) present$")
        list(APPEND present ${CMAKE_MATCH_1})
    elseif(line MATCHES "^package ([^ ]+) absent$")
        list(APPEND absent ${CMAKE_MATCH_1})
    elseif(line MATCHES "^require ([^ ]+) (/.+)$")
        list(APPEND requires "${CMAKE_MATCH_1}|${CMAKE_MATCH_2}")
    elseif(NOT line STREQUAL "")
        message(FATAL_ERROR "RayeDebianInstall: bad line in ${list_file}: ${line}")
    endif()
endforeach()

function(install_into destdir)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${destdir}
                ${CMAKE_COMMAND} --install ${BUILD_DIR} ${ARGN}
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "RayeDebianInstall: cmake --install ${ARGN} failed")
    endif()
endfunction()

function(files_under dir out)
    set(result "")
    if(IS_DIRECTORY "${dir}")
        file(GLOB_RECURSE result LIST_DIRECTORIES false RELATIVE "${dir}" "${dir}/*")
    endif()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

set(errors "")

# The two lists must agree, and in a way that cannot ship an empty .deb.
# dpkg builds a .deb for EVERY stanza of debian/control, whatever the app
# ledger says, so the three rules below are not symmetrical:
#   - a package whose folder is present MUST have a stanza, or everything
#     that folder installs goes into no package at all;
#   - a package whose folder is NOT written MUST NOT have a stanza, because
#     that stanza alone builds a .deb holding nothing but /usr/share/doc
#     while still pulling in its Depends -- eleven of those shipped from
#     this repository until 2026-11-06, every build green;
#   - so every stanza must name a package that is present.
# PACKAGE on an unwritten folder in CMakeLists.txt stays legitimate: it
# reserves the name and keeps its maintainer scripts meaningful. The stanza
# is what waits. cmake/tests/test_control_ships_files.cmake holds the red
# without needing a build.
if(NOT EXISTS "${CONTROL}")
    message(FATAL_ERROR "RayeDebianInstall: ${CONTROL} does not exist")
endif()
file(STRINGS "${CONTROL}" stanza_lines REGEX "^Package:")
set(control_packages "")
foreach(line IN LISTS stanza_lines)
    if(line MATCHES "^Package:[ \t]*([a-z0-9][a-z0-9.+-]*)[ \t]*$")
        list(APPEND control_packages ${CMAKE_MATCH_1})
    endif()
endforeach()
# A Package: line this regex cannot read is a package none of the three
# rules below examines, and dpkg builds its .deb all the same. A CR left
# by an editor, or a trailing comment, is enough to do it.
list(LENGTH stanza_lines stanza_line_count)
list(LENGTH control_packages control_count)
if(NOT stanza_line_count EQUAL control_count)
    list(APPEND errors "debian/control has ${stanza_line_count} Package: line(s) but only ${control_count} could be read: a stanza that cannot be read is a package no check here looks at, while dpkg still builds it")
endif()
foreach(package IN LISTS present)
    if(NOT package IN_LIST control_packages)
        list(APPEND errors "${package} is named by a folder that IS in the tree and has no stanza in debian/control: everything that folder installs goes into no package at all")
    endif()
endforeach()
foreach(package IN LISTS absent)
    if(package IN_LIST control_packages)
        list(APPEND errors "debian/control declares ${package} while its folder is not written: dpkg builds a .deb for every stanza, so this one ships /usr/share/doc and nothing else with its full Depends. Take the stanza out until the folder is written -- NOT_WRITTEN_YET in CMakeLists.txt does not stop the .deb from being built")
    endif()
endforeach()
foreach(package IN LISTS control_packages)
    if(NOT package IN_LIST present AND NOT package IN_LIST absent)
        list(APPEND errors "debian/control has ${package}, which no app folder in CMakeLists.txt names")
    endif()
endforeach()
if(errors)
    list(JOIN errors "\n  " text)
    message(FATAL_ERROR "RayeDebianInstall:\n  ${text}")
endif()

set(shipped "")
foreach(package IN LISTS present)
    set(dest "${DEBIAN_DIR}/${package}")
    install_into("${dest}" --component ${package})
    files_under("${dest}" files)
    list(LENGTH files count)
    message(STATUS "${package}: ${count} file(s)")
    # The other half of "declared but empty", and the half no ledger
    # entry excuses: the folder IS in the tree, so there is nothing left
    # to wait for. This was a message(WARNING) until 2026-11-06, which in
    # an 8 MB build log is the same as saying nothing.
    if(count EQUAL 0)
        list(APPEND errors
             "${package}: its folder is in the tree and yet the package installs nothing, so it would ship as an empty package. A folder marked NOT_WRITTEN_YET in raye_add_app() is expected to be empty; this one is not marked, so either it must install something or its install() calls name the wrong component.")
    endif()
    foreach(f IN LISTS files)
        if(f IN_LIST shipped)
            list(APPEND errors "/${f} is in two packages (one of them ${package})")
        endif()
    endforeach()
    list(APPEND shipped ${files})
endforeach()

# Named one by one, and counted: the count is the figure to watch, and
# it is the figure nobody had until 2026-11-06.
list(LENGTH absent absent_count)
foreach(package IN LISTS absent)
    message(STATUS "${package}: its folder is marked NOT_WRITTEN_YET; the name is reserved and no .deb is built for it")
endforeach()
if(absent_count GREATER 0)
    message(STATUS "RayeDebianInstall: ${absent_count} package name(s) reserved for folders not written yet, none of them built")
endif()

foreach(item IN LISTS requires)
    string(REPLACE "|" ";" item "${item}")
    list(GET item 0 package)
    list(GET item 1 path)
    if(NOT EXISTS "${DEBIAN_DIR}/${package}${path}" AND NOT IS_SYMLINK "${DEBIAN_DIR}/${package}${path}")
        list(APPEND errors "${package} must ship ${path} (its maintainer scripts use it)")
    endif()
endforeach()

# everything, every component: what is left over belongs to no package
set(all "${BUILD_DIR}/raye-install-all")
file(REMOVE_RECURSE "${all}")
install_into("${all}")
files_under("${all}" every)
foreach(f IN LISTS every)
    if(NOT f IN_LIST shipped)
        list(APPEND errors "/${f} is installed but in no package (check its COMPONENT)")
    endif()
endforeach()
file(REMOVE_RECURSE "${all}")

if(errors)
    list(JOIN errors "\n  " text)
    message(FATAL_ERROR "RayeDebianInstall:\n  ${text}")
endif()
message(STATUS "RayeDebianInstall: every file is in its package")
