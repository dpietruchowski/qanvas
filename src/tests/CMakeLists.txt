find_package(Qt6 REQUIRED COMPONENTS Core Qml Sql)

add_executable(qanvas_tests
    main.cpp
    apptest.cpp
    appfilestest.cpp
    appregistrytest.cpp
    ../runtime/app.cpp
    ../runtime/appfiles.cpp
    ../runtime/appregistry.cpp
)

target_include_directories(qanvas_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(qanvas_tests PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Sql
    gtest
)

add_test(NAME qanvas_tests COMMAND qanvas_tests)
