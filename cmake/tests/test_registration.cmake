# Every test file of this repository is reached by a registration, and
# debian/rules does not narrow the set ctest runs.
#   cmake -DSRC=<repo root> -P test_registration.cmake
#
# Why this exists. On 2026-11-06 the theme repository was found with a
# debian/rules that ENUMERATED test folders and had missed two of them:
# 346 tests that the package build never ran, with nothing to say so. This
# repository is not wired that way -- it has no override_dh_auto_test, so
# dh_auto_test runs the whole of ctest -- but nothing stopped it from
# becoming wired that way, and nothing noticed a test_*.cpp added to
# portal/tests/ without the matching raye_add_qtest() line. Both are
# silent: an unregistered file is simply never compiled, and a narrowed
# ctest still prints "100% tests passed".
cmake_minimum_required(VERSION 3.22)

if(NOT SRC)
    message(FATAL_ERROR "test_registration: SRC is not set")
endif()
get_filename_component(SRC "${SRC}" ABSOLUTE)

set(failures 0)
function(bad msg)
    message(SEND_ERROR "${msg}")
    set(failures "1" PARENT_SCOPE)
endfunction()

# A plain substring, not MATCHES: a file name holds dots, and as a regex
# "test_a.cpp" would also match "test_axcpp".
function(names_file text name out)
    string(FIND "${text}" "${name}" pos)
    if(pos LESS 0)
        set(${out} FALSE PARENT_SCOPE)
    else()
        set(${out} TRUE PARENT_SCOPE)
    endif()
endfunction()

# ---- every test source is named by the CMakeLists.txt of its own folder
# The folder's own list is the right place to look: that is where
# raye_add_qtest() and add_test() live in this tree.
#
# The folders are FOUND, not listed. An earlier version of this file
# named three of them -- portal/tests, portal/qml/tests, cmake/tests --
# and so it had the very defect it exists to catch: a new app's tests/
# folder was not in the list, so the files in it were not checked, and
# the run still said "all registered". Measured on 2026-11-06, when
# taskmanager/tests/ arrived with three files and the guard passed
# without looking at one of them. A list of paths goes stale the day a
# folder is added, and nothing says so; a search cannot.
set(checked 0)
set(folders "")
file(GLOB_RECURSE sources RELATIVE "${SRC}"
     "${SRC}/test_*.cpp" "${SRC}/test_*.py" "${SRC}/test_*.sh" "${SRC}/test_*.cmake")
foreach(src IN LISTS sources)
    # a build folder left inside the tree is not source
    if(src MATCHES "(^|/)(build|_build|\\.git)/")
        continue()
    endif()
    get_filename_component(dir "${src}" DIRECTORY)
    if(NOT dir IN_LIST folders)
        list(APPEND folders "${dir}")
    endif()
endforeach()

foreach(dir IN LISTS folders)
    # the folder's own list, or the build tests' list for the folders
    # that have none of their own (cmake/tests is run by tests/)
    set(lists "${SRC}/${dir}/CMakeLists.txt")
    if(NOT EXISTS "${lists}")
        set(lists "${SRC}/tests/CMakeLists.txt")
    endif()
    if(NOT EXISTS "${lists}")
        bad("${dir}: neither ${dir}/CMakeLists.txt nor tests/CMakeLists.txt exists, so nothing can register these files")
        continue()
    endif()
    file(READ "${lists}" text)
    file(GLOB srcs "${SRC}/${dir}/test_*.cpp" "${SRC}/${dir}/test_*.py"
                   "${SRC}/${dir}/test_*.sh" "${SRC}/${dir}/test_*.cmake")
    foreach(src ${srcs})
        get_filename_component(name "${src}" NAME)
        math(EXPR checked "${checked} + 1")
        names_file("${text}" "${name}" found)
        if(NOT found)
            # raye_add_qtest() takes the stem, so accept that spelling too.
            get_filename_component(stem "${src}" NAME_WE)
            string(REGEX REPLACE "^test_" "" stem "${stem}")
            names_file("${text}" "${stem}" found)
        endif()
        if(NOT found)
            bad("${dir}/${name} is registered nowhere: it is not named in ${lists}, so nothing ever runs it")
        endif()
    endforeach()
endforeach()

# A floor, so that a whole folder disappearing is noticed: the search
# above follows a folder that is renamed, but it cannot follow one that
# is deleted, and a deleted folder of tests looks like a clean run.
list(LENGTH folders folder_count)
if(folder_count LESS 4)
    bad("only ${folder_count} folders of tests were found; one has gone")
endif()
if(checked LESS 12)
    bad("only ${checked} test files were checked; a folder of tests has gone")
endif()

# ---- debian/rules must not narrow what ctest runs
set(rules "${SRC}/debian/rules")
if(NOT EXISTS "${rules}")
    bad("${rules} does not exist")
else()
    file(READ "${rules}" rules_text)
    if(NOT rules_text MATCHES "CTEST_NO_TESTS_ACTION[ \t]*=[ \t]*error")
        bad("debian/rules does not export CTEST_NO_TESTS_ACTION=error: ctest exits 0 when it finds no test")
    endif()
    if(rules_text MATCHES "override_dh_auto_test")
        foreach(narrow "-L" "-R" "-E" "--label-regex" "--tests-regex" "--exclude-regex")
            names_file("${rules_text}" "${narrow}" found)
            if(found)
                bad("debian/rules narrows ctest with ${narrow}: every test it leaves out is a test the package build never runs. If that is wanted, the label must be carried by a glob so no file can fall outside it.")
            endif()
        endforeach()
    endif()
endif()

if(failures)
    message(FATAL_ERROR "registration: some checks failed")
endif()
message(STATUS "registration: ${checked} test files, all registered")
