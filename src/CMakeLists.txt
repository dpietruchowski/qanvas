find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup()

qt_add_executable(appqanvas
    main.cpp
)

add_subdirectory(qml)

qt_add_qml_module(appqanvas
    URI Qanvas
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Qanvas
    SOURCES
        runtime/appcontext.h
        runtime/appcontext.cpp
        runtime/apploader.h
        runtime/apploader.cpp
        runtime/chatagent.h
        runtime/chatagent.cpp
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
        qanvas_qml
        themed_components_qml
        app_components_qml
        themed_icons
        app_utils
        app_agent
        $<$<PLATFORM_ID:Android>:log>
)

if(QANVAS_AUTOMATION)
    target_link_libraries(appqanvas PRIVATE app_automation)

    target_compile_definitions(appqanvas PRIVATE
        QANVAS_DEV_APPS_ROOT="${CMAKE_SOURCE_DIR}/test"
    )
endif()

install(TARGETS appqanvas
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
