# 2x2 Persistent Solver — Release Notes

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