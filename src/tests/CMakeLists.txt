find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Sql)

app_add_test(qanvas_tests
    OWN_MAIN
    DEPS
        qanvas_runtime
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Sql
)
