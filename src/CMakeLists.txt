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
        runtime/appcontext.h
        runtime/appcontext.cpp
        runtime/apploader.h
        runtime/apploader.cpp
        runtime/chatagent.h
        runtime/chatagent.cpp
        runtime/app.h
        runtime/app.cpp
        runtime/appfiles.h
        runtime/appfiles.cpp
        runtime/appregistry.h
        runtime/appregistry.cpp
        runtime/apptools.h
        runtime/apptools.cpp
)

target_include_directories(appqanvas PRIVATE runtime)

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
