# Tests of the graphics panel's logic, on sysfs trees written by the
# tests themselves (fakecards.h): no card, no display, no root.
#
# Every file here must be named below (cmake/tests/test_registration.cmake).

# plain "text" literals read better in tests than QStringLiteral everywhere
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

raye_add_qtest(cards SOURCES test_cards.cpp fakecards.h LIBRARIES raye-graphics-logic)
raye_add_qtest(amdgpu SOURCES test_amdgpu.cpp fakecards.h LIBRARIES raye-graphics-logic)
raye_add_qtest(tuning SOURCES test_tuning.cpp fakecards.h LIBRARIES raye-graphics-logic)
raye_add_qtest(fancurve SOURCES test_fancurve.cpp LIBRARIES raye-graphics-logic)
raye_add_qtest(nvidiasmi SOURCES test_nvidiasmi.cpp LIBRARIES raye-graphics-logic)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME raye-graphics/files
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_files.py)
add_test(NAME raye-graphics/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
