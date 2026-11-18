# The logic is linked against Qt::Core alone, so the tests reach it with
# no display, and the helper that runs as root uses nothing else. It says
# no word to the user.
add_library(raye-graphics-logic STATIC
    cards.cpp cards.h
    amdgpu.cpp amdgpu.h
    tuning.cpp tuning.h
    fancurve.cpp fancurve.h
    nvidiasmi.cpp nvidiasmi.h)
target_include_directories(raye-graphics-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-graphics-logic PUBLIC Qt6::Core)

set(RAYE_GRAPHICS_HELPER_PATH "${CMAKE_INSTALL_PREFIX}/libexec/raye-graphics-helper")

# The one part that runs as root, through pkexec and the two services.
add_executable(raye-graphics-helper helper.cpp)
target_link_libraries(raye-graphics-helper PRIVATE raye-graphics-logic)
# pkexec, the policy and the services name /usr/libexec/raye-graphics-helper
install(TARGETS raye-graphics-helper DESTINATION libexec)
install(FILES org.raye.graphics.policy DESTINATION ${KDE_INSTALL_DATADIR}/polkit-1/actions)
install(FILES raye-graphics.service raye-graphics-fan.service DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system)

add_executable(raye-graphics)
set_source_files_properties(qml/Fmt.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(raye-graphics
    URI org.raye.graphics
    VERSION 1.0
    SOURCES
        main.cpp
        graphicsmodel.cpp graphicsmodel.h
    QML_FILES
        qml/Main.qml
        qml/Fmt.qml
        qml/Glyph.qml
        qml/NavButton.qml
        qml/ChoiceButton.qml
        qml/ContextMenu.qml
        qml/WheelScroll.qml
        qml/Graph.qml
        qml/Tile.qml
        qml/LimitSlider.qml
        qml/CurveEditor.qml
        qml/ActionBar.qml
        qml/OverviewPage.qml
        qml/TuningPage.qml
        qml/FanPage.qml)
target_compile_definitions(raye-graphics PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_GRAPHICS_HELPER="${RAYE_GRAPHICS_HELPER_PATH}")
target_link_libraries(raye-graphics PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::I18n KF6::DBusAddons Raye::Version raye-graphics-logic)

install(TARGETS raye-graphics ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.graphics.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.raye.graphics.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
