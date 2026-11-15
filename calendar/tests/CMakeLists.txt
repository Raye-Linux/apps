# Tests of the calendar's logic: no display, no session, no systemd.
#
# Every file here must be named below (cmake/tests/test_registration.cmake).

raye_add_qtest(ical SOURCES test_ical.cpp LIBRARIES raye-calendar-logic)
raye_add_qtest(recurrence SOURCES test_recurrence.cpp LIBRARIES raye-calendar-logic)
raye_add_qtest(reminders SOURCES test_reminders.cpp LIBRARIES raye-calendar-logic)
raye_add_qtest(store SOURCES test_store.cpp LIBRARIES raye-calendar-logic)
# the zone the expected local times are written in
set_property(TEST raye-calendar/ical raye-calendar/recurrence raye-calendar/reminders raye-calendar/store
             APPEND PROPERTY ENVIRONMENT "TZ=America/Toronto")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME raye-calendar/dates
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_dates.py)
add_test(NAME raye-calendar/french
         COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_french.py)
