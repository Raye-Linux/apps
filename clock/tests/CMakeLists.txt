# Tests of the clock's logic: no display, no session, no systemd. Times
# are read in named zones, so the machine's zone does not matter.
#
# Every file here must be named below (cmake/tests/test_registration.cmake).

# plain "text" literals read better in tests than QStringLiteral everywhere
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

raye_add_qtest(alarms SOURCES test_alarms.cpp LIBRARIES raye-clock-logic)
raye_add_qtest(rings SOURCES test_rings.cpp LIBRARIES raye-clock-logic)
raye_add_qtest(countdown SOURCES test_countdown.cpp LIBRARIES raye-clock-logic)
raye_add_qtest(worldtime SOURCES test_worldtime.cpp LIBRARIES raye-clock-logic)
raye_add_qtest(rowmodel SOURCES test_rowmodel.cpp LIBRARIES raye-clock-logic)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME raye-clock/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
add_test(NAME raye-clock/sounds
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_sounds.py
                 ${PROJECT_SOURCE_DIR}/clock/sounds/make-sounds.py)
