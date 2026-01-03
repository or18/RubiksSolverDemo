#!/bin/bash
# Build WASM for Web Worker test (test_wasm_browser.html)
# Non-MODULARIZE build for importScripts compatibility

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUTPUT_NAME="test_solver"

echo "========================================"
echo "WASM Test Build (Web Worker)"
echo "========================================"
echo "Output: ${OUTPUT_NAME}.js / ${OUTPUT_NAME}.wasm"
echo ""

# Source Emscripten environment
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
fi

# Build WITHOUT MODULARIZE (for importScripts in Worker)
echo "Building WASM module for Web Worker..."
emcc -std=c++17 -O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=64MB \
  -s MAXIMUM_MEMORY=2GB \
  -s STACK_SIZE=5MB \
  -s EXPORTED_FUNCTIONS='["_main","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
  -s ENVIRONMENT='web,worker' \
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
    echo "Usage:"
    echo "  1. importScripts('test_solver.js') in worker_test_wasm_browser.js"
    echo "  2. python3 -m http.server 8000"
    echo "  3. Open http://localhost:8000/test_wasm_browser.html"
    echo ""
else
    echo ""
    echo "❌ Build failed with status $BUILD_STATUS"
    exit $BUILD_STATUS
fi
