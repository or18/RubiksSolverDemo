#!/bin/bash
# Compile PersistentSolver2x2 with MODULARIZE support
# For both Node.js and browser environments

set -e

echo "=== Compiling PersistentSolver2x2 with MODULARIZE ==="

# Load Emscripten environment
if [ -f "/workspaces/emsdk/emsdk_env.sh" ]; then
    source /workspaces/emsdk/emsdk_env.sh
else
    echo "Error: Emscripten SDK not found at /workspaces/emsdk"
    echo "Please install emsdk or adjust the path"
    exit 1
fi

# Compile with MODULARIZE
echo "Compiling solver.cpp..."
em++ solver.cpp -o solver.js \
  -O3 -msimd128 -flto \
  -s TOTAL_MEMORY=150MB \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo ""
    echo "Generated files:"
    ls -lh solver.js solver.wasm
    echo ""
    echo "Usage:"
    echo "  Node.js: node test_simple.js"
    echo "  Browser (Worker): http://localhost:8000/test_worker.html"
    echo "  Browser (Direct): http://localhost:8000/test_modularize.html"
else
    echo "✗ Compilation failed"
    exit 1
fi
