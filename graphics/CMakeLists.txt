# raye-graphics: the graphics card panel. Live readings of the card, and
# on AMD cards its performance mode, workload profile, power limit and
# fan curve. NVIDIA cards get their readings and a way to their own
# control panel.
#
# README.md says which sysfs file each setting is, and how the root
# helper checks what it writes.
#
# src/      the window, the root helper and the logic both use
# tests/    QtTests of that logic, on sysfs trees of their own
# po/       the French words

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons)

add_subdirectory(src)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
