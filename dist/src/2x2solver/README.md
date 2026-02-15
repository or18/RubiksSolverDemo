# 2x2x2 Persistent Cube Solver

High-performance WebAssembly-based 2x2x2 Rubik's Cube solver with persistent pruning tables for optimal batch solving.

## 🎯 Quick Start

### Live Demo

Open [demo.html](demo.html) in your browser for an interactive demonstration.

```bash
# Start a local server
python3 -m http.server 8000

# Open in browser
# http://localhost:8000/demo.html
```

**Demo Features:**
- ✅ **Full Parameter Control** - Configure all 9 solver parameters
- ♻️ **Reuse Pruning Table** - Toggle persistence for performance testing
- 🌐 **CDN Toggle** - Switch between local files and jsDelivr CDN
- 🔄 **Cache Busting** - Development option for testing latest CDN changes

Both [demo.html](demo.html) and [example-helper.html](example-helper.html) support local and CDN modes.

### Web Worker API (Recommended)

The recommended approach for browser applications. Runs in a separate thread to avoid blocking the UI.

**Local files:**

```javascript
// Create worker (same origin, no CORS issues)
const worker = new Worker('worker_persistent.js');

// Handle messages
worker.onmessage = function(event) {
  const msg = event.data;
  
  if (msg.type === 'ready') {
    console.log('Solver initialized!');
    // Now you can send solve requests
  } else if (msg.type === 'solution') {
    console.log('Solution:', msg.data);
  } else if (msg.type === 'depth') {
    console.log('Progress:', msg.data);
  } else if (msg.type === 'done') {
    console.log('Search complete');
  } else if (msg.type === 'error') {
    console.error('Error:', msg.data);
  }
};

// Solve a scramble
worker.postMessage({
  scramble: "R U R' U'",
  maxSolutions: 3,
  maxLength: 11,
  pruneDepth: 1,
  allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
});
```

### 🎁 Simplified Helper API ⭐ Recommended for Beginners

For easier usage, use the `Solver2x2Helper` which wraps the worker with a Promise-based API:

```html
<!-- Include the helper -->
<script src="solver-helper.js"></script>

<script>
  // Simple 3-line usage!
  const helper = new Solver2x2Helper();
  await helper.init();
  const solutions = await helper.solve("R U R' U'");
  console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
  
  // With options
  const solutions2 = await helper.solve("R U F", {
    maxSolutions: 5,
    rotation: 'y',
    onProgress: (depth) => console.log(`Searching depth ${depth}`)
  });
  
  // Cleanup
  helper.terminate();
</script>
```

**Helper Benefits:**
- ✅ **No manual `worker.onmessage`** - Promise-based API
- ✅ **async/await support** - Modern JavaScript patterns
- ✅ **Progress callbacks** - Optional real-time updates
- ✅ **Type-safe** - Clear parameter documentation

See [example-helper.html](example-helper.html) for interactive examples.

### 🔧 Direct Worker API (Advanced)

## 🎮 Interactive Demo

Open [demo.html](demo.html) in your browser for a full-featured interactive demo:

**Features:**
- ⚙️ All 9 parameters configurable via UI
- ✅ Real-time input validation (client + worker)
- 📊 Live statistics (solutions, time)
- 🎯 Preset move restrictions (URF, Full, Custom)
- 📝 Help text for each parameter

**Usage:**
```bash
cd dist/src/2x2solver
python -m http.server 8000
# Open http://localhost:8000/demo.html
```

The demo uses local Worker - no CDN required!

---

## 🌐 CDN Usage

**From CDN (requires Blob URL for CORS):**

```javascript
// Fetch worker from CDN and create via Blob URL
async function createWorkerFromCDN(cdnURL) {
  const response = await fetch(cdnURL + 'worker_persistent.js');
  let code = await response.text();
  
  // Replace baseURL calculation with CDN URL
  const oldCode = `const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`;
  const newCode = `const baseURL = '${cdnURL}';`;
  code = code.replace(oldCode, newCode);
  
  const blob = new Blob([code], { type: 'application/javascript' });
  return new Worker(URL.createObjectURL(blob));
}

// Usage
const cdnURL = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/';
const worker = await createWorkerFromCDN(cdnURL);

// Use same message handlers as above
worker.onmessage = function(event) { /* ... */ };
worker.postMessage({ scramble: "R U R' U'", /* ... */ });
```

See [cdn-test.html](cdn-test.html) for a complete CDN loading example.

**Key Features:**
- ✅ **Non-blocking**: UI remains responsive during computation
- ✅ **Persistent tables**: First solve builds pruning table (~3-4s), subsequent solves are fast (<1s)
- ✅ **URF optimization**: Uses 9 URF moves (U, U2, U', R, R2, R', F, F2, F') for optimal 2x2x2 solving

### Node.js API

For server-side or batch processing applications.

```javascript
const createModule = require('./solver.js');

(async () => {
  // Initialize module
  const Module = await createModule();
  
  // Create persistent solver instance
  const solver = new Module.PersistentSolver2x2();
  
  // Override postMessage to capture solutions
  let solutions = [];
  globalThis.postMessage = function(msg) {
    if (msg.startsWith('depth=')) {
      console.log(msg); // Progress
    } else if (msg === 'Search finished.') {
      console.log(`Found ${solutions.length} solutions`);
    } else if (msg !== '') {
      solutions.push(msg);
    }
  };
  
  // Solve multiple scrambles (reuses pruning table)
  const scrambles = [
    "R U R' U'",
    "R2 U2",
    "U R2 U'",
    "U' F2 U R' U' R F' R F2 U"
  ];
  
  for (const scramble of scrambles) {
    solutions = [];
    
    solver.solve(
      scramble,        // scramble
      '',              // rotation
      3,               // maxSolutions
      11,              // maxLength
      1,               // pruneDepth
      'U_U2_U-_R_R2_R-_F_F2_F-', // allowedMoves (URF)
      '',              // preMove
      '',              // moveOrder
      ''               // moveCount
    );
    
    console.log(`Scramble: ${scramble}`);
    console.log(`Solutions:`, solutions);
  }
})();
```

### 🎁 Node.js Helper API ⭐ Recommended for Beginners

For easier usage, use the `Solver2x2HelperNode` which provides a Promise-based API:

```javascript
const Solver2x2HelperNode = require('./solver-helper-node.js');

(async () => {
  // Simple 3-line usage!
  const helper = new Solver2x2HelperNode();
  await helper.init();
  const solutions = await helper.solve("R U R' U'");
  console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
  
  // With options
  const solutions2 = await helper.solve("R U F", {
    maxSolutions: 5,
    rotation: 'y',
    onProgress: (depth) => console.log(`Searching depth ${depth}`),
    onSolution: (sol) => console.log(`Found: ${sol}`)
  });
  
  // Solver automatically reuses pruning table for better performance!
})();
```

**Helper Benefits:**
- ✅ **No manual `globalThis.postMessage`** - Promise-based API
- ✅ **async/await support** - Modern JavaScript patterns
- ✅ **Automatic table reuse** - Optimal performance
- ✅ **Progress & solution callbacks** - Real-time updates

See `backups/test_files/example-helper-node.js` for more examples.

---

## 📚 API Reference

### Worker Messages

#### To Worker (postMessage)

```javascript
{
  scramble: string,        // Required: scramble to solve
  rotation: string,        // Optional: whole-cube rotation (default: '')
  maxSolutions: number,    // Optional: max solutions to find (default: 3)
  maxLength: number,       // Optional: max solution length (default: 11)
  pruneDepth: number,      // Optional: search depth (default: 1, range: 1-11)
  allowedMoves: string,    // Optional: allowed move set (default: URF)
  preMove: string,         // Optional: pre-move sequence (default: '')
  moveOrder: string,       // Optional: move order constraints (default: '')
  moveCount: string        // Optional: move count limits (default: '')
}
```

#### From Worker (onmessage)

```javascript
// Initialization complete
{ type: 'ready', data: null }

// Solution found
{ type: 'solution', data: "U' F2 R' F R' U R U' F2 U" }

// Search progress
{ type: 'depth', data: "depth=5" }

// Search complete
{ type: 'done', data: null }

// Error occurred
{ type: 'error', data: "error message" }
```

### C++ API (via Emscripten)

```cpp
class PersistentSolver2x2 {
public:
  void solve(
    std::string scramble,      // Scramble to solve
    std::string rotation,      // Whole-cube rotation
    int max_solutions,         // Maximum number of solutions
    int max_length,            // Maximum solution length
    int prune_depth,           // Pruning depth (1-11)
    std::string move_restrict, // Allowed moves (underscore-separated)
    std::string post_alg,      // Post-algorithm
    std::string move_order,    // Move ordering
    std::string move_count     // Move count constraints
  );
};
```

## ⚙️ Configuration

### Move Restrictions

The solver supports different move sets. For 2x2x2, **URF moves are optimal**:

```
'U_U2_U-_R_R2_R-_F_F2_F-'  // URF (9 moves) - RECOMMENDED
```

URF moves form a complete system that generates all positions of a 2x2x2 cube optimally.

Other move sets (not recommended for 2x2x2):
```
'U_U2_U-_R_R2_R-'                              // UR only (6 moves)
'U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-'  // All moves (18 moves, excessive)
```

### Prune Depth

Controls the search depth for the pruning table:

- `pruneDepth: 1` - **Recommended for URF moves** (fast, sufficient for URF complete system)
- `pruneDepth: 8` - Deeper search (slower initialization, marginally better for some cases)
- Valid range: 1-11

**With URF moves, `pruneDepth: 1` is optimal** - provides complete coverage with minimal overhead.

### Max Length

Maximum solution length to search:

- `maxLength: 11` - **Recommended** (handles up to 11-move solutions)
- Increase for longer scrambles, decrease for faster searches

## 🔧 Compilation

### Requirements

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

### Build

```bash
# Load Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Compile
cd dist/src/2x2solver
./compile.sh
```

This produces:
- `solver.js` - Universal MODULARIZE module
- `solver.wasm` - WebAssembly binary

**Single compilation works for**:
- ✅ Node.js (CommonJS/ES6)
- ✅ Browser main thread
- ✅ Web Worker

### Compilation Details

The `compile.sh` script runs:

```bash
em++ solver.cpp -o solver.js \
  -O3 \
  -msimd128 \
  -flto \
  -s TOTAL_MEMORY=150MB \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind
```

**Flags**:
- `-O3`: Maximum optimization
- `-msimd128`: SIMD vectorization
- `-flto`: Link-time optimization
- `-s TOTAL_MEMORY=150MB`: Fixed memory (sufficient for ~84MB pruning table)
- `-s MODULARIZE=1`: Factory function pattern
- `-s EXPORT_NAME="createModule"`: Exports `createModule()` factory
- `--bind`: Embind C++ bindings

## 📊 Performance

### Typical Performance

| Solve | Scramble | Time | Notes |
|-------|----------|------|-------|
| **1st** | `R U R' U'` | ~3-4s | Builds pruning table (~84MB) |
| **2nd** | `R2 U2` | <1s | Reuses table |
| **3rd** | `U R2 U'` | <1s | Reuses table |
| **4th** | `U' F2 U R' U' R F' R F2 U` | ~1-2s | 10-move scramble, reuses table |

**Total**: ~5-6 seconds for 4 scrambles

### Performance Tips

1. **Use Web Worker in browser** - prevents UI blocking
2. **Keep solver instance alive** - reuses expensive pruning tables
3. **Use URF moves** - optimal for 2x2x2, fastest searches
4. **Set pruneDepth=1 with URF** - sufficient for complete coverage

## 📁 Files

### Core Implementation
- `solver.cpp` - C++ solver with PersistentSolver2x2
- `solver.js` + `solver.wasm` - Compiled WASM module (universal)
- `compile.sh` - Compilation script

### Browser Usage
- `worker_persistent.js` - Web Worker wrapper (advanced)
- `solver-helper.js` - Promise-based helper for Web Workers ⭐ Recommended
- `demo.html` - Interactive demo with full parameter control
- `example-helper.html` - Helper API examples

### Node.js Usage
- `solver-helper-node.js` - Promise-based helper for Node.js ⭐ Recommended

### Documentation
- `README.md` - This file (quick start & API reference)
- `PERSISTENT_SOLVER_README.md` - Detailed implementation guide
- `RELEASE_GUIDE.md` - Release process and versioning guide

### Development Tools
- `purge-cdn-cache.sh` - CDN cache purging script
- `backups/` - Test files and old implementations

## 🚀 Examples

### Example 1: Single Solve (Worker)

```javascript
const worker = new Worker('worker_persistent.js');

worker.onmessage = (e) => {
  if (e.data.type === 'ready') {
    worker.postMessage({
      scramble: "R U R' U'",
      maxSolutions: 3,
      maxLength: 11,
      pruneDepth: 1,
      allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
    });
  } else if (e.data.type === 'solution') {
    console.log('Solution:', e.data.data);
  } else if (e.data.type === 'done') {
    console.log('Complete!');
  }
};
```

### Example 2: Batch Solving (Node.js)

See [backups/test_files/test_simple.js](backups/test_files/test_simple.js) for a complete example.

```javascript
const createModule = require('./solver.js');

(async () => {
  const Module = await createModule();
  const solver = new Module.PersistentSolver2x2();
  
  // ... (see Node.js API section above)
})();
```

### Example 3: Web Application

See [demo.html](demo.html) for a complete web application example with:
- Real-time progress updates
- Solution display
- Performance statistics
- Test suite

## 🐛 Troubleshooting

### Browser: `postMessage` Error

The solver uses `globalThis.postMessage` which is overridden in `worker_persistent.js`. No action needed when using the Worker.

If using MODULARIZE directly in browser main thread (not recommended):

```javascript
// Define before loading solver.js
globalThis.postMessage = function(msg) {
  console.log(msg);
};

// Then load
const Module = await createModule();
```

### Node.js: Module Not Found

Ensure you're using the MODULARIZE compiled version:

```javascript
// For CommonJS
const createModule = require('./solver.js');

// For ES6 modules
import createModule from './solver.js';
```

### Solutions Not Found

1. Check move restrictions - URF moves cover all 2x2x2 positions
2. Increase `maxLength` if scramble is very long
3. Verify scramble notation is correct (space-separated moves)

### CDN Cache Issues

If you just pushed code changes but CDN still serves old files:

**Method 1: Purge CDN Cache (Immediate)**
```bash
./purge-cdn-cache.sh
```
This script calls jsDelivr's Purge API to instantly clear cached files:
- `worker_persistent.js`
- `solver.js`
- `solver.wasm`

Wait 10-30 seconds after purging, then reload your page.

**Method 2: Cache Busting (For Testing)**

1. Open [cdn-test.html](cdn-test.html)
2. Enable "🔄 Enable Cache Busting" checkbox
3. Initialize the worker

Cache busting adds `?v=<timestamp>` to all CDN URLs, forcing fresh downloads on every load. Use this during development, but disable for production.

**Method 3: Version Tags (For Production)**

Instead of `@main`, use release tags:
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@v1.0.0/dist/src/2x2solver/
```
Versioned URLs are permanently cached and guarantee stability.

**CDN Update Timeline:**
- **jsDelivr**: 2-12 hours typical (can purge immediately with script)
- **Purge Script**: Instant, but affects all users globally ⚠️
- **Cache Busting**: Local only, safe for testing ✅
- **Browser Cache**: Ctrl+Shift+R to force reload

**Production Recommendation:**
Use Git tags (`@v1.0.0`) instead of `@main` for stable releases. Tagged versions are permanently cached and won't be affected by purge operations.

## 📖 Additional Resources

- **Detailed Guide**: See [PERSISTENT_SOLVER_README.md](PERSISTENT_SOLVER_README.md)
- **Demo**: Open [demo.html](demo.html) in browser
- **Test Suite**: See `backups/test_files/` for test examples
- **Web Worker Pattern**: Study [worker_persistent.js](worker_persistent.js)

## 📝 License

See top-level [LICENSE](../../../LICENSE) file.

## 🤝 Contributing

This is part of the RubiksSolverDemo project. See main [README](../../../README.md).
