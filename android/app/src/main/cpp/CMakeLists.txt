cmake_minimum_required(VERSION 3.10)
project(legal_assist_jni)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the repository root (five directories above this CMakeLists.txt:
# android/app/src/main/cpp -> android/app/src/main -> android/app/src
#   -> android/app -> android -> <repo root>)
set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

include_directories(${SDK_ROOT}/include)

add_library(legal_assist_jni SHARED
    ${SDK_ROOT}/src/universal_sdk.cpp
    ${SDK_ROOT}/src/legal_assistant.cpp
    ${SDK_ROOT}/src/stub_providers.cpp
    ${SDK_ROOT}/src/file_audit_writer.cpp
    ${SDK_ROOT}/src/platform/android_impl.cpp
    jni_bridge.cpp
)

target_compile_definitions(legal_assist_jni PRIVATE
    UNIVERSAL_SDK_VERSION="2.0.0"
)

target_link_libraries(legal_assist_jni
    android
    log
)
