# raye-app-identity: what the stock apps are called and which icon they
# use in Start, search and Open with. A table and a generator, no C++.
#
# identity.json             the names, in English and French
# raye-app-overrides        writes them next to the stock .desktop files
# raye-app-overrides.sh     puts that folder first in XDG_DATA_DIRS
#
# Window titles still carry the app's own name: Qt adds it, no setting
# changes it, and it goes away with each app Raye replaces.

install(PROGRAMS raye-app-overrides DESTINATION libexec/raye-app-identity)
install(FILES identity.json DESTINATION ${KDE_INSTALL_DATADIR}/raye/app-identity)
install(FILES raye-app-overrides.sh DESTINATION /etc/xdg/plasma-workspace/env)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
