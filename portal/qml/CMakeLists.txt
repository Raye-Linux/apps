# The Raye share picker: the window shown when an app asks to share the
# screen. A static QML module linked into the backend; portal/README.md
# has what it is given and what the backend reads back.
#
# At run time it imports org.raye.ui (raye-style), Kirigami, KItemModels
# and, for the live pictures, KPipeWire and the task manager module.

qt_add_library(raye-portal-picker STATIC)
# imported by path from the QML files, not a type of the module
set_source_files_properties(PickerLogic.js PROPERTIES QT_QML_SKIP_QMLDIR_ENTRY TRUE)
qt_add_qml_module(raye-portal-picker
    URI org.raye.portal.picker
    VERSION 1.0
    QML_FILES
        SharePicker.qml
        SegmentedTabs.qml
        SourceCard.qml
        CheckOption.qml
        LiveThumbnail.qml
        PickerLogic.js)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
