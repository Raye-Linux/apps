# The logic is a library linked against Qt::Core alone, so the tests reach
# it without a display or a session bus. It says no word to the user.
add_library(raye-calendar-logic STATIC
    ical.cpp ical.h
    recurrence.cpp recurrence.h
    reminders.cpp reminders.h
    holidays.cpp holidays.h
    calstore.cpp calstore.h)
target_include_directories(raye-calendar-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-calendar-logic PUBLIC Qt6::Core)

# The words of the holidays, shared by the window and the tool
add_library(raye-calendar-names STATIC holidaynames.cpp holidaynames.h)
target_compile_definitions(raye-calendar-names PRIVATE TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}")
target_link_libraries(raye-calendar-names PUBLIC Qt6::Core KF6::I18n)

set(RAYE_CALENDAR_TOOL_PATH "${KDE_INSTALL_FULL_LIBEXECDIR}/raye-calendar-tool")

# Reminders, the agenda of the clock's calendar, and the timer: no window,
# so no Qt Gui. It is what runs at the time of a reminder.
add_executable(raye-calendar-tool tool.cpp)
target_compile_definitions(raye-calendar-tool PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_CALENDAR_APP="${KDE_INSTALL_FULL_BINDIR}/raye-calendar")
target_link_libraries(raye-calendar-tool PRIVATE Qt6::Core Qt6::DBus KF6::I18n raye-calendar-logic raye-calendar-names)
install(TARGETS raye-calendar-tool DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(raye-calendar-reminder.service.in ${CMAKE_CURRENT_BINARY_DIR}/raye-calendar-reminder.service @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/raye-calendar-reminder.service DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

add_executable(raye-calendar)
qt_add_qml_module(raye-calendar
    URI org.raye.calendar
    VERSION 1.0
    SOURCES
        main.cpp
        calendarmodel.cpp calendarmodel.h
    QML_FILES
        qml/Main.qml
        qml/datelogic.js
        qml/Glyph.qml
        qml/FlatButton.qml
        qml/ViewSwitch.qml
        qml/MiniMonth.qml
        qml/MonthView.qml
        qml/TimeGrid.qml
        qml/AgendaView.qml
        qml/EventChip.qml
        qml/EventSheet.qml
        qml/ChoiceBox.qml
        qml/DateField.qml
        qml/TimeField.qml
        qml/MenuPopup.qml)
target_compile_definitions(raye-calendar PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}"
    RAYE_CALENDAR_TOOL="${RAYE_CALENDAR_TOOL_PATH}")
target_link_libraries(raye-calendar PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::I18n KF6::DBusAddons Raye::Version raye-calendar-logic raye-calendar-names)

install(TARGETS raye-calendar ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.calendar.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.raye.calendar.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
