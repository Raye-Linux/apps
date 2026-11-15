# xdg-desktop-portal-raye: the screen-sharing backend of the desktop
# portal. ScreenCast and RemoteDesktop (and the Clipboard of remote
# desktop sessions) come here; every other portal stays with the
# desktop's own backend (config/kde-portals.conf).
#
# src/ carries files of xdg-desktop-portal-kde (LGPL), listed with their
# upstream checksums in UPSTREAM; sync-upstream.sh shows what a new
# upstream release changed in them. Raye's own code is in src/raye/;
# edits to the upstream files are marked "Raye:".
#
# Folders other packages add, each used when it has a CMakeLists.txt:
#   qml/        the Raye share picker (a static QML module
#               org.raye.portal.picker, target raye-portal-picker, root
#               component SharePicker); see README.md for its properties
#   indicator/  the sharing indicator; it may add sources to the
#               xdg-desktop-portal-raye target and define
#               RAYE_PORTAL_OWN_INDICATOR to hide the stock tray item
#   access/     the Access portal (permission prompts): sources added to
#               the target, class RayeAccessPortal in
#               access/rayeaccessportal.h; raye.portal then lists Access

# the Wayland protocol code the scanner writes is C
enable_language(C)

set(RAYE_PORTAL_QT_MIN 6.10.0)
set(RAYE_PORTAL_KF_MIN 6.22.0)

find_package(Qt6 ${RAYE_PORTAL_QT_MIN} REQUIRED COMPONENTS
    Core DBus Gui Widgets Qml Quick QuickControls2 WaylandClient)
find_package(Qt6GuiPrivate ${RAYE_PORTAL_QT_MIN} REQUIRED NO_MODULE)
find_package(KF6 ${RAYE_PORTAL_KF_MIN} REQUIRED COMPONENTS
    CoreAddons Config I18n GuiAddons Notifications WindowSystem
    StatusNotifierItem Crash)
find_package(KWayland 6.6 REQUIRED)
find_package(Wayland 1.15 REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols 1.7.0 REQUIRED)

include(ECMQmlModule)
include(ECMQtDeclareLoggingCategory)
include(ECMConfiguredInstall)

set(RAYE_PORTAL_PICKER 0)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qml/CMakeLists.txt)
    set(RAYE_PORTAL_PICKER 1)
endif()
set(RAYE_PORTAL_ACCESS 0)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/access/CMakeLists.txt)
    set(RAYE_PORTAL_ACCESS 1)
endif()

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)
if(RAYE_PORTAL_PICKER)
    add_subdirectory(qml)
    target_link_libraries(xdg-desktop-portal-raye PRIVATE raye-portal-pickerplugin)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/indicator/CMakeLists.txt)
    add_subdirectory(indicator)
endif()
if(RAYE_PORTAL_ACCESS)
    add_subdirectory(access)
endif()

install(FILES config/kde-portals.conf DESTINATION ${KDE_INSTALL_CONFDIR}/xdg-desktop-portal)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
