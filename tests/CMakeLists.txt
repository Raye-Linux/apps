# Tests of the build itself: the CMake helpers every app uses and the
# Debian install. The apps' own tests live in their folders.

set(RAYE_CMAKE_TESTS ${PROJECT_SOURCE_DIR}/cmake/tests)
set(RAYE_TEST_WORK ${CMAKE_CURRENT_BINARY_DIR}/work)

add_test(NAME build/version
    COMMAND ${CMAKE_COMMAND} -DWORK=${RAYE_TEST_WORK}/version
            -P ${RAYE_CMAKE_TESTS}/test_version.cmake)

# Every test file is reached by a registration, and debian/rules does not
# narrow the set ctest runs (the defect found in theme on 2026-11-06).
add_test(NAME build/registration
    COMMAND ${CMAKE_COMMAND} -DSRC=${PROJECT_SOURCE_DIR}
            -P ${RAYE_CMAKE_TESTS}/test_registration.cmake)

add_test(NAME build/po-check
    COMMAND ${CMAKE_COMMAND} -DWORK=${RAYE_TEST_WORK}/po-check
            -DSCRIPT=${PROJECT_SOURCE_DIR}/cmake/RayeCheckPo.cmake
            -P ${RAYE_CMAKE_TESTS}/test_po_check.cmake)

# A folder named by raye_add_app() either exists or is on the ledger of
# folders not written yet: 11 packages were being built empty in silence
# until 2026-11-06. Builds small trees of its own, and also reads this
# repository's own CMakeLists.txt, which is why it needs SRC.
add_test(NAME build/app-ledger
    COMMAND ${CMAKE_COMMAND} -DWORK=${RAYE_TEST_WORK}/app-ledger
            -DRAYE_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            -DSRC=${PROJECT_SOURCE_DIR}
            "-DGENERATOR=${CMAKE_GENERATOR}"
            -P ${RAYE_CMAKE_TESTS}/test_app_ledger.cmake)
set_tests_properties(build/app-ledger PROPERTIES TIMEOUT 600)

# Every stanza of debian/control is backed by a folder that is in the tree
# and installs something. The ledger above makes an unwritten folder be
# NAMED; it does not stop its package from being DECLARED, and a declared
# package is a .deb dpkg builds. Eleven shipped holding only
# /usr/share/doc until 2026-11-06. Reads the two files, builds nothing.
add_test(NAME build/control-ships-files
    COMMAND ${CMAKE_COMMAND} -DSRC=${PROJECT_SOURCE_DIR}
            -P ${RAYE_CMAKE_TESTS}/test_control_ships_files.cmake)

# builds small trees of its own, with the generator of this build
add_test(NAME build/packaging
    COMMAND ${CMAKE_COMMAND} -DWORK=${RAYE_TEST_WORK}/packaging
            -DRAYE_CMAKE_DIR=${PROJECT_SOURCE_DIR}/cmake
            "-DGENERATOR=${CMAKE_GENERATOR}" -DWITH_QT=ON
            -P ${RAYE_CMAKE_TESTS}/test_packaging.cmake)
set_tests_properties(build/packaging PROPERTIES TIMEOUT 600)

# the real tree: debian/control and the app list agree, and every file
# installed so far goes to a package
add_test(NAME build/debian-install
    COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DDEBIAN_DIR=${RAYE_TEST_WORK}/debian-install
            -DCONTROL=${PROJECT_SOURCE_DIR}/debian/control
            -P ${PROJECT_SOURCE_DIR}/cmake/RayeDebianInstall.cmake)

# what the apps show says root and sudo, never administrator
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME build/root-terms
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_root_terms.py)

# what the apps show is American English
add_test(NAME build/american
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_american.py)

# org.raye.ui comes with raye-style: every app that imports it depends on it
add_test(NAME build/style-depends
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_style_depends.py)
