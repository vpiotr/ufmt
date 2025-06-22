#!/bin/bash

# ufmt - Complete rebuild script
# Cleans build directory and rebuilds everything

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== ufmt Complete Rebuild ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Clean build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
echo "Building..."
make -j$(nproc)

echo "=== Build Complete ==="
echo "To run tests: cd $BUILD_DIR && make run_tests"
echo "To run demo:  cd $BUILD_DIR && make run_demo"
