# Tests of the share picker, and a harness to look at it (not installed).

set(picker_test_defines
    RAYE_THEME_QML_DIR="${PROJECT_SOURCE_DIR}/../theme/qml"
    RAYE_UI_STUB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/stub"
    RAYE_PICKER_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    RAYE_PICKER_MO="${CMAKE_BINARY_DIR}/portal/po/po/fr/${RAYE_APP_DOMAIN}.mo")

add_library(raye-portal-picker-testlib STATIC
    fakesources.cpp fakesources.h
    pickerloader.cpp pickerloader.h)
target_compile_definitions(raye-portal-picker-testlib PUBLIC ${picker_test_defines})
target_link_libraries(raye-portal-picker-testlib PUBLIC
    Qt::Core Qt::Gui Qt::Qml Qt::Quick KF6::I18n raye-portal-pickerplugin)

raye_add_qtest(sharepicker SOURCES test_sharepicker.cpp LIBRARIES raye-portal-picker-testlib)
if(TARGET ${RAYE_APP_DOMAIN}-translations)
    add_dependencies(${RAYE_APP_PACKAGE}-test-sharepicker ${RAYE_APP_DOMAIN}-translations)
endif()
set_tests_properties(${RAYE_APP_PACKAGE}/sharepicker PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;LC_ALL=C.UTF-8")

add_executable(raye-portal-picker-harness picker-harness.cpp)
target_link_libraries(raye-portal-picker-harness PRIVATE raye-portal-picker-testlib)
