#!/bin/bash

echo "========================================"
echo "Building Full BFS Test (Native)"
echo "========================================"
echo ""

# Compiler settings
CXX=g++
CXXFLAGS="-std=c++17 -O3 -Wall -I.."
OUTPUT="test_full_bfs_native"
SOURCE="test_full_bfs_native.cpp"

# Build
echo "Compiling ${SOURCE}..."
$CXX $CXXFLAGS $SOURCE -o $OUTPUT

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Files generated:"
    ls -lh $OUTPUT
    echo ""
    echo "Usage:"
    echo "  ./test_full_bfs_native [max_depth]"
    echo ""
    echo "Examples:"
    echo "  ./test_full_bfs_native 1    # Test depth 0-1"
    echo "  ./test_full_bfs_native 2    # Test depth 0-2"
    echo "  ./test_full_bfs_native 3    # Test depth 0-3"
    echo "  ./test_full_bfs_native 4    # Test depth 0-4"
    echo "  ./test_full_bfs_native 5    # Test depth 0-5 (default)"
    echo ""
else
    echo ""
    echo "❌ Build failed!"
    exit 1
fi
