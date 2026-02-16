# Implementation Notes: 2x2x2 Persistent Solver

This document describes the C++ implementation details and modifications made to create the persistent solver. Reference this when creating persistent versions of other solvers in this repository.

## Table of Contents

1. [Overview](#overview)
2. [Key Modifications to solver.cpp](#key-modifications-to-solvercpp)
3. [Persistent Architecture](#persistent-architecture)
4. [MODULARIZE Build System](#modularize-build-system)
5. [Critical Bug Fixes](#critical-bug-fixes)
6. [Web Worker Integration](#web-worker-integration)
7. [Helper API Design](#helper-api-design)

---

## Overview

### Problem Statement

The original 2x2x2 solver (`src/2x2solver/solver.cpp`) rebuilds the pruning table (~84MB, ~3-4s) on every solve call. For applications that solve multiple scrambles sequentially (e.g., batch processing, training tools), this overhead becomes prohibitive:

- **Without persistence**: 100 solves × 3-4s = 300-400s total
- **With persistence**: 3-4s + 100×<1s = ~103-104s total (**75% faster**)

### Solution Approach

Create a **persistent solver struct** (`PersistentSolver2x2`) that maintains the `search` instance across multiple solve calls, preserving the expensive pruning table in memory.

### File Locations

- **Original source**: `/workspaces/RubiksSolverDemo/src/2x2solver/solver.cpp`
  - Web UI version (unchanged)
  - Single-use `solve()` function

- **Modified source**: `/workspaces/RubiksSolverDemo/dist/src/2x2solver/solver.cpp`
  - Library version with persistent solver
  - MODULARIZE build (universal: Node.js + Browser + Worker)

---

## Key Modifications to solver.cpp

### 1. Added Initialization Flag

**Location**: Inside `struct search` (around line 450)

```cpp
struct search {
  std::vector<unsigned char> prune_table;
  bool prune_table_initialized;  // ⬅️ NEW: Track initialization
  
  search() {
    prune_table = std::vector<unsigned char>(88179840, 255);
    prune_table_initialized = false;  // ⬅️ NEW: Initialize to false
  }
  
  // ... existing code ...
};
```

**Why?** Cannot reliably detect initialization by checking `prune_table[0]` because:
- Depending on `move_restrict_tmp`, some positions (including index 0) may remain at 255 even after valid initialization
- Need explicit flag for accurate state tracking

### 2. Created start_search_persistent() Method

**Location**: Inside `struct search` (around line 688)

```cpp
// NEW METHOD: Persistent search with optional table reuse
void start_search_persistent(
  std::string arg_scramble = "", 
  std::string arg_rotation = "", 
  int arg_sol_num = 100, 
  int arg_max_length = 12, 
  std::vector<std::string> arg_restrict = move_names, 
  int prune_depth = 8, 
  std::string arg_post_alg = "", 
  const std::vector<bool> &arg_ma2 = std::vector<bool>(27 * 27, false), 
  const std::vector<int> &arg_mc = std::vector<int>(27, 20), 
  bool reuse = false  // ⬅️ NEW: Control table reuse
) {
  sol.clear();             // ⬅️ NEW: Clear previous solutions
  move_restrict.clear();   // ⬅️ CRITICAL: Prevent accumulation bug (see below)
  
  // Copy parameters (same as original start_search)
  scramble = arg_scramble;
  rotation = arg_rotation;
  max_length = arg_max_length;
  sol_num = arg_sol_num;
  restrict = arg_restrict;
  ma2 = arg_ma2;
  mc = arg_mc;
  mc_tmp = std::vector<int>(27, 0);
  
  // Build move_restrict vector from string names
  for (std::string name : restrict) {
    auto it = std::find(move_names.begin(), move_names.end(), name);
    move_restrict.emplace_back(std::distance(move_names.begin(), it));
  }
  
  // ... scramble/rotation processing (same as original) ...
  
  // ⬅️ NEW: Conditional table creation based on reuse flag
  if (!reuse) {
    // Forced recreation: reset table and rebuild
    prune_table.assign(88179840, 255);
    create_prune_table(cp_move_table, co_move_table, prune_table, move_restrict_tmp, prune_depth);
    prune_table_initialized = true;
  } else {
    // Reuse mode: only create if not yet initialized
    if (!prune_table_initialized) {
      create_prune_table(cp_move_table, co_move_table, prune_table, move_restrict_tmp, prune_depth);
      prune_table_initialized = true;
    }
    // If already initialized: skip table creation (REUSE ✅)
  }
  
  // ... rest of search logic (same as original) ...
}
```

**Key Differences from Original `start_search()`:**

| Feature | `start_search()` | `start_search_persistent()` |
|---------|------------------|----------------------------|
| Solutions cleared | ❌ No | ✅ Yes (`sol.clear()`) |
| move_restrict cleared | ❌ No | ✅ Yes (`move_restrict.clear()`) |
| Table reuse | ❌ Always recreates | ✅ Controlled by `reuse` flag |
| Initialization tracking | ❌ No flag | ✅ Uses `prune_table_initialized` |

### 3. Created PersistentSolver2x2 Struct

**Location**: End of file (around line 987)

```cpp
// Wrapper struct for library use (maintains state across solves)
struct PersistentSolver2x2 {
  search solver;  // ⬅️ Persistent instance (tables initialized once)
  
  // Constructor (tables initialized on first solve)
  PersistentSolver2x2() {
    // search constructor already initializes move tables
    // prune_table created on first solve() call
  }
  
  // Solve method with same signature as standalone solve() function
  void solve(
    std::string scramble, 
    std::string rotation, 
    int num,              // maxSolutions
    int len,              // maxLength
    int prune,            // pruneDepth
    std::string move_restrict_string, 
    std::string post_alg, 
    std::string ma2_string, 
    std::string mcString
  ) {
    // Parse string parameters into vectors
    std::vector<bool> ma2;
    std::vector<int> mc;
    std::vector<std::string> move_restrict;
    buidMoveRestrict(move_restrict_string, move_restrict);
    buidMA2(move_restrict_string, ma2_string, ma2);
    buildMoveCountVector(move_restrict_string, mcString, mc);
    
    // ⬅️ KEY: Call start_search_persistent with reuse=true
    solver.start_search_persistent(
      scramble, rotation, num, len, 
      move_restrict, prune, post_alg, 
      ma2, mc, 
      true  // ⬅️ ALWAYS reuse table
    );
  }
};
```

**Design Rationale:**
- Wraps a `search` instance as a member variable
- Each `PersistentSolver2x2` instance maintains its own pruning table
- Calling `solve()` multiple times on same instance reuses the table
- Destroying the instance frees the table (~84MB)

### 4. Emscripten Bindings

**Location**: End of file (around line 1015)

```cpp
EMSCRIPTEN_BINDINGS(my_module) {
  // Export standalone solve function (original API, for compatibility)
  emscripten::function("solve", &solve);
  
  // ⬅️ NEW: Export persistent solver for library use
  emscripten::class_<PersistentSolver2x2>("PersistentSolver2x2")
    .constructor<>()
    .function("solve", &PersistentSolver2x2::solve);
}
```

**Usage in JavaScript:**

```javascript
const Module = await createModule();

// Option 1: Standalone (original API, no persistence)
Module.solve(scramble, rotation, num, len, prune, moves, ...);

// Option 2: Persistent (library API, table reuse)
const solver = new Module.PersistentSolver2x2();
solver.solve(scramble, rotation, num, len, prune, moves, ...);
solver.solve(scramble2, ...);  // ✅ Reuses table
```

---

## Persistent Architecture

### Memory Lifecycle

```
┌─────────────────────────────────────────────────────────┐
│ new PersistentSolver2x2()                               │
│   └─> search constructor()                              │
│         └─> Initialize move tables (~small, fast)       │
│                                                          │
│ First solve():                                          │
│   └─> start_search_persistent(..., reuse=true)         │
│         └─> prune_table_initialized == false            │
│               └─> create_prune_table() [~3-4s, 84MB]   │
│               └─> prune_table_initialized = true        │
│                                                          │
│ Second solve():                                         │
│   └─> start_search_persistent(..., reuse=true)         │
│         └─> prune_table_initialized == true             │
│               └─> SKIP table creation [<1s] ✅         │
│                                                          │
│ ... (subsequent solves reuse table)                     │
│                                                          │
│ delete solver or instance destroyed:                    │
│   └─> ~PersistentSolver2x2()                           │
│         └─> Free prune_table memory (~84MB)            │
└─────────────────────────────────────────────────────────┘
```

### State Management

```cpp
struct search {
  // Move tables (created once in constructor, ~small)
  std::vector<State> cp_move_table;
  std::vector<std::vector<int>> co_move_table;
  std::vector<std::vector<int>> center_move_table;
  
  // Pruning table (created on first solve, ~84MB)
  std::vector<unsigned char> prune_table;
  bool prune_table_initialized;  // Tracks creation
  
  // Per-solve state (reset each solve)
  std::vector<std::string> sol;        // Solutions found
  std::vector<int> move_restrict;      // Allowed moves
  int sol_num;                          // Max solutions
  int max_length;                       // Max length
  // ... etc
};
```

**Lifetime categories:**
1. **Permanent** (constructor → destructor):
   - Move tables (`cp_move_table`, `co_move_table`, `center_move_table`)
   - Pruning table (`prune_table`) - once created

2. **Per-solve** (cleared/reset each solve):
   - Solutions (`sol`)
   - Move restrictions (`move_restrict`)
   - Search parameters (`sol_num`, `max_length`, etc.)

---

## MODULARIZE Build System

### Compilation Command

```bash
em++ solver.cpp -o solver.js \
  -O3 \                         # Maximum optimization
  -msimd128 \                   # SIMD vectorization
  -flto \                       # Link-time optimization
  -s TOTAL_MEMORY=150MB \       # Fixed memory (sufficient for 84MB table)
  -s WASM=1 \                   # Enable WebAssembly
  -s MODULARIZE=1 \             # ⬅️ Factory function pattern
  -s EXPORT_NAME="createModule" \  # ⬅️ Custom factory name
  --bind                        # Embind C++ bindings
```

### What MODULARIZE Does

**Without MODULARIZE** (old approach):
```javascript
// solver.js creates global Module automatically
<script src="solver.js"></script>
<script>
  Module.then(m => {
    // Use m here
  });
</script>
```

**With MODULARIZE** (current approach):
```javascript
// solver.js exports createModule() factory
const createModule = require('./solver.js');  // Node.js
// OR
import createModule from './solver.js';       // ES6
// OR
<script src="solver.js"></script>             // Browser global

const Module = await createModule();
```

**Benefits:**
1. **No global pollution** - `Module` not on window
2. **Multiple instances** - Can create independent modules
3. **Universal compatibility** - Works in Node.js, Browser, Worker
4. **Clean API** - Explicit factory invocation

### Usage Patterns

**Node.js (CommonJS):**
```javascript
const createModule = require('./solver.js');

(async () => {
  const Module = await createModule();
  const solver = new Module.PersistentSolver2x2();
  // ...
})();
```

**Node.js (ES6):**
```javascript
import createModule from './solver.js';

const Module = await createModule();
const solver = new Module.PersistentSolver2x2();
```

**Browser (main thread):**
```html
<script src="solver.js"></script>
<script>
  createModule().then(Module => {
    const solver = new Module.PersistentSolver2x2();
    // ...
  });
</script>
```

**Web Worker:**
```javascript
importScripts('solver.js');

createModule().then(Module => {
  const solver = new Module.PersistentSolver2x2();
  // ...
});
```

---

## Critical Bug Fixes

### Bug 1: move_restrict Accumulation

**Discovery:** Test 4 (10-move scramble) timing out after running Tests 1-3.

**Symptom:**
```
Test 1: 1.2s ✅
Test 2: 0.8s ✅
Test 3: 0.9s ✅
Test 4: [hangs for minutes...] ❌
```

**Root Cause:**

Original `start_search()` code (line 635):
```cpp
void start_search(...) {
  // ❌ BUG: move_restrict NOT cleared
  
  for (std::string name : restrict) {
    auto it = std::find(move_names.begin(), move_names.end(), name);
    move_restrict.emplace_back(std::distance(move_names.begin(), it));
  }
  // Move_restrict keeps accumulating across calls!
}
```

**Behavior:**
- Solve 1: `move_restrict = [0,1,2,3,4,5,6,7,8]` (9 URF moves)
- Solve 2: `move_restrict = [0,1,2,3,4,5,6,7,8, 0,1,2,3,4,5,6,7,8]` (18 moves, duplicates)
- Solve 3: `move_restrict = [0,1,2,3,4,5,6,7,8, ..., ...]` (27 moves)
- Solve 4: `move_restrict = [0,1,2,3,4,5,6,7,8, ..., ..., ...]` (36 moves)

**Impact:** Exponential search space growth → timeout

**Fix:** Add `move_restrict.clear()` at start of function:
```cpp
void start_search_persistent(...) {
  sol.clear();             // ✅ Clear solutions
  move_restrict.clear();   // ✅ CRITICAL FIX
  
  // Now rebuild from scratch each time
  for (std::string name : restrict) {
    move_restrict.emplace_back(...);
  }
}
```

**Location**: `dist/src/2x2solver/solver.cpp` line 690

**After Fix:**
```
Test 1: 1.2s ✅
Test 2: 0.8s ✅
Test 3: 0.9s ✅
Test 4: 1.5s ✅  [FIXED!]
```

### Bug 2: Prune Table Reuse Detection

**Problem:** Cannot reliably detect initialization by checking `prune_table[0] == 255`.

**Why?** Prune table layout:
```
index = coord3to2(cp, co)  // 0 to 88179839

If move_restrict_tmp limits positions:
  - Some indices never visited during create_prune_table()
  - Those indices remain at 255 (uninitialized sentinel)
  - Including potentially index 0!
```

**Example scenario:**
```cpp
// URF moves with certain rotations
move_restrict_tmp = [...];
create_prune_table(...);
// Result: prune_table[0] might still be 255 even though table is valid
```

**Bad approach:**
```cpp
// ❌ UNRELIABLE
if (prune_table[0] == 255) {
  // Not necessarily uninitialized!
}
```

**Fix:** Explicit initialization flag:
```cpp
struct search {
  bool prune_table_initialized;  // ✅ Explicit state
  
  search() {
    prune_table_initialized = false;
  }
};

void start_search_persistent(..., bool reuse) {
  if (!reuse) {
    create_prune_table(...);
    prune_table_initialized = true;  // ✅ Set flag
  } else if (!prune_table_initialized) {
    create_prune_table(...);
    prune_table_initialized = true;  // ✅ Set flag
  }
  // Else: skip (table already valid)
}
```

**Location**: Lines 452 (flag declaration), 720-730 (usage)

### Bug 3: postMessage Signature Mismatch

**Platform-specific issue:**

**Node.js**: Works fine
```javascript
globalThis.postMessage = function(msg) {
  console.log(msg);  // 1 argument ✅
};
```

**Browser**: Hangs/freezes
```javascript
// window.postMessage requires 2 arguments: (message, targetOrigin)
window.postMessage(msg);  // ❌ TypeError or silent failure
```

**Emscripten EM_JS code** (line 11):
```cpp
EM_JS(void, update, (const char *str), {
  postMessage(UTF8ToString(str));  // ⬅️ 1 argument
});
```

**Fix:** Override `globalThis.postMessage` before loading:
```javascript
// Provide 1-argument version (overrides window.postMessage)
globalThis.postMessage = function(msg) {
  // Custom handler (console.log, or send to Worker)
};

// Then load module
const Module = await createModule();
```

**In Web Worker** (`worker_persistent.js`):
```javascript
let currentPostMessageHandler = (msg) => {
  postMessage({ type: 'solution', data: msg });  // Send to main thread
};

globalThis.postMessage = function(msg) {
  currentPostMessageHandler(msg);
};

// Then load
importScripts('solver.js');
createModule().then(Module => {
  // Use Module
});
```

**Location:** Not in solver.cpp (handled in JavaScript wrapper layer)

---

## Web Worker Integration

### Worker Architecture

```
┌──────────────────────────────────────────────────────────┐
│ Main Thread (HTML page)                                  │
│   └─> worker = new Worker('worker_persistent.js')       │
│   └─> worker.postMessage({ scramble: "R U R' U'" })     │
│                                                          │
│        ┌─ - - - - - - - - - - - - - - - - - - - - ┐    │
│         Message                                          │
│        └─ - - - - - - - - - - - - - - - - - - - - ┘    │
│                          │                               │
│                          ▼                               │
├──────────────────────────────────────────────────────────┤
│ Worker Thread (worker_persistent.js)                    │
│   1. importScripts('solver.js')                         │
│   2. createModule() → Module                            │
│   3. solver = new Module.PersistentSolver2x2()          │
│   4. On message:                                        │
│        solver.solve(scramble, ...)                      │
│        └─> C++ postMessage(solution)                    │
│             └─> globalThis.postMessage(solution)        │
│                  └─> postMessage({ type: 'solution' })  │
│                                                          │
│        ┌─ - - - - - - - - - - - - - - - - - - - - ┐    │
│         Solution                                         │
│        └─ - - - - - - - - - - - - - - - - - - - - ┘    │
│                          │                               │
│                          ▼                               │
├──────────────────────────────────────────────────────────┤
│ Main Thread                                              │
│   └─> worker.onmessage = (e) => {                       │
│          console.log(e.data.data);  // Solution string  │
│       }                                                  │
└──────────────────────────────────────────────────────────┘
```

### worker_persistent.js Implementation

**Key sections:**

1. **postMessage Override:**
```javascript
let currentPostMessageHandler = (msg) => {
  if (msg.startsWith('depth=')) {
    postMessage({ type: 'depth', data: msg });
  } else if (msg === 'Search finished.') {
    postMessage({ type: 'done' });
  } else if (msg !== '') {
    postMessage({ type: 'solution', data: msg });
  }
};

globalThis.postMessage = function(msg) {
  currentPostMessageHandler(msg);
};
```

2. **WASM Module Loading:**
```javascript
// Calculate base URL for solver.js/solver.wasm
const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);

importScripts(baseURL + 'solver.js');

createModule().then(Module => {
  solver = new Module.PersistentSolver2x2();
  postMessage({ type: 'ready' });
});
```

3. **Message Handler:**
```javascript
self.onmessage = function(event) {
  const params = event.data;
  
  try {
    solver.solve(
      params.scramble || '',
      params.rotation || '',
      params.maxSolutions || 3,
      params.maxLength || 11,
      params.pruneDepth || 1,
      params.allowedMoves || 'U_U2_U-_R_R2_R-_F_F2_F-',
      params.preMove || '',
      params.moveOrder || '',
      params.moveCount || ''
    );
  } catch (err) {
    postMessage({ type: 'error', data: err.toString() });
  }
};
```

**Location:** `dist/src/2x2solver/worker_persistent.js`

---

## Helper API Design

### Architecture

```
┌────────────────────────────────────────────────┐
│ User Code                                      │
│   const helper = new Solver2x2Helper();        │
│   await helper.init();                         │
│   const solutions = await helper.solve(...);   │
└────────────────────────────────────────────────┘
                     │
                     ▼
┌────────────────────────────────────────────────┐
│ solver-helper.js (Promise wrapper)             │
│   - Creates Worker                             │
│   - Manages onmessage callbacks                │
│   - Wraps in Promises                          │
│   - CDN auto-detection                         │
│   - Cache busting propagation                  │
└────────────────────────────────────────────────┘
                     │
                     ▼
┌────────────────────────────────────────────────┐
│ worker_persistent.js (Web Worker)              │
│   - Loads WASM module                          │
│   - Maintains PersistentSolver2x2              │
│   - Handles solve requests                     │
└────────────────────────────────────────────────┘
                     │
                     ▼
┌────────────────────────────────────────────────┐
│ solver.wasm (PersistentSolver2x2)              │
│   - C++ solver logic                           │
│   - Pruning table persistence                  │
└────────────────────────────────────────────────┘
```

### Promise Wrapper Pattern

**Problem:** Worker API is callback-based:
```javascript
// ❌ Callback hell
worker.onmessage = (e) => {
  if (e.data.type === 'solution') {
    solutions.push(e.data.data);
  } else if (e.data.type === 'done') {
    // Now what? How do we return solutions?
  }
};
worker.postMessage({ scramble: "R U R' U'" });
```

**Solution:** Wrap in Promise:
```javascript
class Solver2x2Helper {
  solve(scramble, options = {}) {
    return new Promise((resolve, reject) => {
      this.currentResolve = resolve;
      this.currentReject = reject;
      this.currentSolutions = [];
      this.currentProgressCallback = options.onProgress;
      
      this.worker.postMessage({ scramble, ...options });
    });
  }
  
  _handleMessage(event) {
    const msg = event.data;
    
    if (msg.type === 'solution') {
      this.currentSolutions.push(msg.data);
      if (this.currentSolutions && this.onSolution) {
        this.onSolution(msg.data);
      }
    } else if (msg.type === 'done') {
      this.currentResolve(this.currentSolutions);  // ✅ Resolve Promise
    } else if (msg.type === 'error') {
      this.currentReject(new Error(msg.data));     // ✅ Reject Promise
    } else if (msg.type === 'depth') {
      if (this.currentProgressCallback) {
        this.currentProgressCallback(msg.data);
      }
    }
  }
}
```

**Result:** Clean async/await usage:
```javascript
// ✅ Clean async/await
const solutions = await helper.solve("R U R' U'");
console.log(solutions);
```

**Location:** `dist/src/2x2solver/solver-helper.js`

### CDN Auto-Detection

**Problem:** Worker path depends on how helper was loaded:
```javascript
// Local: worker path = './worker_persistent.js'
<script src="solver-helper.js"></script>

// CDN: worker path = 'https://cdn.../worker_persistent.js'
<script src="https://cdn.../solver-helper.js"></script>
```

**Solution:** Auto-detect from script source:
```javascript
constructor(workerPath = null) {
  if (!workerPath) {
    let scriptUrl = null;
    
    // Method 1: document.currentScript (static <script src>)
    if (document.currentScript && document.currentScript.src) {
      scriptUrl = document.currentScript.src;
    }
    // Method 2: Search DOM (dynamic createElement)
    else if (typeof document !== 'undefined') {
      const scripts = document.getElementsByTagName('script');
      for (let i = 0; i < scripts.length; i++) {
        if (scripts[i].src && scripts[i].src.includes('solver-helper.js')) {
          scriptUrl = scripts[i].src;
          break;
        }
      }
    }
    
    if (scriptUrl) {
      // Extract base URL
      const baseUrl = scriptUrl.substring(0, scriptUrl.lastIndexOf('/') + 1);
      
      // Extract query params (cache busting)
      const queryParams = scriptUrl.includes('?') ? ... : '';
      
      // Construct worker path
      this.workerPath = baseUrl + 'worker_persistent.js' + queryParams;
    }
  }
}
```

**Benefits:**
- ✅ Works with local files
- ✅ Works with CDN (static `<script src>`)
- ✅ Works with CDN (dynamic `createElement`)
- ✅ Propagates cache busting params (`?v=`)

**Location:** `dist/src/2x2solver/solver-helper.js` lines 29-70

---

## Summary: What Changed?

### C++ Changes (solver.cpp)

1. **Added `prune_table_initialized` flag** - Reliable state tracking
2. **Created `start_search_persistent()` method** - Table reuse logic
3. **Created `PersistentSolver2x2` struct** - Public API for persistence
4. **Fixed `move_restrict` accumulation bug** - Clear vector each solve
5. **Added Emscripten bindings** - Export `PersistentSolver2x2` class

### Build System

1. **MODULARIZE compilation** - Universal binary (Node + Browser + Worker)
2. **Factory function pattern** - `createModule()` instead of global `Module`
3. **Optimized flags** - `-O3 -msimd128 -flto` for maximum performance

### JavaScript Wrapper

1. **worker_persistent.js** - Web Worker wrapper for MODULARIZE
2. **solver-helper.js** - Promise-based API (browser)
3. **solver-helper-node.js** - Promise-based API (Node.js)
4. **CDN support** - Auto-detection + Blob URL for CORS bypass
5. **Cache busting** - Propagation from helper → worker → wasm

### Documentation

1. **README.md** - User guide + API reference
2. **IMPLEMENTATION_NOTES.md** - This file (C++ implementation)
3. **TROUBLESHOOTING.md** - Common issues + debugging
4. **RELEASE_GUIDE.md** - Release process + versioning

---

## Reference for Future Solvers

When creating persistent versions of other solvers (cross, eocross, pairing, etc.):

### Checklist

- [ ] Add `prune_table_initialized` or equivalent flag
- [ ] Create `start_search_persistent()` method with `reuse` parameter
- [ ] **CRITICAL**: Clear all accumulating vectors (like `move_restrict`)
- [ ] Create `PersistentSolverXXX` struct wrapper
- [ ] Add Emscripten bindings for the struct
- [ ] Use MODULARIZE build
- [ ] Create `worker_persistent.js` wrapper
- [ ] Create `solver-helper.js` Promise wrapper
- [ ] Override `globalThis.postMessage` in Worker
- [ ] Test with demo.html (local + CDN)
- [ ] Test batch solving (verify table reuse)

### Common Pitfalls

1. **Forgetting to clear vectors** → Accumulation bug
2. **Using table contents for init check** → Unreliable
3. **Missing postMessage override** → Browser hang
4. **Not propagating cache bust** → CDN serves old files
5. **Worker path assumptions** → 404 errors

### Performance Validation

```javascript
// Test persistence benefit
const helper = new Solver2x2Helper();
await helper.init();

console.time('First solve');
await helper.solve("R U R' U'");
console.timeEnd('First solve');  // ~3-4s

console.time('Second solve');
await helper.solve("R2 U2");
console.timeEnd('Second solve');  // Should be <1s ✅
```

If second solve is still slow → table not being reused → check implementation.

---

## Version History

- **v1.0.0** (2026-02-16): Initial release with persistent solver, CDN auto-detection, and cache busting support

See [README.md](README.md) for user-facing changes.
