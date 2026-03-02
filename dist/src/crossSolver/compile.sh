#!/bin/bash
# Compile crossSolver with MODULARIZE + ASYNCIFY + ALLOW_MEMORY_GROWTH support.
# Multiple Persistent solver classes (Cross, Xcross, Xxcross, Xxxcross, Xxxxcross,
# LLSubsteps, LL, LLAUF) each allocate their own large prune tables.
#
# Memory sizing:
#   Cross prune table     :  ~0.3 MB
#   Xcross prune table    :  ~5.5 MB
#   Xxcross prune tables  : ~11 MB (x2)
#   Xxxcross/Xxxxcross/LL : ~22 MB (x3-4)
#
# INITIAL_MEMORY=50MB comfortably hosts cross + xcross + xxcross(dual) all at once.
# ALLOW_MEMORY_GROWTH=1 lets LL-family solvers expand as needed without OOM.

set -e

echo "=== Compiling crossSolver with MODULARIZE ==="

# Load Emscripten environment
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh"
else
    echo "Error: Emscripten SDK not found at ~/emsdk"
    echo "Please install emsdk or adjust the path"
    exit 1
fi

echo "Compiling solver.cpp..."
em++ solver.cpp -o solver.js \
  -O3 -msimd128 \
  -s ASYNCIFY=1 \
  -s INITIAL_MEMORY=50MB \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo ""
    echo "Generated files:"
    ls -lh solver.js solver.wasm
else
    echo "✗ Compilation failed"
    exit 1
fi
