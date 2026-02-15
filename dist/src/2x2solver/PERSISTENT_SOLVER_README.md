# Persistent Solver for 2x2 Cube

## Overview

The `PersistentSolver2x2` struct allows you to reuse expensive pruning tables across multiple solves, significantly improving performance when solving many scrambles with the same search parameters.

## Recommended: Web Worker Usage (Browser)

For browser environments, **always use the Web Worker** to avoid blocking the main thread. The solver can take several seconds on the first solve.

### Worker Example ([test_worker.html](test_worker.html))

```html
<script>
    let worker = new Worker('worker_persistent.js');
    
    worker.onmessage = function(event) {
        const msg = event.data;
        
        if (msg.type === 'ready') {
            console.log('Worker ready!');
            // Now you can send solve requests
        } else if (msg.type === 'solution') {
            console.log('Solution:', msg.data);
        } else if (msg.type === 'depth') {
            console.log('Progress:', msg.data);
        } else if (msg.type === 'done') {
            console.log('Search finished');
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
</script>
```

**Benefits of Worker:**
- Non-blocking: UI remains responsive during computation
- Background processing: Can solve while user interacts with page
- Persistent tables: Prune tables stay in memory across solves

## Usage

### Node.js Example (MODULARIZE version)

```javascript
// CRITICAL: Define globalThis.postMessage BEFORE loading module
let currentMessageHandler = (msg) => console.log(msg);

globalThis.postMessage = function(msg) {
    currentMessageHandler(msg);
};

const createModule = require('./solver.js');

// URF moves only (optimal for 2x2x2)
const urfMoves = "U_U2_U-_R_R2_R-_F_F2_F-";

createModule().then(Module => {
    // Create a persistent solver instance
    const solver = new Module.PersistentSolver2x2();
    
    // Set up message handler
    currentMessageHandler = (msg) => {
        if (msg.startsWith('depth=')) {
            console.log('Searching ' + msg);
        } else if (msg !== 'Already solved.' && msg !== 'Search finished.') {
            console.log('Solution: ' + msg);
        }
    };
    
    // Solve multiple scrambles (table created once, reused)
    solver.solve("R U R' U'", "", 3, 11, 1, urfMoves, "", "", "");
    solver.solve("R2 U2", "", 3, 11, 1, urfMoves, "", "", "");
});
```

### Browser Example (Direct, Non-Worker)

⚠️ **Warning**: Direct usage in browser will block the main thread. Use Web Worker instead for production.

```html
<!-- Define postMessage handler BEFORE loading solver.js -->
<script>
    // Create a handler that can be dynamically reassigned
    let postMessageHandler = function(msg) {
        console.log(msg);
    };
    
    // Use globalThis for maximum compatibility
    globalThis.postMessage = function(msg) {
        postMessageHandler(msg);
    };
</script>

<script src="solver.js"></script>

<script>
createModule().then(Module => {
    // Create persistent solver
    const solver = new Module.PersistentSolver2x2();
    
    // Set up the message handler for this solve
    postMessageHandler = (msg) => {
        if (msg.startsWith('depth=')) {
            console.log('Searching ' + msg);
        } else if (msg === 'Search finished.') {
            console.log('Complete!');
        } else if (msg !== 'Already solved.') {
            console.log('Solution: ' + msg);
        }
    };
    
    // Solve scrambles (table created on first solve, reused after)
    solver.solve("R U R' U'", "", 3, 11, 1, "U_U2_U-_R_R2_R-_F_F2_F-", "", "", "");
    solver.solve("R2 U2", "", 3, 11, 1, "U_U2_U-_R_R2_R-_F_F2_F-", "", "", "");
});
</script>
```

**Critical**: You must define `globalThis.postMessage` **before** loading `solver.js`. The MODULARIZE version uses `createModule()` factory function.

## Optimal Settings for 2x2x2

Based on testing and mathematical properties:

- **Move Set**: `"U_U2_U-_R_R2_R-_F_F2_F-"` (URF, 9 moves)
  - URF generates a complete system for 2x2x2
  - More efficient than full 18-move set (UDLRFB)
  - Same optimal solutions, faster search
- **Prune Depth**: `1` (sufficient with URF complete system)
- **Max Length**: `11` (handles up to 11-move solutions)
- **Max Solutions**: `3` (typical usage)

## Parameters

The `solve()` method has the same signature as the standalone `solve()` function:

- `scramble` (string): Scramble algorithm (e.g., "R U R' U'")
- `rotation` (string): Cube rotation (default: "")
- `num` (number): Maximum number of solutions to find
- `len` (number): Maximum solution length
- `prune` (number): Pruning depth (typically 8)
- `moveRestrict` (string): Allowed moves (e.g., "U_U2_U-_R_R2_R-")
- `post_alg` (string): Post-algorithm (default: "")
- `ma2` (string): Move adjacency override (default: "")
- `mc` (string): Move count restrictions (default: "")

## Performance Benefits

The first `solve()` call creates the pruning table (~84MB), which takes some time. Subsequent calls reuse the same table, making them much faster:

- **First solve**: Creates all tables (slower)
- **Subsequent solves**: Reuses tables (faster)

## Implementation Details

### C++ Side (`solver.cpp`)

```cpp
struct PersistentSolver2x2 {
    search solver;  // Persistent instance (tables initialized once)
    
    void solve(/* parameters */) {
        // Uses start_search_persistent with reuse=true
        solver.start_search_persistent(/* params */, true);
    }
};
```

### Key Functions

- `search::start_search()`: Original function (always creates new table)
- `search::start_search_persistent(reuse)`: New function supporting table reuse
  - `reuse=false`: Reset and recreate prune table
  - `reuse=true`: Reuse existing table if initialized

### Table Initialization Check

The `start_search_persistent` function uses a dedicated `prune_table_initialized` flag to track whether the prune table has been created:

```cpp
struct search {
    std::vector<unsigned char> prune_table;
    bool prune_table_initialized;  // Tracks initialization status
    
    search() {
        prune_table = std::vector<unsigned char>(88179840, 255);
        prune_table_initialized = false;
    }
};
```

This approach is more reliable than checking table contents, because depending on `move_restrict_tmp`, some table positions (including `prune_table[0]`) may remain at 255 even after initialization.

## Testing

### Node.js Test
```bash
node test_simple.js
```

Expected output:
- Test 1-4 complete in ~5-6 seconds total
- Prune table created once, reused 3 times
- Test 4 (10-move scramble) solves successfully

### Browser Test (Web Worker - Recommended)
```bash
# Start HTTP server
python3 -m http.server 8000

# Open in browser
# http://localhost:8000/test_worker.html
```

Click "Run Tests" button. Worker runs in separate thread (non-blocking).

### Browser Test (Direct - Not Recommended)
Open `test_modularize.html` in a web browser.
⚠️ Warning: May freeze UI during Test 4 (10-move scramble).

## Compilation

### Single MODULARIZE Version (Universal)

**One compilation for all environments: Node.js, Browser main thread, and Web Worker.**

```bash
cd /workspaces/RubiksSolverDemo/dist/src/2x2solver

# Load Emscripten environment
source /workspaces/emsdk/emsdk_env.sh

# Compile with MODULARIZE
em++ solver.cpp -o solver.js \
  -O3 -msimd128 -flto \
  -s TOTAL_MEMORY=150MB \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind
```

Or use the provided script:
```bash
./compile.sh
```

**Flags explained:**
- `-O3 -msimd128 -flto`: Maximum optimization
- `-s TOTAL_MEMORY=150MB`: Fixed memory (sufficient for prune table ~84MB)
- `-s MODULARIZE=1`: Factory function pattern (not global Module)
- `-s EXPORT_NAME="createModule"`: Clean API (avoid global pollution)
- `--bind`: Embind C++ bindings

**Output:**
- `solver.js` (~45KB) - Module loader with `createModule()` factory
- `solver.wasm` (~195KB) - WebAssembly binary

**Universal usage patterns:**
```javascript
// Node.js (CommonJS)
const createModule = require('./solver.js');
const Module = await createModule();

// Node.js (ES6)
import createModule from './solver.js';
const Module = await createModule();

// Browser main thread
<script src="solver.js"></script>
const Module = await createModule();

// Web Worker (used by worker_persistent.js)
importScripts('solver.js');
createModule().then(Module => {
  // Use Module here
});
```

✅ **Advantage**: Single binary for all environments - no need to maintain separate builds.
```

## Comparison

| Feature | `solve()` function | `PersistentSolver2x2` |
|---------|-------------------|----------------------|
| Table lifetime | Single use | Persistent across calls |
| Memory usage | Created/destroyed each call | Created once, reused |
| First solve speed | Normal | Normal |
| Subsequent solves | Normal | **Faster** (no table rebuild) |
| Use case | Web UI (single solve) | Library (batch processing) |

## Notes

- The `PersistentSolver2x2` struct is designed for library use (Node.js, CDN)
- The standalone `solve()` function remains unchanged for web UI compatibility
- Both implementations use the same underlying solver logic
- Only `dist/src/2x2solver/` was modified (web version in `src/` unchanged)

## Critical Bug Fixes

### move_restrict Accumulation Bug (FIXED)

**Problem**: The `move_restrict` vector was **not cleared** between solves, causing it to accumulate across multiple calls. This led to:
- Exponential search space growth
- Timeouts on simple scrambles
- 10-move scramble taking minutes instead of seconds

**Solution**: Added `move_restrict.clear();` at line 690 in `start_search_persistent()`:

```cpp
void start_search_persistent(..., bool reuse = false) {
    sol.clear();            // Clear previous solutions
    move_restrict.clear();  // CRITICAL: Prevent accumulation bug
    
    // Build move_restrict from arg_restrict
    for (std::string name : restrict) {
        auto it = std::find(move_names.begin(), move_names.end(), name);
        move_restrict.emplace_back(std::distance(move_names.begin(), it));
    }
    // ...
}
```

**Impact**: Performance improved from minutes/timeout to seconds for complex scrambles.

### Browser postMessage Incompatibility (FIXED)

**Problem**: `window.postMessage(msg)` requires 2 arguments. `EM_JS` calls with 1 argument caused browser hanging.

**Solution**: Define custom `globalThis.postMessage` before loading solver:

```javascript
globalThis.postMessage = function(msg) {
    // Your handler
};
```

This overrides the default `window.postMessage` with a single-argument version compatible with EM_JS.

## Files

**Core Implementation:**
- `solver.cpp` - C++ source with PersistentSolver2x2
- `solver.js` - Compiled WASM module (MODULARIZE, universal for Node.js + Browser + Worker)
- `solver.wasm` - WebAssembly binary
- `compile.sh` - Compilation script

**Browser Usage (Recommended):**
- `worker_persistent.js` - Web Worker wrapper using MODULARIZE solver.js
- `test_worker.html` - Browser test with Web Worker (non-blocking, recommended)

**Node.js Usage:**
- `test_simple.js` - Node.js test suite using MODULARIZE solver.js

**Alternative (Not Recommended):**
- `test_modularize.html` - Browser test without Worker (blocking main thread)

**Documentation:**
- `PERSISTENT_SOLVER_README.md` - This file

✅ **All environments use the same solver.js/solver.wasm** - no separate builds needed.
