find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup()

qt_add_executable(appqanvas
    main.cpp
)

add_subdirectory(components)

file(GLOB_RECURSE SHELL_QML_FULL "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml")

set(SHELL_QML "")
foreach(qml_file ${SHELL_QML_FULL})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/qml" ${qml_file})
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${rel_path})
    list(APPEND SHELL_QML ${qml_file})
endforeach()

qt_add_qml_module(appqanvas
    URI Qanvas
    VERSION 1.0
    RESOURCE_PREFIX "/"
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Qanvas
    QML_FILES ${SHELL_QML}
    SOURCES
        runtime/app/app.h
        runtime/app/app.cpp
        runtime/app/appfiles.h
        runtime/app/appfiles.cpp
        runtime/app/appregistry.h
        runtime/app/appregistry.cpp
        runtime/app/appcompiler.h
        runtime/app/appcompiler.cpp
        runtime/app/appwarnings.h
        runtime/app/appwarnings.cpp
        runtime/storage/systemdatabase.h
        runtime/storage/systemdatabase.cpp
        runtime/storage/chatstore.h
        runtime/storage/chatstore.cpp
        runtime/storage/notesstore.h
        runtime/storage/notesstore.cpp
        runtime/host/appcontext.h
        runtime/host/appcontext.cpp
        runtime/host/apploader.h
        runtime/host/apploader.cpp
        runtime/host/notesbridge.h
        runtime/host/notesbridge.cpp
        runtime/chat/apptools.h
        runtime/chat/apptools.cpp
        runtime/chat/chatthread.h
        runtime/chat/chatthread.cpp
        runtime/chat/chatagent.h
        runtime/chat/chatagent.cpp
)

target_include_directories(appqanvas PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/app
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/host
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/chat
)

target_compile_definitions(appqanvas PRIVATE
    APP_VERSION="${QT_ANDROID_VERSION_NAME}"
    APP_VERSION_CODE="${QT_ANDROID_VERSION_CODE}"
)

target_link_libraries(appqanvas
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
        qanvas_components_qml
        themed_components_qml
        app_components_qml
        themed_icons
        app_utils
        app_agent
        $<$<PLATFORM_ID:Android>:log>
)

if(QANVAS_AUTOMATION)
    target_link_libraries(appqanvas PRIVATE app_automation)
endif()

if(QANVAS_TESTS)
    add_subdirectory(tests)
endif()

install(TARGETS appqanvas
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/Qanvas
    DESTINATION lib/qt6/qml
    FILES_MATCHING PATTERN "*.qml" PATTERN "*.qmltypes" PATTERN "qmldir"
)
