#!/bin/bash

# Universal SDK Build Script
# Supports building on Windows, Linux, and Android platforms

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
PLATFORM=""

# Functions
print_usage() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -d, --debug             Build in Debug mode (default: Release)"
    echo "  -b, --build-dir DIR     Set build directory (default: build)"
    echo "  -p, --prefix PREFIX     Set install prefix (default: /usr/local)"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -i, --install           Install after building"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -c|--clean)
            rm -rf "$BUILD_DIR"
            echo -e "${GREEN}Cleaned build directory${NC}"
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo -e "${GREEN}Created build directory: $BUILD_DIR${NC}"
fi

# Change to build directory
cd "$BUILD_DIR"

# Run CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

# Build
echo -e "${YELLOW}Building universal-sdk...${NC}"
cmake --build . --config "$BUILD_TYPE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build completed successfully!${NC}"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    echo -e "${YELLOW}Installing universal-sdk...${NC}"
    cmake --install . --config "$BUILD_TYPE"
    echo -e "${GREEN}Installation completed!${NC}"
fi

echo -e "${GREEN}Build artifacts are in: $BUILD_DIR${NC}"