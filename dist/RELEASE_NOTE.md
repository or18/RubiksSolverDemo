# Rubik's Cube Solver Libraries — Release Notes

## crossSolver-v2.0.0 (2026-03-02)

**New Package: 3×3 Cross / F2L / LL Persistent Solver (`dist/src/crossSolver/`)**

This release introduces the crossSolver, a new WebAssembly-based solver for the 3×3 cube covering cross, F2L (x-cross through xxxx-cross), and last-layer substeps — all with persistent pruning tables, cooperative cancel, and a single-worker multi-class architecture.

---

### New Features

**Eight Persistent Solver Classes:**

| JS class | Purpose | Prune table size | Build time (WASM) |
|---|---|---|---|
| `PersistentCrossSolver` | Cross (4 edges) | ~0.3 MB | ~50 ms |
| `PersistentXcrossSolver(slot)` | x-cross (cross + 1 F2L pair) | ~5.5 MB | ~870 ms |
| `PersistentXxcrossSolver(s1,s2)` | xx-cross | ~11 MB | ~2–4 s |
| `PersistentXxxcrossSolver(s1,s2,s3)` | xxx-cross | ~22 MB | ~5–8 s |
| `PersistentXxxxcrossSolver` | xxxx-cross (full F2L) | ~22 MB | ~7–12 s |
| `PersistentLLSubstepsSolver` | LL substeps (CP/CO/EP/EO) | ~22 MB | ~15–32 s |
| `PersistentLLSolver` | Full LL solve | ~22 MB | ~15–32 s |
| `PersistentLLAUFSolver` | LL solve + AUF | ~22 MB | ~15–32 s |

**Single Worker, Multi-Class Architecture:**

`worker-persistent.js` serves all 8 classes in one Web Worker process. Solver instances are cached by key (`"SolverType:slot1:slot2:..."`) so each unique combination builds its prune table only once per Worker lifetime.

**Helper APIs:**

- `solver-helper.js` — browser, Promise-based `CrossSolverHelper` class
- `solver-helper-node.js` — Node.js, same API (`CrossSolverHelperNode`)

Eight solve methods: `solveCross`, `solveXcross`, `solveXxcross`, `solveXxxcross`, `solveXxxxcross`, `solveLLSubsteps`, `solveLL`, `solveLLAUF`

**`ll` Parameter for LLSubsteps:**

```javascript
// Orient all LL pieces (OLL)
helper.solveLLSubsteps(scramble, ['CO', 'EO'], options)

// Permute all LL pieces (PLL)
helper.solveLLSubsteps(scramble, ['CP', 'EP'], options)

// Full LL solve
helper.solveLLSubsteps(scramble, ['CP', 'CO', 'EP', 'EO'], options)
```

**Cooperative Cancel:**

`helper.cancel()` stops an in-flight IDA* search and resolves the Promise with partial solutions. `onCancel` callback receives the partial array. Note: cancel fires in the IDA* search phase; during prune-table BFS build, `solver_yield()` is intentionally omitted (BFS optimization) — see Problem 8 in `TROUBLESHOOTING.md`.

---

### Technical Changes

**BFS Optimization (ASYNCIFY overhead fix):**

ASYNCIFY instruments every call site of `solver_yield()`. Placing `solver_yield()` inside the 4.5M-iteration `create_prune_table` BFS loop instrumented as the entire inner loop and caused measurable slowdown. Fix: `solver_yield()` is called **only** in `start_search_persistent` (before and between BFS calls), not inside `create_prune_table`. BFS now runs at native speed.

**Compilation flags:**

```bash
em++ solver.cpp -o solver.js \
  -O3 -msimd128 \
  -s ASYNCIFY=1 \
  -s INITIAL_MEMORY=50MB \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 -s EXPORT_NAME="createModule" \
  --bind
```

`ALLOW_MEMORY_GROWTH=1` is used instead of a fixed `TOTAL_MEMORY` because multiple large prune tables may be live simultaneously.

**Output sizes:** `solver.js` ~51 KB, `solver.wasm` ~600 KB

---

### Documentation Added / Updated

| File | Change |
|---|---|
| `dist/src/crossSolver/README.md` | New — user guide, API reference, all 8 methods, options table, code examples |
| `dist/src/crossSolver/IMPLEMENTATION_NOTES.md` | New — C++ architecture, BFS optimization, cancel design, "Adding a New Solver" checklist |
| `dist/README.md` | Updated — crossSolver Quick Start (browser + Node.js), unified Configuration table (2x2 vs 3x3 defaults), crossSolver links |
| `dist/TROUBLESHOOTING.md` | Updated — Problem 7 (slot-pair caching), Problem 8 (cancel timing during BFS), fixed broken relative links |
| `dist/src/2x2solver/IMPLEMENTATION_NOTES.md` | Updated — Cancel Support promoted to top-level section, crossSolver BFS comparison note added |

---

### CDN Distribution

```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@crossSolver-v2.0.0/dist/src/crossSolver/
```

---

### Compatibility

- crossSolver is a new, independent package — no breaking changes to 2x2solver
- Existing `Solver2x2Helper` code continues to work unchanged

---

## v1.2.0 (2026-03-01)

**New Features:**

- **Cooperative Cancel**: Ongoing solves can now be interrupted at any time via `helper.cancel()`
  - Works for both browser (`Solver2x2Helper`) and Node.js (`Solver2x2HelperNode`) helper APIs
  - `solve()` resolves immediately with partial solutions collected up to the cancellation point
  - Optional `onCancel` callback receives the partial solution array
  - Direct Worker users can send `{ type: 'cancel' }` to the Worker; it responds with `{ type: 'cancelled' }`

**Technical Changes:**

- **Asyncify-based cancel mechanism**: `solver_yield()` (EM_ASYNC_JS) suspends WASM execution for one JS event-loop tick, allowing the Worker's message handler to set `Module._cancelRequested = true`
  - Cancel checks at `depth >= 9` in `depth_limited_search` (shallower depths unwind via existing `return true` chain)
  - Per-depth cancel check in `create_prune_table` (partial table reset on cancel)
  - Post-table-build and per-iteration cancel checks in `start_search_persistent`
  - `solver_clear_cancel()` called at the start of every `solve()` to avoid flag bleed-over
- **Compile flag update**: `-s ASYNCIFY=1` added; `-flto` removed (incompatible with Asyncify)
- **WASM binary size increase**: side-effect of Asyncify instrumentation
  - `solver.js`: ~45KB → ~51KB
  - `solver.wasm`: ~195KB → ~344KB

**API Additions:**

| Addition | Description |
|----------|-------------|
| `helper.cancel()` | Cancels the current solve; `solve()` resolves with partial solutions |
| `options.onCancel` | Callback `(partialSolutions: string[]) => void` called on cancel |
| Worker `{ type: 'cancel' }` (to) | Cancels the ongoing search inside the Worker |
| Worker `{ type: 'cancelled' }` (from) | Worker response after cancel completes |

**CDN Distribution:**

- CDN: `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.2.0/dist/src/2x2solver/`

**Compatibility:**

- Fully backward compatible with v1.1.x and v1.0.0
- Existing code continues to work without changes
- `cancel()` and `onCancel` are purely additive; unused options have no effect

---

## v1.1.1 (2026-02-19)

**Improvements:**

- **Real-time Solution Streaming for Web**: Browser helper (`solver-helper.js`) now supports `onSolution` callback for real-time solution streaming, matching Node.js helper API
  - Solutions appear immediately as they're found (no need to wait for search completion)
  - Enables responsive UIs with live progress updates
  - API now consistent across Node.js and browser environments

- **Auto-loading tools.js**: Browser helper automatically loads `tools.js` when used from CDN
  - No manual `<script>` tag needed for tools.js
  - Simplified integration for CDN users
  - Cache-busting parameters propagated automatically

**Bug Fixes:**

- Fixed missing `currentSolutions.push()` in solution handler (solutions now correctly accumulate)
- Removed duplicate cleanup code in error handler

**CDN Distribution:**

- CDN: `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.1.1/dist/src/2x2solver/`

**Compatibility:**

- Fully backward compatible with v1.1.0 and v1.0.0
- Existing code continues to work without changes

---

## v1.1.0 (2026-02-19)

**New Features:**

- **Structured Parameter Input Support**: Helper APIs now accept structured data formats for solver parameters
  - `allowedMovesArray`: Array of move tokens (e.g., `['U', 'U2', "U'", 'R', 'R2', "R'", ...]`)
  - `moveOrderArray`: Array of `[left, right]` pairs for move-order constraints
  - `moveCountMap`: Object mapping moves to allowed counts (e.g., `{ "F'": 0, "F2": 0 }`)
  
- **Centralized Utilities**: Added `dist/src/utils/tools.js` for input normalization and validation
  - `validateRest`, `normalizeRestForCpp`, `buildRestFromArray`: Move set validation and conversion
  - `buildMavFromPairs`, `buildMcvFromObject`: Structured constraint builders
  - `validateMav`, `validateMcv`: Parameter validators
  - `invertAlg`: Algorithm inversion utility
  - Apostrophe notation support (`R'`) with automatic normalization to hyphen (`R-`)

- **Enhanced Helper APIs**: Both browser (`solver-helper.js`) and Node.js (`solver-helper-node.js`) helpers now:
  - Accept structured parameters alongside legacy string formats
  - Automatically convert structured inputs to worker-compatible strings
  - Provide better error messages for invalid inputs

**Documentation:**

- Added `dist/src/utils/TOOLS_API.md`: Complete API reference for utility functions
- Updated `dist/TROUBLESHOOTING.md` with consolidated troubleshooting guidance
- Added `dist/TROUBLESHOOTING_WORKER_COMPAT.md` with detailed worker/module compatibility notes
- Updated README files with links to new documentation

**CDN Distribution:**

- CDN: `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.1.0/dist/src/2x2solver/`

**Compatibility:**

- Fully backward compatible with v1.0.0
- Existing code using string parameters continues to work without changes
- New structured parameters are optional enhancements

---

## v1.0.0 (2026-02-16)

This release introduces the WebAssembly-based 2x2x2 solver (PersistentSolver2x2) and the companion integration artifacts required to use it in browser and Node.js environments.

**Highlights:**

- Added `2x2solver/` distribution with a compiled WebAssembly module (`solver.js` + `solver.wasm`).
- Persistent pruning-table support: table is built once and reused across solves to enable efficient batch processing.
- Browser integration via Web Worker (`worker_persistent.js`) and a Promise-based Helper API (`solver-helper.js`).
- Node.js support (MODULARIZE-compatible module + `node-example.js`).
- CDN-ready packaging and documentation (README.md, IMPLEMENTATION_NOTES.md, TROUBLESHOOTING.md, demo.html).

**Files included:**

`solver.js`, `solver.wasm`, `worker_persistent.js`, `solver-helper.js`, `solver-helper-node.js`, `node-example.js`, `demo.html`, `example-helper.html`, documentation files under `dist/src/2x2solver/`.

**CDN Distribution:**

- CDN: `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/`
- Local: use `node node-example.js` or load `worker_persistent.js`/`solver-helper.js` from `dist/src/2x2solver/`.

**Compatibility:**

- This is the initial public release for the 2x2 solver; there are no prior public versions to be compatible with.
- API exposes both a Web Worker `postMessage` interface and a Promise-based Helper API.