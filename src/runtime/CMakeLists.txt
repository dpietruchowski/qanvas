find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sql)

app_add_module(qanvas_runtime
    DEPS
        app_agent
        app_async
        app_chat
        app_platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Sql
)

qt_add_resources(qanvas_runtime "systemprompt"
    PREFIX "/qanvas"
    BASE chat
    FILES chat/systemprompt.txt chat/chatprompt.txt
)
