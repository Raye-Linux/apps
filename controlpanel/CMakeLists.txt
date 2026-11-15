# raye-controlpanel: the Control Panel. Its items open the pages of Raye
# Settings; System Tools is a page of its own.
#
# src/      the window, the list of items (Catalogue.qml), the launcher
# tests/    the items against Settings' pages, the launcher, the French

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n DBusAddons KIO Service)

add_subdirectory(src)
add_subdirectory(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
