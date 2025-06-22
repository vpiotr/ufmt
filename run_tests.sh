#!/bin/bash

# ufmt - Run tests script

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Running ufmt Tests ==="

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Running rebuild first..."
    "$PROJECT_ROOT/rebuild.sh"
fi

# Run tests
cd "$BUILD_DIR"
echo "Running test suite..."
make run_tests
