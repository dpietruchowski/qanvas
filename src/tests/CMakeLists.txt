find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(qanvas_tests
    appwritertest.cpp
    ../runtime/appwriter.cpp
)

target_include_directories(qanvas_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(qanvas_tests PRIVATE
    Qt6::Core
    gtest
    gtest_main
)

add_test(NAME qanvas_tests COMMAND qanvas_tests)
