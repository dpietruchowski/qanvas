find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql Test)

app_add_test(qanvas_tests
    OWN_MAIN
    DEPS
        qanvas_runtime
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Sql
        Qt6::Test
)
