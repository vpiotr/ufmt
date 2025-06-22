#!/bin/bash

# ufmt - ustr.h Integration Demo Runner
# This script demonstrates the ustr.h integration capability

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$PROJECT_ROOT/demos"

echo "=== ufmt ustr.h Integration Demo ==="
echo ""

# Build and run demo without ustr.h integration
echo "1. Running demo WITHOUT ustr.h integration (default mode):"
echo "   Command: g++ -std=c++11 -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp"
echo ""

cd "$PROJECT_ROOT"
g++ -std=c++11 -Wall -Wextra -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp

echo "--- Output without ustr.h integration ---"
./demos/demo_ustr_integration
echo ""

# Build and run demo with ustr.h integration
echo "2. Running demo WITH ustr.h integration (enhanced mode):"
echo "   Command: g++ -std=c++11 -DUFMT_USE_USTR -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp"
echo ""

g++ -std=c++11 -DUFMT_USE_USTR -Wall -Wextra -I include -o demos/demo_ustr_integration demos/demo_ustr_integration.cpp

echo "--- Output with ustr.h integration ---"
./demos/demo_ustr_integration
echo ""

echo "=== ustr.h Integration Demo Complete ==="
echo ""
echo "Summary:"
echo "- Without UFMT_USE_USTR: Standard ufmt functionality"
echo "- With UFMT_USE_USTR: Enhanced type support via ustr::to_string()"
echo "- Both modes are fully functional and demonstrate the optional nature of ustr.h integration"
