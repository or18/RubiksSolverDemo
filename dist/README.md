# Rubik's Cube Solver Libraries

High-performance WebAssembly-based Rubik's Cube solvers with persistent pruning tables for optimal batch solving.

## Overview

This library provides WebAssembly-compiled cube solvers with persistent state optimization. Solvers build expensive pruning tables once and reuse them across multiple solves, dramatically improving performance for batch operations.

## 🚀 Quick Start

All examples use the **Helper API** - a Promise-based wrapper that eliminates complex worker message handling.

### Browser (HTML + CDN)

Copy-paste ready example using CDN:

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>2x2 Solver</title>
</head>
<body>
  <h1>2x2 Solver Demo</h1>
  <button onclick="solve()">Solve</button>
  <div id="output"></div>

  <script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
  <script>
    const helper = new Solver2x2Helper();
    
    async function solve() {
      await helper.init();
      const solutions = await helper.solve("R U R' U'", {
        maxSolutions: 5,
        onProgress: (depth) => {
          console.log(`Searching depth: ${depth}`);
        }
      });
      
      document.getElementById('output').innerHTML = 
        solutions.map((s, i) => `${i+1}. ${s}`).join('<br>');
    }
  </script>
</body>
</html>
```

### Node.js

```javascript
const { Solver2x2HelperNode } = require('./src/2x2solver/solver-helper-node.js');

async function solve() {
  const helper = new Solver2x2HelperNode();
  
  await helper.init();
  console.log('✅ Solver ready!\n');
  
  const solutions = await helper.solve("R U R' U'", {
    maxSolutions: 5,
    onProgress: (depth) => console.log(`Depth: ${depth}`),
    onSolution: (sol) => console.log(`Found: ${sol}`)
  });
  
  console.log(`\nTotal: ${solutions.length} solutions`);
}

solve().catch(console.error);
```

### Local Files (Browser)

```html
<script src="src/2x2solver/solver-helper.js"></script>
<script>
  const helper = new Solver2x2Helper();
  
  (async () => {
    await helper.init();
    const solutions = await helper.solve("R U R' U'");
    console.log(solutions); // ['U2 R', 'U F2 U', ...]
  })();
</script>
```

## 📊 Performance

| Solve | Time | Notes |
|-------|------|-------|
| **1st** | ~3-4s | Builds pruning table (~84MB) |
| **2nd+** | <1s | **Reuses table** |

**Batch Example**: 4 scrambles in ~5-6 seconds (vs. ~12-16s without persistence)

## ⚙️ Configuration

```javascript
const solutions = await helper.solve(scramble, {
  maxSolutions: 3,        // No upper limit (use with caution)
  maxLength: 11,          // 1-30 moves
  pruneDepth: 1,          // 0-20 (1 recommended for URF)
  allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-', // URF move set
  rotation: '',           // Cube rotation (e.g., 'y', 'x2')
  preMove: '',            // Pre-move sequence
  onProgress: (depth) => console.log(depth)  // Progress callback
});
```

## 📡 Streaming Output

### Browser

```javascript
const solutions = await helper.solve("R U R' U'", {
  maxSolutions: 10,
  onProgress: (depth) => {
    console.log(`🔍 Searching depth: ${depth}`);
  }
});
```

### Node.js (True Streaming)

```javascript
const solutions = await helper.solve("R U R' U'", {
  maxSolutions: 10,
  onProgress: (depth) => console.log(`Depth: ${depth}`),
  onSolution: (sol) => console.log(`✅ ${sol}`)  // Real-time!
});
```

See [example-helper.html](src/2x2solver/example-helper.html) for interactive examples.

## 🎮 Interactive Demos

Try the solver directly in your browser:

```bash
cd dist/src/2x2solver
python3 -m http.server 8000
# Open http://localhost:8000/demo.html
```

**Demo Features:**
- ✅ **Full Parameter Control** - All 9 solver parameters with validation
- 🔄 **Reuse Pruning Table** - Toggle table persistence for performance testing
- 🌐 **CDN Toggle** - Switch between local files and jsDelivr CDN
- 🔄 **Cache Busting** - Development option for testing latest CDN changes
- 📊 **Real-time Results** - Solutions display as they're found

Both [demo.html](src/2x2solver/demo.html) and [example-helper.html](src/2x2solver/example-helper.html) support:
- **Local mode** (default) - Uses files in current directory
- **CDN mode** - Loads from jsDelivr with optional cache busting

## 🌐 CDN Usage

**Recommended for production** - Use versioned tags:

```html
<!-- Stable version (permanent cache) -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@v1.0.0/dist/src/2x2solver/solver-helper.js"></script>

<!-- Latest development (may change) -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
```

### CDN Cache Management (Developers)

When updating code, jsDelivr may cache old versions.

**Method 1: Interactive Demo Pages**

Both [demo.html](src/2x2solver/demo.html) and [example-helper.html](src/2x2solver/example-helper.html) have built-in CDN controls:

1. Open demo page in browser
2. Check "🌐 Use CDN (jsDelivr)"
3. Check "🔄 Cache Busting" (development only)
4. Test your changes with bypassed cache

⚠️ **Cache Busting** adds `?v=timestamp` to URLs - only affects your browser, safe for testing.

**Important:** If you see old code even with cache busting enabled:

1. **Check Console** - Look for `[Solver2x2Helper] v1.1.0 loaded from: ...` to verify version
2. **Hard Refresh Browser Cache**:
   - Windows/Linux: `Ctrl + Shift + R` or `Ctrl + F5`
   - Mac: `Cmd + Shift + R`
3. **Clear Browser Cache** (if still old):
   - Chrome: DevTools → Network tab → "Disable cache" checkbox
   - Or clear site data: DevTools → Application → Clear storage

**Method 2: Instant Purge** (affects all users globally):
```bash
cd dist/src/2x2solver
./purge-cdn-cache.sh
```

⚠️ **Warning**: This clears CDN cache globally. Use for development only.

**After purging CDN**, you must also hard-refresh your browser (see above) to clear local cache.
- Open [demo.html](src/2x2solver/demo.html) and enable "🔄 Cache Busting" checkbox
- Only affects your browser, safe for testing

## 📁 2x2x2 Solver Files

- `solver.cpp` - C++ solver with PersistentSolver2x2
- `solver.js` + `solver.wasm` - Universal binary (Node.js + Browser)
- `solver-helper.js` - Promise-based Web Worker API ⭐
- `solver-helper-node.js` - Promise-based Node.js API ⭐
- `worker_persistent.js` - Low-level Web Worker (advanced)
- `demo.html` - Interactive demo with full UI
- `example-helper.html` - Code examples and tutorials

📖 **[Full Documentation](./src/2x2solver/README.md)** - Complete API reference and advanced usage

📖 **[Technical Details](./src/2x2solver/PERSISTENT_SOLVER_README.md)** - Implementation guide

## 🔧 Compilation

```bash
cd dist/src/2x2solver
source /path/to/emsdk/emsdk_env.sh
./compile.sh
```

See [compile.sh](src/2x2solver/compile.sh) for compilation flags.

## 📦 Other Solvers

Additional solvers available in `src/` directory:

- **Cross Solver** (`crossSolver/`) - First step cross solving
- **EOCross Solver** (`EOCrossSolver/`) - Edge orientation + cross
- **XCross Trainer** (`xcrossTrainer/`) - Extended cross training
- **XXCross Trainer** (`xxcrossTrainer/`) - Double cross
- **F2L Pairing** (`F2L_PairingSolver/`) - First two layers

Each solver has its own README with usage instructions.

## 🎯 Interactive Demos

Try the solvers in your browser:

```bash
cd dist/src/2x2solver
python3 -m http.server 8000
# Open http://localhost:8000/demo.html
```

## Browser Requirements

- WebAssembly support
- Web Workers (for non-blocking operation)
- ES6 modules (for Helper API)

All modern browsers (Chrome, Firefox, Safari, Edge) are supported.

## License

See repository root for license information.

## Contributing

Contributions welcome! Please see the main repository for guidelines.

