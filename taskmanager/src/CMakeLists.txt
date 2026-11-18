# The logic is a library of its own, linked against Qt::Core alone, so
# the tests reach it without a display, without a session bus and
# without the translation libraries. Nothing in it says a word to the
# user: the words are in the QML, in this package's catalogue.
add_library(raye-taskmanager-logic STATIC
    proctable.cpp proctable.h
    efficiency.cpp efficiency.h
    endtask.cpp endtask.h
    sysinfo.cpp sysinfo.h
    appgroups.cpp appgroups.h
    autostart.cpp autostart.h
    services.cpp services.h
    devicelist.cpp devicelist.h
    runtask.cpp runtask.h
    rowdiff.h)
target_include_directories(raye-taskmanager-logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(raye-taskmanager-logic PUBLIC Qt6::Core)

add_executable(raye-taskmanager)
set_source_files_properties(Fmt.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# SOURCES, not target_sources() afterwards: the type registrar of the
# module only sees what the module was given, and ProcessModel is a
# QML_ELEMENT the QML needs by name.
qt_add_qml_module(raye-taskmanager
    URI org.raye.taskmanager
    VERSION 1.0
    SOURCES
        main.cpp
        processmodel.cpp processmodel.h
        processgroups.cpp processgroups.h
        performancemodel.cpp performancemodel.h
        startupmodel.cpp startupmodel.h
        servicesmodel.cpp servicesmodel.h
    QML_FILES
        Main.qml
        Fmt.qml
        Glyph.qml
        NavButton.qml
        CommandButton.qml
        HeaderCell.qml
        SearchField.qml
        Graph.qml
        ContextMenu.qml
        ConfirmSheet.qml
        ChoiceButton.qml
        CheckRow.qml
        WheelScroll.qml
        RunTaskSheet.qml
        ProcessesPage.qml
        GroupRow.qml
        PerformancePage.qml
        StartupPage.qml
        UsersPage.qml
        DetailsPage.qml
        ServicesPage.qml
        SettingsPage.qml)
target_compile_definitions(raye-taskmanager PRIVATE
    TRANSLATION_DOMAIN="${RAYE_APP_DOMAIN}")
target_link_libraries(raye-taskmanager PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus Qt6::Network
    KF6::I18n KF6::DBusAddons Raye::Version raye-taskmanager-logic)

install(TARGETS raye-taskmanager ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.raye.taskmanager.desktop DESTINATION ${KDE_INSTALL_APPDIR})
