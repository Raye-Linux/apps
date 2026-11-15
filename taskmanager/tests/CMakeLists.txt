# Tests of the Task Manager's logic. They run without a display and
# without a session: every one of them either reads a line of its own or
# starts a sleep(1) of its own.
#
# Every file here must be named below. cmake/tests/test_registration.cmake
# fails the build when one is not: an unregistered test is never compiled,
# and nothing else says so.

# plain "text" literals read better in tests than QStringLiteral everywhere
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

raye_add_qtest(proctable SOURCES test_proctable.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(endtask SOURCES test_endtask.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(efficiency SOURCES test_efficiency.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(sysinfo SOURCES test_sysinfo.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(appgroups SOURCES test_appgroups.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(autostart SOURCES test_autostart.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(services SOURCES test_services.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(rowdiff SOURCES test_rowdiff.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(devicelist SOURCES test_devicelist.cpp LIBRARIES raye-taskmanager-logic)
raye_add_qtest(runtask SOURCES test_runtask.cpp LIBRARIES raye-taskmanager-logic)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME raye-taskmanager/columns
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_columns.py)
add_test(NAME raye-taskmanager/performance
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_performance.py)
add_test(NAME raye-taskmanager/popups
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_popups.py)
add_test(NAME raye-taskmanager/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
