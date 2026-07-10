cmake_minimum_required(VERSION 3.10)
project(universal-sdk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/universal_sdk.cpp
)

# Platform-specific implementations
if(WIN32)
    list(APPEND SOURCES src/platform/win32_impl.cpp)
elseif(LINUX)
    list(APPEND SOURCES src/platform/linux_impl.cpp)
elseif(ANDROID)
    list(APPEND SOURCES src/platform/android_impl.cpp)
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

install(FILES include/universal_sdk.h
    DESTINATION include
)