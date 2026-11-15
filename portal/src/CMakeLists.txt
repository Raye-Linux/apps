# The backend. The files of xdg-desktop-portal-kde keep its words (and
# its translations, installed by xdg-desktop-portal-kde); Raye's code
# and main.cpp use this package's catalog.

# Raye's logic, without Wayland, so the tests can link it
add_library(raye-portal-logic STATIC
    raye/appidentity.cpp raye/appidentity.h
    raye/pickerchoice.cpp raye/pickerchoice.h
    raye/sharesessions.cpp raye/sharesessions.h)
target_include_directories(raye-portal-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(raye-portal-logic PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_PORTAL_PICKER=${RAYE_PORTAL_PICKER})
target_link_libraries(raye-portal-logic PUBLIC
    Qt::Core Qt::DBus KF6::ConfigCore KF6::I18n)

add_executable(xdg-desktop-portal-raye)
ecm_add_qml_module(xdg-desktop-portal-raye
    URI org.kde.xdgdesktopportal
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE)
ecm_target_qml_sources(xdg-desktop-portal-raye
    SOURCES
        ScreenChooserDialog.qml
        RemoteDesktopDialog.qml
        PipeWireDelegate.qml
        PortalDialog.qml
        PipeWireLayout.qml
        ScreenChooserDialogTemplate.qml
        region-select/RegionSelectOverlay.qml
        region-select/FloatingTextBox.qml
        region-select/FloatingBackground.qml)

set(upstream_sources
    clipboard.cpp clipboard.h
    dbushelpers.cpp dbushelpers.h
    desktopportal.cpp desktopportal.h
    notificationinhibition.cpp notificationinhibition.h
    outputsmodel.cpp outputsmodel.h
    quickdialog.cpp quickdialog.h
    remotedesktop.cpp remotedesktop.h
    remotedesktopdialog.cpp remotedesktopdialog.h
    request.cpp request.h
    restoredata.cpp restoredata.h
    screencast.cpp screencast.h
    screencasting.cpp screencasting.h
    screenchooserdialog.cpp screenchooserdialog.h
    session.cpp session.h
    utils.cpp utils.h
    waylandintegration.cpp waylandintegration.h waylandintegration_p.h
    region-select/SelectionEditor.cpp region-select/SelectionEditor.h)
set(raye_sources
    main.cpp
    raye/sharehook.cpp raye/sharehook.h)

set_source_files_properties(${upstream_sources} PROPERTIES
    COMPILE_DEFINITIONS TRANSLATION_DOMAIN="xdg-desktop-portal-kde")
set_source_files_properties(${raye_sources} PROPERTIES
    COMPILE_DEFINITIONS TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}")

set(logging_sources "")
foreach(category IN ITEMS
        "XdgDesktopPortalKde|xdp-raye|debug.h"
        "XdgDesktopPortalKdeClipboard|xdp-raye.clipboard|clipboard_debug.h"
        "XdgDesktopPortalKdeDesktopPortal|xdp-raye.desktop-portal|desktopportal_debug.h"
        "XdgDesktopPortalKdeNotificationInhibition|xdp-raye.notificationinhibition|notificationinhibition_debug.h"
        "XdgDesktopPortalKdeRemoteDesktop|xdp-raye.remotedesktop|remotedesktop_debug.h"
        "XdgDesktopPortalKdeRemoteDesktopDialog|xdp-raye.remote-desktop-dialog|remotedesktopdialog_debug.h"
        "XdgRequestKdeRequest|xdp-raye.request|request_debug.h"
        "XdgDesktopPortalKdeScreenCast|xdp-raye.screencast|screencast_debug.h"
        "XdgSessionKdeSession|xdp-raye.session|session_debug.h"
        "XdgDesktopPortalKdeWaylandIntegration|xdp-raye.wayland-integration|waylandintegration_debug.h")
    string(REPLACE "|" ";" parts "${category}")
    list(GET parts 0 identifier)
    list(GET parts 1 name)
    list(GET parts 2 header)
    ecm_qt_declare_logging_category(logging_sources
        IDENTIFIER ${identifier} CATEGORY_NAME ${name} HEADER ${header})
endforeach()

qt_add_dbus_adaptor(dbus_sources org.freedesktop.impl.portal.Session.xml session.h Session)
set_source_files_properties(../data/org.freedesktop.impl.portal.PermissionStore.xml PROPERTIES
    NO_NAMESPACE TRUE INCLUDE "dbushelpers.h")
qt_add_dbus_interface(dbus_sources ../data/org.freedesktop.impl.portal.PermissionStore.xml permission_store)

qt6_generate_wayland_protocol_client_sources(xdg-desktop-portal-raye
    PRIVATE_CODE
    FILES
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml
        ${Wayland_DATADIR}/wayland.xml)

target_sources(xdg-desktop-portal-raye PRIVATE
    ${upstream_sources} ${raye_sources} ${logging_sources} ${dbus_sources})
# the folders other packages add (access/...) include as "access/..."
target_include_directories(xdg-desktop-portal-raye PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(xdg-desktop-portal-raye PRIVATE
    RAYE_PORTAL_PICKER=${RAYE_PORTAL_PICKER}
    RAYE_PORTAL_ACCESS=${RAYE_PORTAL_ACCESS})
target_link_libraries(xdg-desktop-portal-raye PRIVATE
    raye-portal-logic
    Raye::Version
    Qt::Core Qt::GuiPrivate Qt::DBus Qt::Widgets Qt::Quick Qt::Qml Qt::WaylandClient
    KF6::CoreAddons KF6::ConfigCore KF6::ConfigGui KF6::GuiAddons KF6::I18n
    KF6::Notifications KF6::WindowSystem KF6::StatusNotifierItem KF6::Crash
    Plasma::KWaylandClient Wayland::Client)

install(TARGETS xdg-desktop-portal-raye DESTINATION ${KDE_INSTALL_LIBEXECDIR})
