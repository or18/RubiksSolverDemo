#!/bin/bash
# Production WASM Build Script for f2leo_trainer
# Usage: ./build_production.sh [debug]
#   debug: Build with assertions for error diagnosis (-O2 -sASSERTIONS=1)
# Output: f2leo_solver.js, f2leo_solver.wasm

set -e  # Exit on error

echo "=== F2L-EO Solver - Production Build ==="
echo "Date: $(date)"
echo ""

# Activate Emscripten SDK
echo "[1/4] Activating Emscripten SDK..."
if [ -f "/workspaces/emsdk/emsdk_env.sh" ]; then
    source /workspaces/emsdk/emsdk_env.sh
elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh"
else
    echo "WARNING: emsdk_env.sh not found at default paths. Assuming em++ is already in PATH."
fi

# Verify em++ is available
if ! command -v em++ &> /dev/null; then
    echo "ERROR: em++ not found. Please verify Emscripten SDK installation."
    exit 1
fi

echo "[2/4] Compiler: $(em++ --version | head -n 1)"
echo ""

# Build configuration
INPUT="f2leo_solver_prod.cpp"
OUTPUT="f2leo_solver.js"
WASM_OUTPUT="f2leo_solver.wasm"

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
# 512MB Initial Memory (536870912 bytes) to accommodate 4M/4M/2M/2M buckets without growth overhead
MEMORY_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=402653184 -s INITIAL_MEMORY=268435456"
EXPORT_FLAGS="-s EXPORTED_RUNTIME_METHODS=[cwrap] -s MODULARIZE=1 -s EXPORT_NAME=createF2LEOModule -s INVOKE_RUN=0"
BINDING_FLAG="--bind"

echo "[3/4] Building WASM module..."
echo "  Input:        $INPUT"
echo "  Output:       $OUTPUT"
echo "  Include:      $INCLUDE_PATH"
echo "  Optimization: $OPTIMIZATION"
echo "  Init Memory:  512 MB"
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
if [ -f "$OUTPUT" ] && [ -f "$WASM_OUTPUT" ]; then
    echo ""
    echo "[4/4] Build successful!"
    echo ""
    echo "Output files:"
    ls -lh $OUTPUT $WASM_OUTPUT
    echo ""
    echo "=== Production module ready for deployment ==="
    echo "Module: createF2LEOModule"
    echo "Class:  f2leo_search"
    echo "API:    func(scramble, length)"
else
    echo ""
    echo "ERROR: Build failed - output files not found"
    exit 1
fi
