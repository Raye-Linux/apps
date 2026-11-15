# Tests of the screen-sharing backend. AP-18's checklist lives in matrix/.

raye_add_qtest(appidentity SOURCES test_appidentity.cpp LIBRARIES raye-portal-logic)
target_compile_definitions(${RAYE_APP_PACKAGE}-test-appidentity PRIVATE
    FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

raye_add_qtest(pickerchoice SOURCES test_pickerchoice.cpp LIBRARIES raye-portal-logic)

raye_add_qtest(sharesessions SOURCES test_sharesessions.cpp LIBRARIES raye-portal-logic)
# with a session bus of its own, the D-Bus part runs too
find_program(RAYE_DBUS_RUN_SESSION dbus-run-session)
if(RAYE_DBUS_RUN_SESSION)
    add_test(NAME ${RAYE_APP_PACKAGE}/sharesessions-bus
        COMMAND ${RAYE_DBUS_RUN_SESSION} -- $<TARGET_FILE:${RAYE_APP_PACKAGE}-test-sharesessions> onTheBus)
    set_tests_properties(${RAYE_APP_PACKAGE}/sharesessions-bus PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=C.UTF-8")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME ${RAYE_APP_PACKAGE}/files
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/test_portal_files.py
            ${PROJECT_SOURCE_DIR}/portal ${CMAKE_BINARY_DIR}/portal/data
            ${KDE_INSTALL_FULL_LIBEXECDIR} ${RAYE_PORTAL_ACCESS}
            /usr/share/xdg-desktop-portal/kde-portals.conf)

add_test(NAME ${RAYE_APP_PACKAGE}/sync-upstream
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/test_sync_upstream.sh
            ${PROJECT_SOURCE_DIR}/portal/sync-upstream.sh ${CMAKE_CURRENT_BINARY_DIR}/sync-work)

add_test(NAME ${RAYE_APP_PACKAGE}/restore-off-by-default
    COMMAND Python3::Interpreter -I -B ${CMAKE_CURRENT_SOURCE_DIR}/test_restore_off_by_default.py)
