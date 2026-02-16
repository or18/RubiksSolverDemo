# 2x2x2 Persistent Cube Solver

High-performance WebAssembly-based 2x2x2 Rubik's Cube solver with persistent pruning tables for optimal batch solving.

## 🎯 Quick Start

### Live Demo

Open [demo.html](demo.html) in your browser for an interactive demonstration.

```bash
# Start a local server
python3 -m http.server 8000

# Open in browser
# http://localhost:8000/dist/src/2x2solver/demo.html
```

**Demo Features:**
- ✅ **Full Parameter Control** - Configure all 9 solver parameters
- ♻️ **Reuse Pruning Table** - Toggle persistence for performance testing
- 🌐 **CDN Toggle** - Switch between local files and jsDelivr CDN
- 🔄 **Cache Busting** - Development option for testing latest CDN changes

### 🎁 Simplified Helper API ⭐ Recommended for Beginners

The easiest way to use the solver with a Promise-based API:

**Browser (via local files or CDN):**

```html
<!-- From local files -->
<script src="solver-helper.js"></script>

<!-- OR from CDN (auto-detects Worker path) -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>

<script>
  // Simple 3-line usage!
  const helper = new Solver2x2Helper();
  await helper.init();
  const solutions = await helper.solve("R U R' U'");
  console.log(solutions); // ['U R U\' R\'', 'R\' U\' R\' U R U', ...]
  
  // With options
  const solutions2 = await helper.solve("R U F", {
    maxSolutions: 5,
    maxLength: 12,
    rotation: 'y',
    onProgress: (depth) => console.log(`Searching depth ${depth}`)
  });
  
  // Cleanup when done
  helper.terminate();
</script>
```

**Node.js:**

```javascript
const Solver2x2HelperNode = require('./solver-helper-node.js');

(async () => {
  const helper = new Solver2x2HelperNode();
  await helper.init();
  const solutions = await helper.solve("R U R' U'");
  console.log(solutions);
})();
```

**Helper Benefits:**
- ✅ **No manual `worker.onmessage`** - Promise-based API
- ✅ **async/await support** - Modern JavaScript patterns
- ✅ **Progress callbacks** - Optional real-time updates
- ✅ **CDN auto-detection** - Works with both local and CDN
- ✅ **Cache busting support** - Development-friendly

See [example-helper.html](example-helper.html) for interactive examples (local + CDN modes).

---

## 📚 API Reference

### Helper API (Recommended)

#### Browser: Solver2x2Helper

```javascript
const helper = new Solver2x2Helper(workerPath);  // workerPath optional (auto-detected)

await helper.init();  // Initialize WASM (required before solve)

const solutions = await helper.solve(scramble, options);
// scramble: string, e.g. "R U R' U'"
// options: {
//   maxSolutions?: number (default: 3)
//   maxLength?: number (default: 11)
//   pruneDepth?: number (default: 1, range: 0-20)
//   allowedMoves?: string (default: 'U_U2_U-_R_R2_R-_F_F2_F-')
//   rotation?: string (default: '')
//   preMove?: string (default: '')
//   moveOrder?: string (default: '')
//   moveCount?: string (default: '')
//   onProgress?: (depth: string) => void
//   onSolution?: (solution: string) => void
// }

helper.terminate();  // Clean up Worker
```

#### Node.js: Solver2x2HelperNode

Same API as browser version, see [solver-helper-node.js](solver-helper-node.js) for implementation.

### Direct Worker API (Advanced)

For full control over the Web Worker:

```javascript
const worker = new Worker('worker_persistent.js');

worker.onmessage = function(event) {
  const msg = event.data;
  
  if (msg.type === 'ready') {
    console.log('Solver initialized!');
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

// Send solve request
worker.postMessage({
  scramble: "R U R' U'",
  maxSolutions: 3,
  maxLength: 11,
  pruneDepth: 1,
  allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
});
```

**Worker messages:**

To Worker (postMessage):
```javascript
{
  scramble: string,        // Required
  rotation: string,        // Optional (default: '')
  maxSolutions: number,    // Optional (default: 3)
  maxLength: number,       // Optional (default: 11)
  pruneDepth: number,      // Optional (default: 1, range: 0-20)
  allowedMoves: string,    // Optional (default: URF moves)
  preMove: string,         // Optional (default: '')
  moveOrder: string,       // Optional (default: '')
  moveCount: string        // Optional (default: '')
}
```

From Worker (onmessage):
```javascript
{ type: 'ready' }                    // Initialization complete
{ type: 'solution', data: string }   // Solution found
{ type: 'depth', data: string }      // Search progress
{ type: 'done' }                     // Search complete
{ type: 'error', data: string }      // Error occurred
```

### CDN Usage (Browser)

**Recommended**: Use the Helper API which auto-detects CDN paths:

```html
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
<script>
  // Works automatically! No workerPath needed.
  const helper = new Solver2x2Helper();
  await helper.init();
  const solutions = await helper.solve("R U R' U'");
</script>
```

**Advanced**: Manual Worker loading from CDN (requires Blob URL for CORS bypass):

```javascript
async function createWorkerFromCDN(cdnURL) {
  const response = await fetch(cdnURL + 'worker_persistent.js');
  let code = await response.text();
  
  // Replace baseURL with CDN path
  const oldCode = `const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`;
  const newCode = `const baseURL = '${cdnURL}';`;
  code = code.replace(oldCode, newCode);
  
  // Create Worker via Blob URL
  const blob = new Blob([code], { type: 'application/javascript' });
  return new Worker(URL.createObjectURL(blob));
}

// Usage
const cdnURL = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/';
const worker = await createWorkerFromCDN(cdnURL);
```

See [demo.html](demo.html) for complete CDN implementation with cache busting support.

**Production tip**: Use versioned tags instead of `@main` for stable releases:
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/
```

---

## ⚙️ Configuration

### Parameter Ranges

| Parameter | Range | Default | Format | Description |
|-----------|-------|---------|--------|-------------|
| maxSolutions | 1 to ∞ | 3 | number | Maximum solutions to find |
| maxLength | 1 to 30 | 11 | number | Maximum solution length |
| pruneDepth | 0 to 20 | 1 | number | Pruning table search depth |
| allowedMoves | string | URF (9 moves) | `'MOVE1_MOVE2_...'` | Allowed move set (underscore-separated) |
| rotation | string | '' | `'x'`, `'y'`, `'z'`, `'x2'`, etc. | Whole-cube rotation (empty string for none) |
| preMove | string | '' | `'R U R\' U\''` | Pre-move sequence (space-separated) |
| moveOrder | string | '' | `'ROW~COL\|ROW~COL\|...'` | Move ordering constraints (see notes) |
| moveCount | string | '' | `'U:2_R:3_F:1'` | Move count limits (see notes) |

**Important Notes:**
- **allowedMoves**: Moves must be underscore-separated (e.g., `'U_U2_U-_R_R2_R-'`). Use `-` for inverse (e.g., `U-` = U')
- **rotation**: Only rotations defined in the solver are recognized. Undefined rotations are ignored
- **preMove**: Standard cube notation with spaces (e.g., `'R U R\' U\''`)
- **moveOrder**: Advanced feature for move sequence constraints (rarely used)
- **moveCount**: Limits specific move types (rarely used)

### Optimal Settings for 2x2x2

Based on mathematical properties and performance testing:

**Move Set**: **URF (9 moves) - RECOMMENDED**
```javascript
allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
```

- URF generates a complete system for 2x2x2 (all positions reachable)
- More efficient than full 18-move set (UDLRFB)
- Same optimal solutions, faster search

**Prune Depth**: **1 - RECOMMENDED with URF**
```javascript
pruneDepth: 1
```

- Sufficient for URF complete system
- Faster initialization (~3-4s for first solve)
- Deeper values (8-11) provide minimal improvement with URF

**Performance comparison**:
```
URF + pruneDepth=1:  First solve ~3-4s, Subsequent <1s
Full + pruneDepth=8: First solve ~10-15s, marginal improvement
```

---

## 📁 File Structure

```
dist/src/2x2solver/

# Core Implementation
solver.cpp              # C++ source with PersistentSolver2x2
solver.js               # Compiled WASM module (universal)
solver.wasm             # WebAssembly binary (~195KB)
compile.sh              # Build script

# Web Worker
worker_persistent.js    # Web Worker wrapper (MODULARIZE compatible)

# Helper APIs (Recommended)
solver-helper.js        # Browser Promise-based wrapper
solver-helper-node.js   # Node.js Promise-based wrapper

# Documentation
README.md               # This file (user guide)
IMPLEMENTATION_NOTES.md # C++ implementation details
TROUBLESHOOTING.md      # Common issues & solutions
RELEASE_GUIDE.md        # Release process

# Demo & Examples
demo.html               # Interactive demo (local + CDN)
example-helper.html     # Helper API examples
node-example.js         # Node.js direct usage example

# Development Tools
purge-cdn-cache.sh      # CDN cache purging script
backups/                # Old test files & deprecated docs
```

**Key files for developers:**
- **Quick start**: [demo.html](demo.html) or [example-helper.html](example-helper.html)
- **Production use**: `solver-helper.js` (browser) or `solver-helper-node.js` (Node.js)
- **Advanced**: [worker_persistent.js](worker_persistent.js) for custom Worker implementations
- **Build**: [compile.sh](compile.sh) to rebuild from C++ source

**Documentation:**
- **Usage**: [README.md](README.md) (this file)
- **Implementation**: [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) - C++ changes, persistence architecture
- **Troubleshooting**: [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - CDN issues, common errors, debugging
- **Release**: [RELEASE_GUIDE.md](RELEASE_GUIDE.md) - Versioning, publishing workflow

---

##  Compilation

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
- `solver.js` (~45KB) - Universal MODULARIZE module
- `solver.wasm` (~195KB) - WebAssembly binary

**Single compilation works for**:
- ✅ Node.js (CommonJS/ES6)
- ✅ Browser main thread
- ✅ Web Worker

### Compilation Details

[compile.sh](compile.sh) runs:

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
- `-s MODULARIZE=1`: Factory function pattern (clean API)
- `-s EXPORT_NAME="createModule"`: Exports `createModule()` factory
- `--bind`: Embind C++ bindings

See [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) for C++ source modifications.

---

## 🚀 Examples

### Example 1: Simple Solve (Helper API)

```javascript
const helper = new Solver2x2Helper();
await helper.init();
const solutions = await helper.solve("R U R' U'");
console.log(solutions);  // ['U R U\' R\'', ...]
helper.terminate();
```

### Example 2: With Progress Callback

```javascript
const helper = new Solver2x2Helper();
await helper.init();

const solutions = await helper.solve("R U R' U'", {
  maxSolutions: 5,
  onProgress: (depth) => console.log(`Searching depth ${depth}`),
  onSolution: (sol) => console.log(`Found: ${sol}`)
});

console.log(`Total: ${solutions.length} solutions`);
helper.terminate();
```

### Example 3: Batch Solving (Node.js)

```javascript
const Solver2x2HelperNode = require('./solver-helper-node.js');

(async () => {
  const helper = new Solver2x2HelperNode();
  await helper.init();  // Table built once
  
  const scrambles = ["R U R' U'", "R2 U2", "U R2 U'", "F R F' R'"];
  
  for (const scramble of scrambles) {
    const solutions = await helper.solve(scramble);  // Reuses table
    console.log(`${scramble}: ${solutions[0]}`);
  }
  
  // helper automatically maintains table for all solves
})();
```

### Example 4: CDN Loading (Dynamic)

```html
<script>
  // Load helper from CDN dynamically
  const script = document.createElement('script');
  script.src = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js';
  document.head.appendChild(script);
  
  script.onload = async () => {
    const helper = new Solver2x2Helper();  // Auto-detects CDN Worker path
    await helper.init();
    const solutions = await helper.solve("R U R' U'");
    console.log(solutions);
  };
</script>
```

See [example-helper.html](example-helper.html) for live interactive examples.

### Example 5: Node.js without Helper (Direct Module Usage)

```javascript
// node-example.js
// Run: node node-example.js

let currentMessageHandler = (msg) => console.log('Unhandled:', msg);

// CRITICAL: Define globalThis.postMessage BEFORE loading module
globalThis.postMessage = function(msg) {
    currentMessageHandler(msg);
};

const createModule = require('./solver.js');

// URF moves only (optimal for 2x2x2)
const urfMoves = "U_U2_U-_R_R2_R-_F_F2_F-";

createModule().then(Module => {
    console.log('=== PersistentSolver2x2 Example ===\n');
    
    // Create persistent solver instance
    let solver = new Module.PersistentSolver2x2();
    console.log('✓ Created solver instance\n');
    
    // Solve first scramble
    console.log('Solving: R U R\' U\'');
    let solutionCount = 0;
    
    currentMessageHandler = (msg) => {
        if (msg.startsWith('depth=')) {
            console.log('  ' + msg.trim());
        } else if (msg === 'Search finished.') {
            console.log('  ' + msg + '\n');
        } else if (msg !== 'Already solved.') {
            solutionCount++;
            console.log('  Solution ' + solutionCount + ': ' + msg.trim());
        }
    };
    
    // solve(scramble, rotation, maxSolutions, maxLength, pruneDepth, allowedMoves, preMove, moveOrder, moveCount)
    solver.solve("R U R' U'", "", 3, 11, 1, urfMoves, "", "", "");
    console.log(`✓ Found ${solutionCount} solutions\n`);
    
    // Solve second scramble (reuses prune table)
    console.log('Solving: R2 U2 (reusing prune table)');
    solutionCount = 0;
    solver.solve("R2 U2", "", 3, 11, 1, urfMoves, "", "", "");
    console.log(`✓ Found ${solutionCount} solutions\n`);
    
    console.log('=== Complete! ===');
    console.log('Prune table was built once and reused for both solves.');
});
```

**Notes:**
- No helper required - direct PersistentSolver2x2 usage
- `globalThis.postMessage` must be defined **before** loading module
- Solver persists prune table between `solve()` calls

---

## 🐛 Troubleshooting

For detailed troubleshooting, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

### Quick Fixes

**Issue**: "Worker not found" error
```
Solution: Verify Worker path. Use Helper API for auto-detection.
```

**Issue**: CDN serving old files
```
Solution: Run ./purge-cdn-cache.sh and wait 1-2 minutes
```

**Issue**: Browser hanging during solve
```
Solution: Use Web Worker (worker_persistent.js or Helper API)
```

**Issue**: Solutions not found
```
Solution: Increase maxLength, verify scramble notation, check allowedMoves
```

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for:
- CDN cache debugging
- CORS issues
- Worker path problems
- Performance optimization
- Common error messages

---

## 📖 Additional Resources

- **Implementation Details**: [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md)
  - C++ source modifications
  - Persistent solver architecture
  - MODULARIZE build system

- **Common Issues**: [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
  - CDN cache problems
  - Worker loading errors
  - Performance debugging

- **Release Process**: [RELEASE_GUIDE.md](RELEASE_GUIDE.md)
  - Versioning strategy
  - CDN deployment
  - Git tagging workflow

- **Interactive Demos**:
  - [demo.html](demo.html) - Full-featured solver demo
  - [example-helper.html](example-helper.html) - Helper API examples

---

## 📝 License

See top-level [LICENSE](../../../LICENSE) file.

## 🤝 Contributing

This is part of the RubiksSolverDemo project. See main [README](../../../README.md).

---

## Version History

- **v1.0.0** (2026-02-16): Initial release with persistent solver
