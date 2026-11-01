cmake_minimum_required(VERSION 3.16)

project(qanvas VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

include(libs/cmake/AppProject.cmake)

app_load_env(${CMAKE_SOURCE_DIR}/app.env)

set(QANVAS_TESTS_DEFAULT OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT ANDROID)
    set(QANVAS_TESTS_DEFAULT ON)
endif()
option(BUILD_TESTING "Build the runtime unit tests" ${QANVAS_TESTS_DEFAULT})

app_project_setup(
    SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src
    FORMAT_DIRS src
)

app_option_debug_default(QANVAS_CLAUDE_CLI "Offer the local claude CLI as a provider")
app_option_debug_default(QANVAS_KEY_IMPORT "Import API keys from import_keys.ini on startup")

set(LIBS_CHAT ON)

add_subdirectory(libs)
add_subdirectory(src)

if(ANDROID)
    app_configure_android(${APP_TARGET}
        PACKAGE_NAME "${ANDROID_PACKAGE}"
        VERSION_CODE ${VERSION_CODE}
        VERSION_NAME "${VERSION_NAME}"
        KEYSTORE "${CMAKE_SOURCE_DIR}/${ANDROID_KEYSTORE}"
        KEY_ALIAS "${ANDROID_KEY_ALIAS}"
        PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android"
        OPENSSL_CMAKE "${CMAKE_SOURCE_DIR}/third_party/android_openssl/android_openssl.cmake"
    )
endif()
