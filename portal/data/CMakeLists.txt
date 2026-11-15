# How xdg-desktop-portal finds this backend, and how it is started.

set(RAYE_PORTAL_INTERFACES
    org.freedesktop.impl.portal.ScreenCast
    org.freedesktop.impl.portal.RemoteDesktop
    org.freedesktop.impl.portal.Clipboard)
if(RAYE_PORTAL_ACCESS)
    list(APPEND RAYE_PORTAL_INTERFACES org.freedesktop.impl.portal.Access)
endif()
list(JOIN RAYE_PORTAL_INTERFACES ";" RAYE_PORTAL_INTERFACE_LIST)

configure_file(raye.portal.in raye.portal @ONLY)
configure_file(org.freedesktop.impl.portal.desktop.raye.service.in
               org.freedesktop.impl.portal.desktop.raye.service @ONLY)
configure_file(org.freedesktop.impl.portal.desktop.raye.desktop.in
               org.freedesktop.impl.portal.desktop.raye.desktop @ONLY)
configure_file(xdg-desktop-portal-raye.service.in xdg-desktop-portal-raye.service @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/raye.portal
        DESTINATION ${KDE_INSTALL_DATADIR}/xdg-desktop-portal/portals)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.raye.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.raye.desktop
        DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-raye.service
        DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
install(FILES xdg-desktop-portal-raye.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
