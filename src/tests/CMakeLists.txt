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
    ../runtime/app.cpp
    ../runtime/appfiles.cpp
    ../runtime/systemdatabase.cpp
    ../runtime/appregistry.cpp
    ../runtime/appcompiler.cpp
    ../runtime/appwarnings.h
    ../runtime/appwarnings.cpp
    ../runtime/apptools.h
    ../runtime/apptools.cpp
    ../runtime/chatstore.cpp
    ../runtime/chatthread.h
    ../runtime/chatthread.cpp
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
