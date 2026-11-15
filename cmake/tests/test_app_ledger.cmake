# A folder named by raye_add_app() either exists, or is on the ledger of
# folders not written yet. Nothing in between, and nothing silent.
#   cmake -DWORK=<dir> -DRAYE_CMAKE_DIR=<cmake/> -DGENERATOR=<gen> -DSRC=<repo root>
#         -P test_app_ledger.cmake
#
# Why this exists. Measured on 2026-11-06: of the 15 folders named by
# raye_add_app(), THIRTEEN did not exist, and raye_add_app() skipped each
# one with one STATUS line in an 8 MB build log. Eleven Debian packages
# were therefore built with nothing in them but /usr/share/doc --
# changelog.gz and copyright, 1.8 kB -- while each still declared its
# full Depends. raye-taskmanager had been in that state since the repo
# began, and the plan of record said it was "to finish" when its folder
# had never existed at all. The build was green every time.
#
# The silence is the defect, not the absence: declaring an app before
# writing it is a legitimate way to work. So an unwritten folder must be
# NAMED on its raye_add_app() line, and the two reds below keep that
# ledger honest in both directions -- it can only shrink.
cmake_minimum_required(VERSION 3.22)

foreach(var WORK RAYE_CMAKE_DIR SRC)
    if(NOT ${var})
        message(FATAL_ERROR "test_app_ledger: ${var} is not set")
    endif()
endforeach()
get_filename_component(SRC "${SRC}" ABSOLUTE)
if(NOT GENERATOR)
    set(GENERATOR "Unix Makefiles")
endif()

set(failed FALSE)
macro(fail text)
    message(SEND_ERROR "${text}")
    set(failed TRUE)
endmacro()

# Two traps, both met while writing this file.
#
# message() WRAPS its text at about 72 columns and indents the wrapped
# lines, so any expected phrase longer than a word or two can be cut by
# a newline: the search then fails or succeeds depending on how long the
# surrounding sentence happens to be. Both sides are squeezed to single
# spaces before searching.
#
# And a failure raised inside a FUNCTION does not reach the caller:
# set() there is scoped to the function, so an earlier version of this
# file printed "all checks passed" under a wall of errors. PARENT_SCOPE
# is what carries it out. Every call below is made from the top level or
# a top-level foreach(), which share that scope.
function(says log text what)
    string(REGEX REPLACE "[ \t\r\n]+" " " flat "${log}")
    string(REGEX REPLACE "[ \t\r\n]+" " " want "${text}")
    string(FIND "${flat}" "${want}" at)
    if(at EQUAL -1)
        message(SEND_ERROR "${what}: the message does not say \"${text}\":\n${log}")
        set(failed TRUE PARENT_SCOPE)
    endif()
endfunction()

# ---- the fixture tree ------------------------------------------------
#
# alpha is written and ships a file; beta is declared and not written;
# shared is the chrome-like folder, declared without a package.

function(write_tree root variant)
    file(REMOVE_RECURSE ${root})

    set(alpha_mark "")
    set(beta_mark " NOT_WRITTEN_YET")
    set(shared_mark "")
    set(gamma_line "")
    set(gamma_stanza "")
    if(variant STREQUAL "stale-ledger")
        set(alpha_mark " NOT_WRITTEN_YET")
    elseif(variant STREQUAL "unmarked-absent")
        set(beta_mark "")
    elseif(variant STREQUAL "stale-ledger-shared")
        set(shared_mark " NOT_WRITTEN_YET")
    elseif(variant STREQUAL "ships-nothing")
        set(gamma_line "raye_add_app(gamma PACKAGE raye-gamma)")
        set(gamma_stanza "\nPackage: raye-gamma\nArchitecture: any\n")
    endif()

    file(WRITE ${root}/CMakeLists.txt "
cmake_minimum_required(VERSION 3.22)
project(fixture LANGUAGES NONE)
list(APPEND CMAKE_MODULE_PATH ${RAYE_CMAKE_DIR})
include(RayeApps)
raye_add_app(shared${shared_mark})
raye_add_app(alpha PACKAGE raye-alpha REQUIRES bin/raye-alpha${alpha_mark})
raye_add_app(beta PACKAGE raye-beta${beta_mark})
${gamma_line}
raye_write_package_list()
")
    # shared code: installs nothing, and has no package to install into
    file(WRITE ${root}/shared/CMakeLists.txt "# linked into every app\n")
    file(WRITE ${root}/alpha/CMakeLists.txt "install(PROGRAMS raye-alpha DESTINATION bin)\n")
    file(WRITE ${root}/alpha/raye-alpha "#!/bin/sh\necho alpha\n")
    # beta has no CMakeLists.txt: the folder on the ledger
    file(WRITE ${root}/beta/README "later\n")
    # gamma is written but installs nothing: a package built empty while
    # its folder is there, which no ledger entry excuses
    file(WRITE ${root}/gamma/CMakeLists.txt "# nothing installed yet\n")

    # the chrome-like folder, gone, for the two variants that need it
    if(variant STREQUAL "marked-absent-shared" OR variant STREQUAL "unmarked-absent-shared")
        file(REMOVE_RECURSE ${root}/shared)
        if(variant STREQUAL "marked-absent-shared")
            file(READ ${root}/CMakeLists.txt t)
            string(REPLACE "raye_add_app(shared)" "raye_add_app(shared NOT_WRITTEN_YET)" t "${t}")
            file(WRITE ${root}/CMakeLists.txt "${t}")
        endif()
    endif()

    file(WRITE ${root}/debian/control
         "Source: fixture\nBuild-Depends: cmake\n\nPackage: raye-alpha\nArchitecture: any\n${gamma_stanza}")
endfunction()

function(configure root build out_ok out_log)
    file(REMOVE_RECURSE ${build})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${root} -B ${build} -G "${GENERATOR}"
                -DCMAKE_INSTALL_PREFIX=/usr
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc EQUAL 0)
        set(${out_ok} TRUE PARENT_SCOPE)
    else()
        set(${out_ok} FALSE PARENT_SCOPE)
    endif()
    set(${out_log} "${out}\n${err}" PARENT_SCOPE)
endfunction()

function(debian_install build debian control out_ok out_log)
    file(REMOVE_RECURSE ${debian})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build} -DDEBIAN_DIR=${debian}
                -DCONTROL=${control} -P ${RAYE_CMAKE_DIR}/RayeDebianInstall.cmake
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc EQUAL 0)
        set(${out_ok} TRUE PARENT_SCOPE)
    else()
        set(${out_ok} FALSE PARENT_SCOPE)
    endif()
    set(${out_log} "${out}\n${err}" PARENT_SCOPE)
endfunction()

# ---- a folder on the ledger is allowed, and is NAMED -----------------

set(root ${WORK}/good-src)
set(build ${WORK}/good-build)
write_tree(${root} good)
configure(${root} ${build} ok log)
if(NOT ok)
    message(FATAL_ERROR "a folder marked NOT_WRITTEN_YET must still configure:\n${log}")
endif()
# the log must name the PACKAGE, not merely the folder it skipped: a
# reader of the log has to be able to list the reserved package names
# without cross-reading debian/control.
says("${log}" "beta/ is not written yet" "ledger entry")
says("${log}" "raye-beta is not built" "ledger entry")
if(EXISTS ${build}/raye-packages.list)
    file(READ ${build}/raye-packages.list list)
    set(want "package raye-alpha present\npackage raye-beta absent\nrequire raye-alpha /usr/bin/raye-alpha\n")
    if(NOT list STREQUAL want)
        fail("package list of the good tree:\n${list}\nwanted:\n${want}")
    endif()
else()
    fail("the good tree wrote no raye-packages.list")
endif()

# a packageless folder on the ledger is allowed too, and says so without
# inventing a package name for it
set(root ${WORK}/shared-absent-src)
write_tree(${root} marked-absent-shared)
configure(${root} ${WORK}/shared-absent-build ok log)
if(NOT ok)
    fail("a packageless folder marked NOT_WRITTEN_YET must still configure:\n${log}")
else()
    says("${log}" "shared/ is not written yet" "packageless ledger entry")
endif()

# ---- red 1: declared, absent, and NOT on the ledger ------------------

foreach(case unmarked-absent unmarked-absent-shared)
    if(case STREQUAL "unmarked-absent")
        set(folder "beta")
        set(extra "nothing would fill raye-beta")
    else()
        set(folder "shared")
        set(extra "")
    endif()
    set(root ${WORK}/${case}-src)
    write_tree(${root} ${case})
    configure(${root} ${WORK}/${case}-build ok log)
    if(ok)
        fail("${case}: ${folder}/ does not exist and is not on the ledger, so configure should have stopped")
    else()
        says("${log}" "${folder}/CMakeLists.txt does not exist" "${case}")
        says("${log}" "NOT_WRITTEN_YET" "${case}")
        if(extra)
            says("${log}" "${extra}" "${case}")
        endif()
    endif()
endforeach()

# ---- red 2: on the ledger, and the folder exists ---------------------
# Without this one the ledger rots: in six months it would cover folders
# that exist and would go on excusing the next one that does not.

foreach(case stale-ledger stale-ledger-shared)
    if(case STREQUAL "stale-ledger")
        set(folder "alpha")
    else()
        set(folder "shared")
    endif()
    set(root ${WORK}/${case}-src)
    write_tree(${root} ${case})
    configure(${root} ${WORK}/${case}-build ok log)
    if(ok)
        fail("${case}: ${folder}/ exists while still marked NOT_WRITTEN_YET, so configure should have stopped")
    else()
        says("${log}" "${folder}/ exists now" "${case}")
        says("${log}" "drop NOT_WRITTEN_YET" "${case}")
    endif()
endforeach()

# ---- red 3: the folder is there and still produces nothing -----------
# The other half of "declared but empty". No ledger entry excuses this:
# the folder exists, so there is nothing left to wait for.

set(root ${WORK}/ships-nothing-src)
set(build ${WORK}/ships-nothing-build)
write_tree(${root} ships-nothing)
configure(${root} ${build} ok log)
if(NOT ok)
    fail("ships-nothing: the tree should configure:\n${log}")
else()
    debian_install(${build} ${WORK}/ships-nothing-debian ${root}/debian/control ok log)
    if(ok)
        fail("ships-nothing: raye-gamma ships no file at all, so the Debian install should have stopped")
    else()
        says("${log}" "raye-gamma" "ships-nothing")
        says("${log}" "installs nothing" "ships-nothing")
    endif()
endif()

# ---- this repository's own ledger, right now -------------------------
# The fixtures prove the mechanism; this proves the real CMakeLists.txt.
# It is a static read on purpose: the apps tree cannot be configured
# without KF6I18n, so a check that needed a configure would simply not
# run on a machine that lacks it, and would report nothing while doing
# nothing.

set(top "${SRC}/CMakeLists.txt")
if(NOT EXISTS "${top}")
    fail("${top} does not exist")
else()
    file(STRINGS "${top}" top_lines REGEX "^[ \t]*raye_add_app\\(")
    set(declared 0)
    set(on_ledger 0)
    foreach(line IN LISTS top_lines)
        if(NOT line MATCHES "raye_add_app\\([ \t]*([A-Za-z0-9_-]+)")
            fail("cannot read the folder name out of: ${line}")
            continue()
        endif()
        set(folder "${CMAKE_MATCH_1}")
        math(EXPR declared "${declared} + 1")
        set(marked FALSE)
        if(line MATCHES "NOT_WRITTEN_YET")
            set(marked TRUE)
            math(EXPR on_ledger "${on_ledger} + 1")
        endif()
        set(exists FALSE)
        if(EXISTS "${SRC}/${folder}/CMakeLists.txt")
            set(exists TRUE)
        endif()
        if(exists AND marked)
            fail("CMakeLists.txt: ${folder}/ exists, so drop NOT_WRITTEN_YET from its raye_add_app() line")
        elseif(NOT exists AND NOT marked)
            fail("CMakeLists.txt: ${folder}/CMakeLists.txt does not exist, so nothing would fill its package and a stanza in debian/control would ship it empty, with nothing said; put NOT_WRITTEN_YET on its raye_add_app() line, or write the folder")
        endif()
    endforeach()
    # A floor, so the whole list vanishing is not a clean run.
    if(declared LESS 10)
        fail("only ${declared} raye_add_app() lines were read from ${top}; the list has gone")
    endif()
    message(STATUS "app ledger: ${declared} folders declared, ${on_ledger} not written yet")
endif()

if(failed)
    message(FATAL_ERROR "app-ledger: some checks failed")
endif()
message(STATUS "app-ledger: all checks passed")
