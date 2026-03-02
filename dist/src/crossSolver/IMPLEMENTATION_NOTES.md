# Implementation Notes: crossSolver Persistent Solver

This document describes the C++ architecture, key design decisions, and
modifications behind `dist/src/crossSolver/solver.cpp`.

It is the primary reference for all future solver additions that will be based on this file (EOCross, F2L pairs, etc.). When porting or extending the solver, read this document first.

## Table of Contents

1. [Overview](#overview)
2. [Solver Structs](#solver-structs)
3. [Global Move Tables](#global-move-tables)
4. [Persistent Architecture](#persistent-architecture)
5. [Prune Table Design](#prune-table-design)
6. [BFS Optimization: Removing ASYNCIFY from create_prune_table](#bfs-optimization-removing-asyncify-from-create_prune_table)
7. [Cancel Support](#cancel-support)
8. [LLSubsteps `ll` Parameter](#llsubsteps-ll-parameter)
9. [Emscripten Bindings](#emscripten-bindings)
10. [Memory Sizing and ALLOW_MEMORY_GROWTH](#memory-sizing-and-allow_memory_growth)
11. [MODULARIZE Build System](#modularize-build-system)
12. [Web Worker Integration](#web-worker-integration)
13. [Helper APIs](#helper-apis)

---

## Overview

### Problem Statement

Each solver (Cross, Xcross, …, LLAUF) must build one or more BFS-based pruning
tables before it can search. These tables are large and slow to build:

| Solver | Prune table(s) | Approximate size | Build time (Node.js WASM) |
|---|---|---|---|
| Cross | 1 | ~0.3 MB | ~50 ms |
| Xcross | 1 | ~5.5 MB | ~870 ms |
| Xxcross | 2 | ~11 MB | ~2–4 s |
| Xxxcross | 3 | ~22 MB | ~5–8 s |
| Xxxxcross / LL / LLAUF | 4 | ~22 MB | ~7–32 s |

For training-tool applications that solve many scrambles in a session, rebuilding
the table per solve call is prohibitive. The persistent solver keeps each table
alive across calls.

### Solution

Each C++ `*_search` struct owns its pruning table as a member variable
(`std::vector<unsigned char> prune_table`).  An explicit `bool
prune_table_initialized` flag guards first-time construction.
`start_search_persistent()` builds the table only when the flag is false; all
subsequent calls skip the BFS and go straight to IDA*-search.

---

## Solver Structs

There are eight solver structs, each implementing the same pattern:

| C++ struct | Persistent wrapper | Exported JS class | Additional constructor args |
|---|---|---|---|
| `cross_search` | `PersistentCrossSolver` | `PersistentCrossSolver` | — |
| `xcross_search` | `PersistentXcrossSolver` | `PersistentXcrossSolver` | `slot` (int, 0-3) |
| `xxcross_search` | `PersistentXxcrossSolver` | `PersistentXxcrossSolver` | `slot1`, `slot2` |
| `xxxcross_search` | `PersistentXxxcrossSolver` | `PersistentXxxcrossSolver` | `slot1`, `slot2`, `slot3` |
| `xxxxcross_search` | `PersistentXxxxcrossSolver` | `PersistentXxxxcrossSolver` | — |
| `LL_substeps_search` | `PersistentLLSubstepsSolver` | `PersistentLLSubstepsSolver` | — |
| `LL_search` | `PersistentLLSolver` | `PersistentLLSolver` | — |
| `LL_AUF_search` | `PersistentLLAUFSolver` | `PersistentLLAUFSolver` | — |

All eight struct definitions follow at the bottom of `solver.cpp`
(lines 6781–6960 in the current source). Each wrapper:

1. Holds a `*_search` member as `search`.
2. Stores slot indices (if applicable) as member fields.
3. In `solve()`: calls `solver_clear_cancel()`, parses string parameters via
   helper functions (`buidMoveRestrict`, `buildCenterOffset`, etc.), then
   delegates to `search.start_search_persistent(…)`.

```cpp
// Example: PersistentXcrossSolver
struct PersistentXcrossSolver {
    xcross_search search;
    int slot;
    PersistentXcrossSolver(int slot_) : slot(slot_) {}

    void solve(std::string scr, std::string rot,
               int num, int len,
               std::string move_restrict_string, std::string post_alg,
               std::string center_offset_string, int max_rot_count,
               std::string ma2_string, std::string mcString) {
        solver_clear_cancel();
        // ... parameter parsing ...
        search.start_search_persistent(scr, rot, slot, num, len,
                                       move_restrict, post_alg,
                                       center_offset, max_rot_count, ma2, mc);
    }
};
```

---

## Global Move Tables

Unlike the 2x2 solver where all move tables live inside a single `struct search`,
the crossSolver maintains several **global** move tables that are shared across
all search structs:

```cpp
// Defined at global scope (outside any struct)
std::vector<State>              g_cross_multi_move_table;
std::vector<std::vector<int>>   g_corner_move_table;
std::vector<std::vector<int>>   g_xcross_multi_move_table;
std::vector<std::vector<int>>   g_center_move_table;
// ... etc.
```

These are initialized once at module load time (in static initializers or on
first access). Individual `*_search` structs reference them by pointer/index
rather than owning copies. **This is a memory optimization**: the cross edge
move table is the same for both cross and xcross solvers; storing it twice would
be wasteful.

**Implication for future solvers:** Any new *_search struct should reuse these
global tables where possible. New structs should only own state that is truly
per-instance (prune table, `prune_table_initialized` flag, solution list, etc.).

---

## Persistent Architecture

### Initialization flag

Every `*_search` struct that participates in the persistent design has:

```cpp
std::vector<unsigned char> prune_table;
bool prune_table_initialized;

// In constructor:
prune_table_initialized = false;
// prune_table allocation is deferred to start_search_persistent()
```

The allocation of `prune_table` is **deferred** (not done in the constructor) so
that JavaScript code can create many solver instances cheaply. Memory is only
committed when the first `solve()` call fires.

### start_search_persistent()

Each `*_search` struct has a `start_search_persistent()` method alongside the
original `start_search()`. The persistent variant:

1. Clears per-solve state (`sol.clear()`, `move_restrict.clear()`, etc.).
2. Resets the global cancel-check counter (`g_cancel_check_counter = 0`).
3. **Builds the prune table only if not yet initialized:**

```cpp
if (!prune_table_initialized) {
    prune_table.assign(table_size, 255);  // allocate + set sentinel value
    solver_yield();                        // yield before BFS starts
    if (solver_is_cancelled()) { update("Search cancelled."); return; }
    create_prune_table(/* args */);        // BFS (may take seconds)
    if (!solver_is_cancelled()) {
        prune_table_initialized = true;    // mark ready only on success
    } else {
        update("Search cancelled."); return;
    }
}
// Check cancel one more time before entering IDA* d-loop
if (solver_is_cancelled()) { update("Search cancelled."); return; }
```

4. Runs the IDA* depth-limited search d-loop.

### Memory lifecycle

```
new PersistentXcrossSolver(0)
  └─> xcross_search constructor
        └─> prune_table = [] (empty, NOT yet allocated)
        └─> prune_table_initialized = false

First solve(scr, ...)
  └─> start_search_persistent(...)
        └─> prune_table_initialized == false
              └─> prune_table.assign(size, 255)  [allocate ~5.5 MB]
              └─> create_prune_table(...)         [BFS, ~870 ms]
              └─> prune_table_initialized = true

Second solve(scr2, ...)
  └─> start_search_persistent(...)
        └─> prune_table_initialized == true
              └─> SKIP BFS entirely             [<1 ms] ✅

// When JS GC collects the instance:
~xcross_search()
  └─> prune_table freed (~5.5 MB returned to heap)
```

---

## Prune Table Design

### BFS overview

Each `create_prune_table()` call performs a BFS over the coordinate space defined
by two indices (e.g., `index1` for edge positions, `index2` for corner
positions). It fills `prune_table` with the minimum moves-to-solved distance for
each `(index1, index2)` pair.

The function signature is:

```cpp
void create_prune_table(
    int index1, int index2,          // starting coordinates
    int size1, int size2,            // coordinate space sizes
    int depth,                       // max BFS depth
    const std::vector<int> &table1,
    const std::vector<int> &table2,
    std::vector<unsigned char> &prune_table,
    std::vector<int> &move_restrict,
    std::vector<unsigned char> &tmp_array,
    std::vector<std::vector<int>> &center_move_table
);
```

### Prune table sizes

| Solver | size1 × size2 | Total entries |
|---|---|---|
| Cross | 24×22 × 24×22 | ~279,936 |
| Xcross | 24×22×20×18 × 24 | ~4,561,920 |
| Xxcross | 24×22×20×18 × 24 | ~4,561,920 (×2) |
| Xxxcross | (similar scale) × 3 | |
| Xxxxcross / LL / LLAUF | (similar scale) × 4 | |

---

## BFS Optimization: Removing ASYNCIFY from create_prune_table

This section documents an important performance investigation and the resulting
optimization.

### Root cause of slow BFS in WASM

In an early version, `create_prune_table()` contained a `solver_yield()` call
inside the outer depth loop of the BFS and `solver_is_cancelled()` checks inside
the inner iteration loop. Both of these are Emscripten-provided functions that
cross the C++/JS boundary:

- `solver_yield()` — `EM_ASYNC_JS`: suspends the WASM stack and resumes after `setTimeout(0)`.
- `solver_is_cancelled()` — `EM_JS`: synchronous flag read.

When `solver_yield()` (an `EM_ASYNC_JS` function) is called inside a function,
**Emscripten's ASYNCIFY instrumenter must rewrite that entire call chain** to be
suspendable/restorable. For `create_prune_table()`, this meant the *entire
function body* — including the inner 4.5-million-iteration i-loop — was
instrumented with ASYNCIFY save/restore logic. The result was ~10–20× slowdown
of the BFS compared to the browser (where ASYNCIFY overhead is lower due to
different execution models).

Additionally, `solver_is_cancelled()` in the inner loop prevented the compiler
from auto-vectorizing the loop body.

### Fix: move yield/cancel to the caller

The solution is to **never call `solver_yield()` inside `create_prune_table()`**.
Instead, every `start_search_persistent()` method does:

```cpp
// 1. Yield before BFS — lets a cancel message arrive if pending
solver_yield();
if (solver_is_cancelled()) { update("Search cancelled."); return; }

// 2. Run BFS — no yield or cancel inside; ASYNCIFY does NOT instrument this
create_prune_table(/* ... */);

// 3. Check cancel after BFS
if (!solver_is_cancelled()) {
    prune_table_initialized = true;
} else {
    update("Search cancelled."); return;
}
```

With this layout, `create_prune_table()` never appears on any `solver_yield()`
call stack, so ASYNCIFY leaves it uninstrumented. The inner loop is free to be
vectorized. BFS speed returns to browser-equivalent levels.

**Cancel granularity during BFS:** The BFS itself is now uninterruptible. In
practice, each depth-0 BFS for the solvers used here takes well under 1 second,
so the single pre-BFS yield provides sufficient responsiveness. If a future
prune table requires multiple-second BFS per depth, additional per-depth yields
can be inserted between `create_prune_table` calls (or after each BFS pass) at
the `start_search_persistent` level without affecting ASYNCIFY instrumentation of
the inner loop.

---

## Cancel Support

### JS functions

```cpp
// Suspend WASM execution for one JS event-loop tick (requires ASYNCIFY)
EM_ASYNC_JS(void, solver_yield, (), {
    await new Promise(function(resolve) { setTimeout(resolve, 0); });
});

// Synchronous flag read (no suspension)
EM_JS(int, solver_is_cancelled, (), {
    return (Module._cancelRequested === true) ? 1 : 0;
});

// Reset flag at start of each solve
EM_JS(void, solver_clear_cancel, (), {
    Module._cancelRequested = false;
});
```

`solver_clear_cancel()` is called at the very beginning of every
`PersistentXxxSolver::solve()`, ensuring a leftover cancel from a previous
aborted solve never bleeds into the next call.

### Cancel-check counter

```cpp
// File-scope globals
static int g_cancel_check_counter = 0;
static int g_cancel_check_mask    = 0x7FFF;  // check every 32768 nodes
```

Inside `depth_limited_search()`:

```cpp
if ((++g_cancel_check_counter & g_cancel_check_mask) == 0) {
    if (solver_is_cancelled()) return true;
}
```

This avoids the overhead of an EM_JS call on every node. The mask can be tuned via:

```cpp
// Exposed to JS:
void setCancelCheckMask(int mask);
// Module.setCancelCheckMask(0x7FFFFFFF);  // effectively disable (benchmark use)
```

The `g_cancel_check_counter` is reset to 0 at the start of each
`start_search_persistent()` call.

### Cancel in the d-loop (IDA* depth iteration)

Each `start_search_persistent()` also yields once per IDA* depth level:

```cpp
for (int d = 0; d <= max_length; d++) {
    g_cancel_check_counter = 0;
    solver_yield();
    if (solver_is_cancelled()) { update("Search cancelled."); return; }
    // ... run depth_limited_search for depth d ...
}
```

This means the fastest cancel response (while searching) is one depth iteration
plus up to 32768 nodes.

### Cancel flow summary

```
JS: helper.cancel()
  └─> worker.postMessage({ type: 'cancel' })
      └─> wasmModule._cancelRequested = true

C++ (running on next iteration):
  depth_limited_search():
    └─> (++counter & mask) == 0
          └─> solver_is_cancelled() == true
                └─> return true  (propagates up to d-loop)

  d-loop in start_search_persistent():
    └─> solver_yield()  →  JS event loop tick
    └─> solver_is_cancelled() == true
          └─> update("Search cancelled.")
          └─> return

JS:
  globalThis.postMessage("Search cancelled.")
  └─> worker sends { type: 'cancelled' }
  └─> helper resolves Promise with partial solutions
  └─> options.onCancel(partial) called
```

If cancel fires during the pre-BFS yield, the BFS never runs. If it fires during
the BFS, the BFS completes uninterrupted (by design—see BFS optimization above),
then the post-BFS cancel check fires.

---

## LLSubsteps `ll` Parameter

### C++ side

`LL_substeps_option_array()` parses a space-separated token string into a
`std::vector<bool>` of length 4:

```
Index 0 → CP (Corner Permutation)
Index 1 → CO (Corner Orientation)
Index 2 → EP (Edge Permutation)
Index 3 → EO (Edge Orientation)
```

Valid tokens: `CP`, `CO`, `EP`, `EO` (case-sensitive).
Any other token is silently ignored.

**Default `''` (empty string):** all four flags `false` → no LL conditions
enforced. The solver accepts any LL state as a solution endpoint. This is
functionally equivalent to solving xxxxcross.

**All four `true`:** does NOT guarantee a fully solved LL, because individual
AUF moves are still allowed per piece group (corners and edges can have
independent AUF → H-perm accepted).

```cpp
// Called inside PersistentLLSubstepsSolver::solve():
std::vector<bool> option_list = LL_substeps_option_array(ll);
search.start_search_persistent(
    scr, option_list[0], option_list[1], option_list[2], option_list[3],
    rot, num, len, ...
);
```

### JS side

The `solver-helper-node.js` and `solver-helper.js` helpers expose the `ll`
parameter as `string | string[]`. Internally, `_llStr()` normalizes it:

```javascript
_llStr(ll) {
    if (!ll) return '';
    if (Array.isArray(ll)) return ll.join(' ');
    return String(ll);
}
// ['CO', 'EO'] → 'CO EO' → C++ LL_substeps_option_array('CO EO')
//                         → [false, true, false, true]
```

`worker-persistent.js` applies the same conversion before forwarding to the C++
`PersistentLLSubstepsSolver::solve()`.

---

## Emscripten Bindings

```cpp
EMSCRIPTEN_BINDINGS(my_module) {
    // Legacy standalone solve (non-persistent, original API)
    emscripten::function("solve", &controller);

    // Tune cancel-check frequency (benchmark / testing)
    emscripten::function("setCancelCheckMask", &setCancelCheckMask);

    // Persistent solver classes
    emscripten::class_<PersistentCrossSolver>("PersistentCrossSolver")
        .constructor<>()
        .function("solve", &PersistentCrossSolver::solve);

    emscripten::class_<PersistentXcrossSolver>("PersistentXcrossSolver")
        .constructor<int>()             // slot
        .function("solve", &PersistentXcrossSolver::solve);

    emscripten::class_<PersistentXxcrossSolver>("PersistentXxcrossSolver")
        .constructor<int, int>()        // slot1, slot2
        .function("solve", &PersistentXxcrossSolver::solve);

    emscripten::class_<PersistentXxxcrossSolver>("PersistentXxxcrossSolver")
        .constructor<int, int, int>()   // slot1, slot2, slot3
        .function("solve", &PersistentXxxcrossSolver::solve);

    emscripten::class_<PersistentXxxxcrossSolver>("PersistentXxxxcrossSolver")
        .constructor<>()
        .function("solve", &PersistentXxxxcrossSolver::solve);

    emscripten::class_<PersistentLLSubstepsSolver>("PersistentLLSubstepsSolver")
        .constructor<>()
        .function("solve", &PersistentLLSubstepsSolver::solve);

    emscripten::class_<PersistentLLSolver>("PersistentLLSolver")
        .constructor<>()
        .function("solve", &PersistentLLSolver::solve);

    emscripten::class_<PersistentLLAUFSolver>("PersistentLLAUFSolver")
        .constructor<>()
        .function("solve", &PersistentLLAUFSolver::solve);
}
```

### JavaScript usage

```javascript
const Module = await createModule();

// Create instances
const cross   = new Module.PersistentCrossSolver();
const xcross0 = new Module.PersistentXcrossSolver(0);  // slot=0 (BR)
const xxcross = new Module.PersistentXxcrossSolver(0, 3);
const ll      = new Module.PersistentLLSolver();

// solve() signature (all except LLSubsteps):
// solve(scr, rot, num, len, move_restrict_string, post_alg,
//        center_offset_string, max_rot_count, ma2_string, mcString)
cross.solve("R U R' U'", "", 3, 8,
    "U_U2_U-_D_D2_D-_R_R2_R-_L_L2_L-_F_F2_F-_B_B2_B-",
    "", "EMPTY_EMPTY", 0, "", "");

// LLSubsteps adds `ll` as the 3rd argument:
// solve(scr, rot, ll, num, len, move_restrict_string, ...)
const lls = new Module.PersistentLLSubstepsSolver();
lls.solve("R U R' ...", "", "CO EO", 2, 16,
    "U_U2_U-_R_R2_R-_F_F2_F-",
    "", "EMPTY_EMPTY", 0, "", "");
```

### String parameter formats

| Parameter | C++ helper | Format |
|---|---|---|
| `move_restrict_string` | `buidMoveRestrict()` | Underscore-separated move names; apostrophe written as `-` (e.g. `U-` = U') |
| `center_offset_string` | `buildCenterOffset()` | Pipe-separated `ROW_COL` pairs; empty rotation = `EMPTY_EMPTY`. Example: `EMPTY_EMPTY\|EMPTY_y\|EMPTY_y2` |
| `ma2_string` | `buidMA2()` | Move-after-move restriction string |
| `mcString` | `buildMoveCountVector()` | Per-move count restriction string |
| `ll` | `LL_substeps_option_array()` | Space-separated tokens: `CP CO EP EO` |

---

## Memory Sizing and ALLOW_MEMORY_GROWTH

Unlike the 2x2 solver which uses a fixed `TOTAL_MEMORY=150MB`, the crossSolver
uses:

```bash
-s INITIAL_MEMORY=50MB
-s ALLOW_MEMORY_GROWTH=1
```

**Rationale:** Multiple large prune tables may be live simultaneously. If a user
creates instances for cross + xcross(4 slots) + xxcross + xxxxcross + LLAUF, the
total can exceed 100 MB. A fixed allocation large enough for worst-case would
waste memory in typical usage.

`INITIAL_MEMORY=50MB` covers the most common scenario (cross + xcross + xxcross
dual slot at once). LL-family solvers will trigger a heap growth event on first
build; this is transparent to JS code but may cause a brief GC pause.

**Note:** `ALLOW_MEMORY_GROWTH=1` is slightly slower for memory accesses on
some platforms due to bounds-check overhead. The impact on this solver is
negligible (dominated by BFS and IDA* computation, not memory access patterns).

---

## MODULARIZE Build System

Same as the 2x2 solver. The compiled output exports a single `createModule()`
factory function (configured via `-s MODULARIZE=1 -s EXPORT_NAME="createModule"`).

```javascript
// Node.js
const createModule = require('./solver.js');
const Module = await createModule();

// Browser
<script src="solver.js"></script>
const Module = await createModule();

// Web Worker
importScripts('solver.js');
const Module = await createModule({ locateFile: (p) => baseURL + p });
```

All three environments use the exact same `solver.js` and `solver.wasm` binaries.

---

## Web Worker Integration

### worker-persistent.js design

Unlike a solver-specific worker (one class per worker), `worker-persistent.js`
is a **multi-solver worker**: it handles all 8 Persistent classes in a single
worker instance, routing by `solverType` in the incoming message.

```
Main thread                           Worker
──────────────                        ──────────────────────────────────
                                      importScripts('solver.js')
                                      createModule() → wasmModule
                                      → postMessage({ type: 'ready' })

postMessage({ type: 'solve',         →
  solverType: 'Xcross',
  slot: 0, scramble: '...', ...
})
                                      _getSolver(Module, 'Xcross', 0)
                                        → new Module.PersistentXcrossSolver(0)
                                           (cached in _solvers map)
                                      solver.solve(...)
                                        → C++: update("R U ...")
                                           → globalThis.postMessage(...)
                                             → { type: 'solution', data: '...' }
                                        → C++: update("Search finished.")
                                           → { type: 'done', data: null }

                                      ← postMessage({ type: 'solution', ... })
                                      ← postMessage({ type: 'done', ... })
```

**Solver instance caching inside the worker:**

```javascript
const _solvers = {};  // keyed by "Type:slot1:slot2:..."

function _getSolver(Module, type, ...slots) {
    const key = [type, ...slots].join(':');
    if (!_solvers[key]) {
        switch (type) {
            case 'Xcross': _solvers[key] = new Module.PersistentXcrossSolver(slots[0]); break;
            // ...
        }
    }
    return _solvers[key];  // Prune table persists between messages ✅
}
```

A `PersistentXcrossSolver` for slot=0 lives in `_solvers['Xcross:0']` for the
lifetime of the worker. Calling `solve` again on the same slot re-uses the table.

### postMessage override

C++ calls `update(str)` which calls `postMessage(str)` (a 1-argument form).
The worker overrides `globalThis.postMessage` before loading the WASM module to
intercept these raw strings and convert them to structured `{ type, data }`:

```javascript
const originalPostMessage = self.postMessage.bind(self);

globalThis.postMessage = function(message) {
    if (typeof message !== 'string') return;
    if (message === 'Search finished.') {
        originalPostMessage({ type: 'done', data: null });
    } else if (message === 'Search cancelled.') {
        originalPostMessage({ type: 'cancelled', data: null });
    } else if (message === 'Already solved.') {
        originalPostMessage({ type: 'solution', data: '' });
        originalPostMessage({ type: 'done',     data: null });
    } else if (message.startsWith('depth=')) {
        originalPostMessage({ type: 'depth',    data: message });
    } else if (message.startsWith('Error')) {
        originalPostMessage({ type: 'error',    data: message });
    } else {
        originalPostMessage({ type: 'solution', data: message });
    }
};
```

---

## Helper APIs

### Common pattern

Both `solver-helper-node.js` (Node.js) and `solver-helper.js` (Browser) expose
the same 8 solve methods. The key design choices:

**Solver instance cache (Node.js version only):**

```javascript
_getSolver(type, ...slots) {
    const key = [type, ...slots].join(':');
    if (!this._solvers[key]) {
        // new Module.PersistentXcrossSolver(slot), etc.
    }
    return this._solvers[key];
}
```

In Node.js, the helper holds the solver instances directly in the JS process.
In the browser, the instances live inside the worker; the helper only communicates
via `postMessage`.

**`_doSolve()` intercepts `globalThis.postMessage` (Node.js only):**

```javascript
_doSolve(callFn, options = {}) {
    const solutions = [];
    const origPostMessage = globalThis.postMessage;
    return new Promise((resolve) => {
        globalThis.postMessage = (msg) => {
            if (msg === 'Search finished.' || msg === 'Already solved.') {
                globalThis.postMessage = origPostMessage;
                resolve(solutions);
            } else if (msg === 'Search cancelled.') {
                globalThis.postMessage = origPostMessage;
                if (options.onCancel) options.onCancel(solutions.slice());
                resolve(solutions);
            } else if (msg.startsWith('depth=')) {
                if (options.onProgress) { /* ... */ }
            } else if (msg !== '') {
                solutions.push(msg);
                if (options.onSolution) options.onSolution(msg);
            }
        };
        callFn();
    });
}
```

**String normalization helpers (both versions):**

| Helper | Input | Output | Used by |
|---|---|---|---|
| `_restStr(allowedMoves)` | `string \| string[]` | `'U_U2_U-_...'` | all solve methods |
| `_centerOffsetStr(centerOffset)` | `string \| string[]` | `'EMPTY_EMPTY\|EMPTY_y'` | all solve methods |
| `_llStr(ll)` | `string \| string[]` | `'CO EO'` | `solveLLSubsteps` only |

### CDN support (browser version)

`solver-helper.js` detects CDN URLs and uses a Blob URL technique to bypass CORS
for Workers:

1. `fetch()` the worker script source.
2. Replace the `scriptPath`/`baseURL` extraction block with a hardcoded CDN URL.
3. Inline `solver.js` source into the blob to avoid `importScripts` failures in
   module contexts.
4. `new Worker(URL.createObjectURL(blob))`.

This is identical to the pattern used in `2x2solver/solver-helper.js`.

---

## Adding a New Solver

When adding a new solver (e.g., EOCrossSolver) based on this file:

1. **Add a new `*_search` struct** following the existing pattern:
   - Own `prune_table` + `prune_table_initialized` flag.
   - `start_search_persistent()` uses BFS-first-then-IDA* pattern.
   - No `solver_yield()` inside BFS loops; yield only in `start_search_persistent` before/between BFS calls.
2. **Reuse global move tables** where applicable.
3. **Add a `Persistent*Solver` wrapper** struct with `solver_clear_cancel()` at the start.
4. **Add `EMSCRIPTEN_BINDINGS`** entry.
5. **Recompile** with `./compile.sh`.
6. **Add a method** to `solver-helper-node.js` following the `_getSolver` / `_args` / `_doSolve` pattern.
7. **Add a `solverType` case** to `worker-persistent.js`'s `_getSolver` function and the `switch` in `onmessage`.
8. **Add a method** to `solver-helper.js` (browser) following the `_doSolve` pattern.
9. **Write tests** in `test/test-node-helper.js` and `test/test-worker.html` / `test/test-helper.html`.
