#!/bin/bash
# Production WASM Build Script for xxcross_trainer.html
# 2026-01-04: Optimized build with minimal API surface
#
# Usage: ./build_production.sh [debug]
#   debug: Build with assertions for error diagnosis (-O2 -sASSERTIONS=1)
# Output: solver_prod.js, solver_prod.wasm

set -e  # Exit on error

echo "=== XXcross Solver - Production Build ==="
echo "Date: $(date)"
echo ""

# Activate Emscripten SDK
echo "[1/4] Activating Emscripten SDK..."
source ~/emsdk/emsdk_env.sh

# Verify em++ is available
if ! command -v em++ &> /dev/null; then
    echo "ERROR: em++ not found. Please install Emscripten SDK."
    exit 1
fi

echo "[2/4] Compiler: $(em++ --version | head -n 1)"
echo ""

# Build configuration
INPUT="solver_prod_stable_20260108.cpp"
OUTPUT="solver_prod.js"

# Debug mode adds assertions for error diagnosis
if [ "$1" = "debug" ]; then
    echo "*** DEBUG MODE: Building with ASSERTIONS ***"
    OPTIMIZATION="-O2 -sASSERTIONS=1"
else
    OPTIMIZATION="-O3 -msimd128 -flto"
fi
STANDARD="-std=c++17"
INCLUDE_PATH="-I."
WASM_FLAGS="-s WASM=1"
MEMORY_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=4294967296 -s INITIAL_MEMORY=67108864"
EXPORT_FLAGS="-s EXPORTED_RUNTIME_METHODS=[cwrap] -s MODULARIZE=1 -s EXPORT_NAME=createModule -s INVOKE_RUN=0"
BINDING_FLAG="--bind"

# Note: production/tsl/ is used (not src/tsl/)
echo "[3/4] Building WASM module..."
echo "  Input:  $INPUT"
echo "  Output: $OUTPUT"
echo "  Robin hash: production/tsl/"
echo "  Optimization: $OPTIMIZATION"
echo ""

em++ $INPUT \
  -o $OUTPUT \
  $STANDARD \
  $OPTIMIZATION \
  $INCLUDE_PATH \
  $WASM_FLAGS \
  $MEMORY_FLAGS \
  $EXPORT_FLAGS \
  $BINDING_FLAG

# Check build success
if [ -f "$OUTPUT" ] && [ -f "solver_prod.wasm" ]; then
    echo ""
    echo "[4/4] Build successful!"
    echo ""
    echo "Output files:"
    ls -lh solver_prod.js solver_prod.wasm
    echo ""
    echo "=== Production module ready for deployment ==="
    echo "API: xxcross_search(bool adj, string bucket_model)"
    echo "Models: MOBILE_LOW, MOBILE_MIDDLE, MOBILE_HIGH, DESKTOP_STD, DESKTOP_HIGH, DESKTOP_ULTRA"
else
    echo ""
    echo "ERROR: Build failed - output files not found"
    exit 1
fi
