# 2x2 Persistent Solver — Release v1.0.0

This release introduces the WebAssembly-based 2x2x2 solver (PersistentSolver2x2) and the companion integration artifacts required to use it in browser and Node.js environments.

## Highlights

- Added `2x2solver/` distribution with a compiled WebAssembly module (`solver.js` + `solver.wasm`).
- Persistent pruning-table support: table is built once and reused across solves to enable efficient batch processing.
- Browser integration via Web Worker (`worker_persistent.js`) and a Promise-based Helper API (`solver-helper.js`).
- Node.js support (MODULARIZE-compatible module + `node-example.js`).
- CDN-ready packaging and documentation (README.md, IMPLEMENTATION_NOTES.md, TROUBLESHOOTING.md, demo.html).

## Files included

`solver.js`, `solver.wasm`, `worker_persistent.js`, `solver-helper.js`, `solver-helper-node.js`, `node-example.js`, `demo.html`, `example-helper.html`, documentation files under `dist/src/2x2solver/`.

## How to get it

- CDN: `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/`
- Local: use `node node-example.js` or load `worker_persistent.js`/`solver-helper.js` from `dist/src/2x2solver/`.

## Compatibility

- This is the initial public release for the 2x2 solver; there are no prior public versions to be compatible with.
- API exposes both a Web Worker `postMessage` interface and a Promise-based Helper API.