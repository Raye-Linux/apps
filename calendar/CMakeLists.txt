# raye-calendar: a calendar of dates, with reminders.
#
# README.md says where the events live and how a reminder reaches the
# screen without anything running in between.
#
# src/      the window, the reminder tool and the logic both use
# tests/    QtTests of that logic, with no display and no session
# po/       the French words

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons)

add_subdirectory(src)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
