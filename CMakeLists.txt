cmake_minimum_required(VERSION 3.10)
project(universal-sdk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/universal_sdk.cpp
    src/legal_assistant.cpp
    src/stub_providers.cpp
    src/file_audit_writer.cpp
)

# Platform-specific implementations — exactly one file is compiled.
# Android must be checked before UNIX since Android is also UNIX.
if(WIN32)
    list(APPEND SOURCES src/platform/win32_impl.cpp)
elseif(ANDROID)
    list(APPEND SOURCES src/platform/android_impl.cpp)
elseif(UNIX)
    list(APPEND SOURCES src/platform/linux_impl.cpp)
else()
    list(APPEND SOURCES src/platform/stub_platform_impl.cpp)
endif()

# Create the library
add_library(universal_sdk SHARED ${SOURCES})

# Set output directory
set_target_properties(universal_sdk PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Include directories
target_include_directories(universal_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Installation rules
install(TARGETS universal_sdk
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    include/universal_sdk.h
    include/legal_assistant.h
    include/stub_providers.h
    include/file_audit_writer.h
    DESTINATION include
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

find_package(GTest QUIET)
if(GTest_FOUND)
    # Helper macro to reduce duplication
    macro(add_sdk_test target_name)
        target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        target_link_libraries(${target_name}
            universal_sdk
            GTest::GTest
            GTest::Main
        )
        include(GoogleTest)
        gtest_discover_tests(${target_name})
    endmacro()

    # legal_assistant tests — link against the library
    add_executable(legal_assistant_test
        tests/legal_assistant_test.cpp
    )
    add_sdk_test(legal_assistant_test)

    # universal_sdk core tests — link against the library
    add_executable(universal_sdk_test
        tests/universal_sdk_test.cpp
    )
    add_sdk_test(universal_sdk_test)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
add_executable(legal_assistant_sample
    examples/legal_assistant_sample.cpp
)
target_include_directories(legal_assistant_sample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(legal_assistant_sample
    universal_sdk
)
