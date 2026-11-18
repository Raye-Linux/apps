# raye-clock: alarms, timers, a stopwatch and world clocks.
#
# README.md says where things are kept and how an alarm rings with the
# window closed.
#
# src/      the window, the ring tool and the logic both use
# sounds/   the alarm sounds, made at build time
# tests/    QtTests of that logic, with no display and no session
# po/       the French words

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus Multimedia)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons)

add_subdirectory(src)
add_subdirectory(sounds)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
