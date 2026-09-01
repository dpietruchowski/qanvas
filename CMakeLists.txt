cmake_minimum_required(VERSION 3.16)

project(qanvas VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android packaging — consumed by the if(ANDROID) block at the bottom and by
# build.sh / sign.sh, which grep the version out of this file.
set(ANDROID_PACKAGE_NAME "com.qanvas.app")
set(ANDROID_KEYSTORE_PATH "${CMAKE_SOURCE_DIR}/android/qanvas.keystore")
set(ANDROID_KEY_ALIAS "qanvas_key")
set(QT_ANDROID_VERSION_CODE 1)
set(QT_ANDROID_VERSION_NAME "0.1.0")
set(QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android")

set(FORMAT_DIRS "src")

find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
    string(REPLACE ";" " " DIRS_STRING "${FORMAT_DIRS}")

    set(FORMAT_FIND_EXPR
        "${DIRS_STRING} -type d -name third_party -prune -o \
-type f '(' -name '*.cpp' -o -name '*.h' -o -name '*.hpp' ')'"
    )

    add_custom_target(format
        COMMAND sh -c "find ${FORMAT_FIND_EXPR} -exec '${CLANG_FORMAT}' -style=file -i {} +"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting files in directories: ${DIRS_STRING}"
        VERBATIM
    )

    add_custom_target(format_check
        COMMAND sh -c "find ${FORMAT_FIND_EXPR} -exec '${CLANG_FORMAT}' -style=file --dry-run -Werror {} +"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking format in directories: ${DIRS_STRING}"
        VERBATIM
    )

    message(STATUS "Clang-format found: ${CLANG_FORMAT} for dirs: ${DIRS_STRING}")
else()
    message(WARNING "Clang-format not found")
endif()

add_compile_definitions(
    $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG_OUTPUT>
    $<$<NOT:$<CONFIG:Debug>>:QT_NO_INFO_OUTPUT>
)

# Shared component libraries (git submodule, github.com/dpietruchowski/app-libs).
# Checked out but not built yet — uncomment once the app actually links against
# dbtoolkit / eventbus / themed components.
# add_subdirectory(libs)

add_subdirectory(src)

if(ANDROID)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()

    set_target_properties(appqanvas PROPERTIES
        QT_ANDROID_PACKAGE_NAME   "${ANDROID_PACKAGE_NAME}"
        QT_ANDROID_VERSION_CODE   "${QT_ANDROID_VERSION_CODE}"
        QT_ANDROID_VERSION_NAME   "${QT_ANDROID_VERSION_NAME}"
        QT_ANDROID_TARGET_SDK_VERSION 36
        QT_ANDROID_PACKAGE_SOURCE_DIR "${QT_ANDROID_PACKAGE_SOURCE_DIR}"
        QT_ANDROID_KEYSTORE_PATH "${ANDROID_KEYSTORE_PATH}"
        QT_ANDROID_KEY_ALIAS "${ANDROID_KEY_ALIAS}"
    )

    # Prebuilt OpenSSL for Android (git submodule, github.com/KDAB/android_openssl).
    # Uncomment once the app talks to the network over TLS.
    # include(${CMAKE_SOURCE_DIR}/third_party/android_openssl/android_openssl.cmake)
    # set_target_properties(OpenSSL::Crypto OpenSSL::SSL PROPERTIES IMPORTED_GLOBAL TRUE)
    # add_android_openssl_libraries(appqanvas)

    qt_android_generate_deployment_settings(appqanvas)
    qt_android_add_apk_target(appqanvas)

    get_target_property(QT_ANDROID_DIR appqanvas QT_ANDROID_PACKAGE_SOURCE_DIR)
    message(STATUS "Target property QT_ANDROID_PACKAGE_SOURCE_DIR = ${QT_ANDROID_DIR}")
endif()
