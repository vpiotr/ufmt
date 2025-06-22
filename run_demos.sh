#!/bin/bash

echo "=== ufmt Demos Runner ==="

# Build if needed
if [ ! -d "build" ]; then
    echo "Build directory not found, running complete build..."
    ./rebuild.sh
fi

# Navigate to build directory
cd build

# Function to run demo if it exists
run_demo() {
    local demo_name=$1
    local demo_description=$2
    
    if [ -f "$demo_name" ]; then
        echo ""
        echo "=== $demo_description ==="
        ./$demo_name
        echo "--- End of $demo_description ---"
    else
        echo "Building $demo_name..."
        make $demo_name 2>/dev/null
        if [ -f "$demo_name" ]; then
            echo ""
            echo "=== $demo_description ==="
            ./$demo_name
            echo "--- End of $demo_description ---"
        else
            echo "Warning: $demo_name not available (may not be configured in CMake)"
        fi
    fi
}

# Auto-detect and run all demo_* executables
echo "Scanning for demo executables..."

found_demos=false
for demo_executable in demo_*; do
    if [ -f "$demo_executable" ] && [ -x "$demo_executable" ]; then
        found_demos=true
        demo_name="$demo_executable"
        
        # Generate a friendly description from the executable name
        description=$(echo "$demo_name" | sed 's/demo_//' | sed 's/_/ /g' | sed 's/\b\w/\U&/g')
        description="$description Demo"
        
        echo ""
        echo "=== $description ==="
        ./"$demo_name"
        echo "--- End of $description ---"
    fi
done

if [ "$found_demos" = false ]; then
    echo "No demo executables found. Trying to build them..."
    # Try to build known demos
    for demo_src in demo_basic demo_multithreading demo_transparent_api demo_ustr_integration; do
        if make "$demo_src" 2>/dev/null && [ -f "$demo_src" ]; then
            description=$(echo "$demo_src" | sed 's/demo_//' | sed 's/_/ /g' | sed 's/\b\w/\U&/g')
            description="$description Demo"
            
            echo ""
            echo "=== $description ==="
            ./"$demo_src"
            echo "--- End of $description ---"
        fi
    done
fi

echo ""
echo "=== All Demos Complete ==="
