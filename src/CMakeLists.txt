find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup()

qt_add_executable(appqanvas
    main.cpp
)

# Alias keeps the QML file at the module root in the resource system, so it is
# reachable as qrc:/Qanvas/Main.qml regardless of where it lives on disk.
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/Panel.qml PROPERTIES QT_RESOURCE_ALIAS Panel.qml)

qt_add_qml_module(appqanvas
    URI Qanvas
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        qml/Main.qml
        qml/Panel.qml
    SOURCES
        runtime/appcontext.h
        runtime/appcontext.cpp
        runtime/apploader.h
        runtime/apploader.cpp
)

# The QML type registration file Qt generates includes the headers by base name,
# so the directory they live in has to be on the include path.
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
        $<$<PLATFORM_ID:Android>:log>
)

if(QANVAS_AUTOMATION)
    target_link_libraries(appqanvas PRIVATE app_automation)

    # A dev build loads the checked-in app under test/ from the source tree, so
    # the files you edit are the files the automation tests drive. Tied to the
    # automation option because both are development-only.
    target_compile_definitions(appqanvas PRIVATE
        QANVAS_DEV_APPS_ROOT="${CMAKE_SOURCE_DIR}/test"
    )
endif()

install(TARGETS appqanvas
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
