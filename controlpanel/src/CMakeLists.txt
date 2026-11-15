add_library(raye-controlpanel-launcher STATIC launcher.cpp launcher.h)
target_include_directories(raye-controlpanel-launcher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-controlpanel-launcher PUBLIC Qt6::Core Qt6::Gui Qt6::Qml KF6::KIOGui KF6::Service)

add_executable(raye-controlpanel)
set_source_files_properties(Catalogue.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(raye-controlpanel
    URI org.raye.controlpanel
    VERSION 1.0
    SOURCES
        main.cpp
        launcher.h
    QML_FILES
        Main.qml
        Catalogue.qml
        Glyph.qml
        TextBox.qml
        LinkText.qml
        CategoryBlock.qml
        ItemTile.qml
        AdminToolsPage.qml
        ItemBlock.qml
        search.js)
target_compile_definitions(raye-controlpanel PRIVATE TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}")
target_link_libraries(raye-controlpanel PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::I18n KF6::DBusAddons Raye::Version raye-controlpanel-launcher)

install(TARGETS raye-controlpanel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.controlpanel.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.raye.controlpanel.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
