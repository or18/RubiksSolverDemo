#!/bin/bash
# Unified WASM Build for Heap Measurement
# Single binary, bucket model selectable at runtime via UI

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUTPUT_NAME="solver_heap_measurement"

echo "========================================"
echo "Unified WASM Heap Measurement Build"
echo "========================================"
echo "Output: ${OUTPUT_NAME}.js / ${OUTPUT_NAME}.wasm"
echo ""

# Source Emscripten environment
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
fi

# Build with Emscripten
echo "Building WASM module..."
emcc -std=c++17 -O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=64MB \
  -s MAXIMUM_MEMORY=2GB \
  -s STACK_SIZE=5MB \
  -s EXPORTED_FUNCTIONS='["_main","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
  -s ENVIRONMENT='web' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='SolverModule' \
  -s INVOKE_RUN=0 \
  -DVERBOSE_MODE \
  -I.. \
  -o "${OUTPUT_NAME}.js" \
  solver_dev.cpp \
  -lembind

BUILD_STATUS=$?

if [ $BUILD_STATUS -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Files generated:"
    ls -lh "${OUTPUT_NAME}.js" "${OUTPUT_NAME}.wasm" 2>/dev/null || true
    echo ""
    echo "Next steps:"
    echo "  1. python3 -m http.server 8000"
    echo "  2. Open http://localhost:8000/wasm_heap_measurement_unified.html"
    echo "  3. Select bucket configuration from UI"
    echo "  4. Click 'Run Test' and record results"
    echo ""
else
    echo ""
    echo "❌ Build failed with status $BUILD_STATUS"
    exit $BUILD_STATUS
fi
