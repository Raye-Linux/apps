# raye-regedit: the Registry Editor, Raye edition.
#
# README.md says what the tree shows, which values are read only, and how
# a change is written without breaking the file.
#
# src/      the window, the admin helper, and the logic both use
# tests/    QtTests of that logic, with no display
# po/       the French words

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DCONF REQUIRED IMPORTED_TARGET dconf)

add_subdirectory(src)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
