#!/usr/bin/env bash
set -euo pipefail

# Build script for cpp-functions (modularized Emscripten output)
# Run this from: dist/src/utils/cpp-functions
# Produces: functions.js + functions.wasm

EMCC=${EMCC:-em++}

"${EMCC}" functions.cpp \
  -o functions.js \
  -O3 -msimd128 -flto \
  -s WASM=1 \
  --bind \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=\"createCppFunctionsModule\" \
  -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
  "$@"

echo "Built functions.js (MODULARIZE=1, EXPORT_NAME=createCppFunctionsModule)"
