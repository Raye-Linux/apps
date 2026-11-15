# Tests of the Control Panel: the launcher, the search box, the list of
# items against Settings' pages and the image, and the French.
#
# Every file here must be named below (cmake/tests/test_registration.cmake).

remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

raye_add_qtest(launcher SOURCES test_launcher.cpp LIBRARIES raye-controlpanel-launcher)
raye_add_qtest(search SOURCES test_search.cpp LIBRARIES Qt6::Qml)
target_compile_definitions(raye-controlpanel-test-search PRIVATE SEARCH_JS="${CMAKE_CURRENT_SOURCE_DIR}/../src/search.js")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME raye-controlpanel/catalogue
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_catalogue.py)
# a missing settings layout is a skip, never a pass
set_tests_properties(raye-controlpanel/catalogue PROPERTIES SKIP_REGULAR_EXPRESSION "skipped=")
add_test(NAME raye-controlpanel/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
