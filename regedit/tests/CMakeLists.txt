# Tests of the Registry Editor's logic. They write only in temporary
# folders, and the dconf one in a database of its own.
#
# Every file here must be named below (cmake/tests/test_registration.cmake).

# plain "text" literals read better in tests than QStringLiteral everywhere
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

raye_add_qtest(settingsfile SOURCES test_settingsfile.cpp LIBRARIES raye-regedit-logic)
raye_add_qtest(edits SOURCES test_edits.cpp LIBRARIES raye-regedit-logic)
raye_add_qtest(locations SOURCES test_locations.cpp LIBRARIES raye-regedit-logic)
raye_add_qtest(regfile SOURCES test_regfile.cpp LIBRARIES raye-regedit-logic)
raye_add_qtest(helperargs SOURCES test_helperargs.cpp LIBRARIES raye-regedit-logic)
raye_add_qtest(registry SOURCES test_registry.cpp LIBRARIES raye-regedit-model)

# dconf writes go through dconf-service, on a session bus: give it one,
# and a config folder of its own so nobody's settings are touched
add_executable(raye-regedit-test-dconf test_dconf.cpp)
target_link_libraries(raye-regedit-test-dconf PRIVATE Qt6::Test raye-regedit-logic)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
add_test(NAME raye-regedit/dconf
         COMMAND sh -c "d=$(mktemp -d) && mkdir -m 700 $d/config $d/run && XDG_CONFIG_HOME=$d/config XDG_RUNTIME_DIR=$d/run ${DBUS_RUN_SESSION} -- \"$0\"; rc=$?; rm -rf \"$d\"; exit $rc"
                 $<TARGET_FILE:raye-regedit-test-dconf>)
add_test(NAME raye-regedit/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
add_test(NAME raye-regedit/policy
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_policy.py)
add_test(NAME raye-regedit/filetype
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_filetype.py)
add_test(NAME raye-regedit/popups
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_popups.py)
