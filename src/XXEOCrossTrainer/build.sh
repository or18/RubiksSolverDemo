#!/bin/bash
set -e

# Emscripten 環境の読み込み
source /workspaces/emsdk/emsdk_env.sh

OUTPUT_JS="xxeocross_solver_prod.js"
OUTPUT_WASM="xxeocross_solver_prod.wasm"
SRC_FILE="xxeocross_solver_prod.cpp"

echo "Building ${OUTPUT_JS}..."

em++ -O3 -I. "${SRC_FILE}" -o "${OUTPUT_JS}" \
    -std=c++17 \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=734003200 \
    -s MAXIMUM_MEMORY=943718400 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createXXEOCrossModule" \
    -s ENVIRONMENT="worker,web" \
    -s DYNAMIC_EXECUTION=0 \
    -s FILESYSTEM=0 \
    --bind

echo "Build successful: ${OUTPUT_JS} and ${OUTPUT_WASM} generated."
