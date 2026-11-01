find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql Test)

app_add_test(qanvas_tests
    OWN_MAIN
    DEFINITIONS
        QANVAS_QML_IMPORTS="${CMAKE_BINARY_DIR}"
    DEPS
        qanvas_runtime
        -Wl,--no-as-needed
        app_theme_qml
        app_themed_qml
        app_icons
        qanvas_components_qml
        -Wl,--as-needed
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
        Qt6::Test
)
