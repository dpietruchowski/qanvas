set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(qanvas_components_qml STATIC)

app_add_qml_module(qanvas_components_qml
    URI Qanvas.Components
)
