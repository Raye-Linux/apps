# The logic is linked against Qt::Core and dconf alone, and the helper
# that runs as root uses nothing else.
add_library(raye-regedit-logic STATIC
    settingsfile.cpp settingsfile.h
    edits.cpp edits.h
    locations.cpp locations.h
    dconfstore.cpp dconfstore.h
    regfile.cpp regfile.h
    helperargs.cpp helperargs.h)
target_include_directories(raye-regedit-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-regedit-logic PUBLIC Qt6::Core PkgConfig::DCONF)

add_executable(raye-regedit-helper helper.cpp)
target_link_libraries(raye-regedit-helper PRIVATE raye-regedit-logic)
# pkexec and the policy name /usr/libexec/raye-regedit-helper
install(TARGETS raye-regedit-helper DESTINATION libexec)
install(FILES org.raye.regedit.policy DESTINATION ${KDE_INSTALL_DATADIR}/polkit-1/actions)

# the window's model, apart so tests reach it without loading any QML
add_library(raye-regedit-model STATIC registry.cpp registry.h)
target_include_directories(raye-regedit-model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(raye-regedit-model PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_REGEDIT_HELPER="${CMAKE_INSTALL_PREFIX}/libexec/raye-regedit-helper")
target_link_libraries(raye-regedit-model PUBLIC raye-regedit-logic Qt6::Core Qt6::Qml KF6::I18n)

add_executable(raye-regedit)
qt_add_qml_module(raye-regedit
    URI org.raye.regedit
    VERSION 1.0
    SOURCES
        main.cpp
        registry.h
    QML_FILES
        Main.qml
        Glyph.qml
        MenuButton.qml
        ContextMenu.qml
        Sheet.qml
        TextBox.qml
        CheckRow.qml
        TreeRow.qml
        ValueRow.qml)
target_compile_definitions(raye-regedit PRIVATE TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}")
target_link_libraries(raye-regedit PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::I18n KF6::DBusAddons Raye::Version raye-regedit-model)

install(TARGETS raye-regedit ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.regedit.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.raye.regedit.mime.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
install(FILES org.raye.regedit.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
