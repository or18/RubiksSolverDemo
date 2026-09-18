#!/bin/bash
set -e

# Emscripten SDK activation
if [ -f "/workspaces/emsdk/emsdk_env.sh" ]; then
    source /workspaces/emsdk/emsdk_env.sh
elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh"
fi

INPUT="f2leo_solver_prod.cpp"
OUTPUT="f2leo_xcross_solver.js"

# Memory config: 700 MB Initial, 900 MB Maximum (64KB aligned)
MEMORY_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=734003200 -s MAXIMUM_MEMORY=943718400"
EXPORT_FLAGS="-s EXPORTED_RUNTIME_METHODS=[cwrap] -s MODULARIZE=1 -s EXPORT_NAME=createF2LEOXCrossModule -s INVOKE_RUN=0"

echo "Building $OUTPUT from $INPUT..."

em++ $INPUT \
  -o $OUTPUT \
  -std=c++17 \
  -O3 -msimd128 -flto \
  -I. \
  -s WASM=1 \
  $MEMORY_FLAGS \
  $EXPORT_FLAGS \
  --bind

echo "Build successful: $OUTPUT, ${OUTPUT%.js}.wasm"
ls -lh $OUTPUT "${OUTPUT%.js}.wasm"
