set(LIBRARY_NAME qanvas_qml)

file(GLOB_RECURSE QML_FILES_FULL "${CMAKE_CURRENT_SOURCE_DIR}/*.qml")

set(QML_FILES "")
foreach(qml_file ${QML_FILES_FULL})
    file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${qml_file})
    list(APPEND QML_FILES ${rel_path})
endforeach()

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(${LIBRARY_NAME} STATIC)

qt_add_qml_module(${LIBRARY_NAME}
    URI Qanvas.Components
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES ${QML_FILES}
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Qanvas/Components
    NO_PLUGIN
)

target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        themed_components_qml
        app_components_qml
)

install(TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/Qanvas
    DESTINATION lib/qt6/qml
    FILES_MATCHING PATTERN "*.qml" PATTERN "*.qmltypes" PATTERN "qmldir"
)
