find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Sql)

add_executable(qanvas_tests
    main.cpp
    scriptedclient.h
    apptest.cpp
    appfilestest.cpp
    appregistrytest.cpp
    appcompilertest.cpp
    appwarningstest.cpp
    apptoolstest.cpp
    chatstoretest.cpp
    chatthreadtest.cpp
    notesstoretest.cpp
    notesbridgetest.cpp
    tasksstoretest.cpp
    tasksbridgetest.cpp
    ../runtime/app/app.h
    ../runtime/app/app.cpp
    ../runtime/app/appfiles.h
    ../runtime/app/appfiles.cpp
    ../runtime/app/appregistry.h
    ../runtime/app/appregistry.cpp
    ../runtime/app/appcompiler.h
    ../runtime/app/appcompiler.cpp
    ../runtime/app/appwarnings.h
    ../runtime/app/appwarnings.cpp
    ../runtime/storage/systemdatabase.h
    ../runtime/storage/systemdatabase.cpp
    ../runtime/storage/chatstore.h
    ../runtime/storage/chatstore.cpp
    ../runtime/storage/notesstore.h
    ../runtime/storage/notesstore.cpp
    ../runtime/storage/tasksstore.h
    ../runtime/storage/tasksstore.cpp
    ../runtime/host/storebridge.h
    ../runtime/host/storebridge.cpp
    ../runtime/host/notesbridge.h
    ../runtime/host/notesbridge.cpp
    ../runtime/host/tasksbridge.h
    ../runtime/host/tasksbridge.cpp
    ../runtime/chat/apptools.h
    ../runtime/chat/apptools.cpp
    ../runtime/chat/chatthread.h
    ../runtime/chat/chatthread.cpp
)

target_include_directories(qanvas_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(qanvas_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Sql
    app_agent
    gtest
)

add_test(NAME qanvas_tests COMMAND qanvas_tests)
