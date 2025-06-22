#!/bin/bash

# Build documentation using Doxygen
echo "Building documentation..."

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen not found. Please install doxygen to generate documentation."
    echo "On Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "On macOS: brew install doxygen graphviz"
    exit 1
fi

# Create docs directory if it doesn't exist
mkdir -p docs

# Clean previous documentation
if [ -d "docs/html" ]; then
    echo "Cleaning previous documentation..."
    rm -rf docs/html/*
fi

# Generate documentation
cd docs
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo "Documentation generated successfully in docs/html/"
    echo "Open docs/html/index.html in your browser to view the documentation."
else
    echo "Error: Documentation generation failed."
    exit 1
fi
