find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_standard_project_setup()

add_subdirectory(runtime)
add_subdirectory(components)
add_subdirectory(qml)

qt_add_executable(${APP_TARGET}
    main.cpp
)

if(LIBS_AUTOMATION)
    target_link_libraries(${APP_TARGET} PRIVATE app_automation)
endif()

target_compile_definitions(${APP_TARGET} PRIVATE
    APP_NAME="${APP_NAME}"
    APP_QML_URI="${APP_QML_URI}"
    APP_VERSION="${VERSION_NAME}"
    APP_VERSION_CODE="${VERSION_CODE}"
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        qanvas_runtime
        app_platform
        app_qmlutils
        -Wl,--no-as-needed
        app_theme_qml
        app_themed_qml
        app_components_qml
        app_chat_qml
        app_icons
        qanvas_components_qml
        ${APP_QML_TARGET}
        -Wl,--as-needed
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
        $<$<PLATFORM_ID:Android>:log>
)

if(BUILD_TESTING AND NOT ANDROID)
    add_subdirectory(tests)
endif()

install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
