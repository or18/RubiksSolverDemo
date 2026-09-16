#!/bin/bash
# Production WASM Build Script for xeocross_trainer
# Usage: ./build.sh [debug]
#    debug: Build with assertions for error diagnosis (-O2 -sASSERTIONS=1)
# Output: xeocross_solver_prod.js, xeocross_solver_prod.wasm

set -e

echo "=== XEOCross Solver - Production Build ==="
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
INPUT="xeocross_solver_prod.cpp"
OUTPUT="xeocross_solver_prod.js"
WASM_OUTPUT="xeocross_solver_prod.wasm"

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

# Memory settings:
# Initial: 768 MB (805306368 bytes) - covers peak memory without reallocation
# Maximum: 1024 MB (1073741824 bytes) - safe upper ceiling
MEMORY_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=734003200 -s MAXIMUM_MEMORY=838860800"

# Non-modularized build to match worker.js (uses self.Module and onRuntimeInitialized)
EXPORT_FLAGS="-s EXPORTED_RUNTIME_METHODS=[cwrap] -s INVOKE_RUN=0"
BINDING_FLAG="--bind"

echo "[3/4] Building WASM module..."
echo "  Input:        $INPUT"
echo "  Output:       $OUTPUT"
echo "  Include:      $INCLUDE_PATH"
echo "  Optimization: $OPTIMIZATION"
echo "  Init Memory:  700 MB"
echo "  Max Memory:   800 MB"
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
    echo "Class: xeocross_search"
    echo "API:   func(scramble, length)"
else
    echo ""
    echo "ERROR: Build failed - output files not found"
    exit 1
fi
