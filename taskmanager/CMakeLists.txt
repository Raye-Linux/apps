# raye-taskmanager: the Task Manager.
#
# README.md holds the pages, which Linux mechanism fills each of them,
# why App history is not one, and what is not written yet.
#
# src/      the window, and the logic it reads /proc with
# tests/    QtTests of that logic, with no display and no session
# po/       the French words, one fragment per part

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 DBus Network)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons)

add_subdirectory(src)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
