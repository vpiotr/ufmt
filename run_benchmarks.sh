#!/bin/bash

echo "=== ufmt Benchmarks Runner ==="

# Build if needed
if [ ! -d "build" ]; then
    echo "Build directory not found, running complete build..."
    ./rebuild.sh
fi

# Navigate to build directory
cd build

# Function to run benchmark if it exists
run_benchmark() {
    local benchmark_name=$1
    local benchmark_description=$2
    
    if [ -f "$benchmark_name" ]; then
        echo ""
        echo "=== $benchmark_description ==="
        ./$benchmark_name
        echo "--- End of $benchmark_description ---"
    else
        echo "Building $benchmark_name..."
        make $benchmark_name
        if [ -f "$benchmark_name" ]; then
            echo ""
            echo "=== $benchmark_description ==="
            ./$benchmark_name
            echo "--- End of $benchmark_description ---"
        else
            echo "Error: Failed to build $benchmark_name"
        fi
    fi
}

# Run all benchmarks
run_benchmark "benchmark_basic" "Basic Performance Benchmark"
run_benchmark "benchmark_multithreading" "Multi-threading Performance Benchmark"

echo ""
echo "=== All Benchmarks Complete ==="
