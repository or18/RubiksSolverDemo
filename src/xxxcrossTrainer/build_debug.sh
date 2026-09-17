#!/bin/bash
set -e

source /workspaces/emsdk/emsdk_env.sh

OUTPUT_JS="xxxcross_solver_prod.js"
OUTPUT_WASM="xxxcross_solver_prod.wasm"
SRC_FILE="xxxcross_solver_prod.cpp"

echo "Building ${OUTPUT_JS} with debug assertions..."

em++ -O1 -g -I. "${SRC_FILE}" -o "${OUTPUT_JS}" \
    -std=c++17 \
    -s WASM=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=734003200 \
    -s MAXIMUM_MEMORY=943718400 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createXXXCrossModule" \
    -s ENVIRONMENT="worker,web" \
    -s DYNAMIC_EXECUTION=0 \
    -s FILESYSTEM=0 \
    -s ASSERTIONS=2 \
    -s SAFE_HEAP=1 \
    -s STACK_OVERFLOW_CHECK=2 \
    --bind

echo "Build successful: ${OUTPUT_JS} and ${OUTPUT_WASM} generated (Debug mode)."
