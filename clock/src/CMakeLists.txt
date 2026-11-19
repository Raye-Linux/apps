# The logic is a library linked against Qt::Core alone, so the tests reach
# it without a display or a session bus. It says no word to the user.
add_library(raye-clock-logic STATIC
    alarms.cpp alarms.h
    countdown.cpp countdown.h
    rings.cpp rings.h
    clockstore.cpp clockstore.h
    worldtime.cpp worldtime.h
    rowmodel.cpp rowmodel.h)
target_include_directories(raye-clock-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-clock-logic PUBLIC Qt6::Core)

set(RAYE_CLOCK_TOOL_PATH "${KDE_INSTALL_FULL_LIBEXECDIR}/raye-clock-tool")

# What rings: no window, so no Qt Gui of its own. Started by the user timer.
add_executable(raye-clock-tool tool.cpp)
target_compile_definitions(raye-clock-tool PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_CLOCK_APP="${KDE_INSTALL_FULL_BINDIR}/raye-clock")
target_link_libraries(raye-clock-tool PRIVATE Qt6::Core Qt6::DBus Qt6::Multimedia KF6::I18n raye-clock-logic)
install(TARGETS raye-clock-tool DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(raye-clock-ring.service.in ${CMAKE_CURRENT_BINARY_DIR}/raye-clock-ring.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/raye-clock-ring.service DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

add_executable(raye-clock)
qt_add_qml_module(raye-clock
    URI org.raye.clock
    VERSION 1.0
    SOURCES
        main.cpp
        clockmodel.cpp clockmodel.h
        cities.cpp cities.h
    QML_FILES
        qml/Main.qml
        qml/Glyph.qml
        qml/NavButton.qml
        qml/FlatButton.qml
        qml/RoundButton.qml
        qml/WheelScroll.qml
        qml/Scroller.qml
        qml/ProgressRing.qml
        qml/TimersPage.qml
        qml/TimerCard.qml
        qml/TimerSheet.qml
        qml/AlarmsPage.qml
        qml/AlarmCard.qml
        qml/AlarmSheet.qml
        qml/SoundPreview.qml
        qml/DayChips.qml
        qml/DigitWheel.qml
        qml/Sheet.qml
        qml/TextBox.qml
        qml/ChoiceBox.qml
        qml/MenuPopup.qml
        qml/StopwatchPage.qml
        qml/WorldPage.qml
        qml/CityCard.qml
        qml/CitySheet.qml)
target_compile_definitions(raye-clock PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_CLOCK_TOOL="${RAYE_CLOCK_TOOL_PATH}")
target_link_libraries(raye-clock PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::I18n KF6::I18nLocaleData KF6::DBusAddons Raye::Version raye-clock-logic)

install(TARGETS raye-clock ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.clock.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.raye.clock.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
