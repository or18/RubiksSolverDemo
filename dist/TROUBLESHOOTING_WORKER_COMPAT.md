# Troubleshooting — Worker Compatibility and Blob-runner fallback

This document explains common worker-loading issues that affect all solvers, how the Helper/Worker code handles them, and recommended mitigations.

## Module worker vs classic worker (key differences)

- classic-worker:
  - Created as `new Worker('worker_persistent.js')` (no `type: 'module'`).
  - Inside the worker `importScripts()` is available and commonly used to load additional scripts (e.g. `importScripts(baseURL + 'solver.js')`).
  - Works reliably when the worker script and assets are same-origin or CORS headers allow access.

- module-worker:
  - Created as `new Worker('worker_persistent.js', { type: 'module' })`.
  - `importScripts()` is not available; module syntax and `import` are used instead.
  - Module workers enforce stricter loading semantics and sometimes different CSP/CORS behavior.

Important: many existing solver worker shims (including `worker_persistent.js`) assume the classic-worker environment and call `importScripts(baseURL + 'solver.js')`. That pattern will fail in a module worker (TypeError / ReferenceError) because `importScripts` is undefined.

## Common failure modes

- `TypeError: importScripts is not a function`
  - Cause: `worker_persistent.js` ran in module-worker context.
  - Fix: Use the Helper API which detects CDN/cross-origin cases and uses a Blob-runner fallback (see below), or host worker/solver on the same origin and load as classic worker.

- `SecurityError` / `DOMException` when calling `new Worker(url)` with a cross-origin URL
  - Cause: Browsers prevent direct cross-origin worker creation in many cases (CORS, cross-origin policy).
  - Fix: Use a Blob-runner (inline a worker script via Blob URL) or ensure worker and resources are same-origin and served with appropriate CORS headers.

- WebAssembly or compile errors under strict CSP
  - Cause: CSP policies that disallow `unsafe-eval` or necessary runtime features may block Emscripten's glue code.
  - Fix: Relax CSP or build an alternative (e.g. asm.js) — not provided in this distribution.

## Blob-runner fallback (what it is and why Helper uses it)

When the Worker script is hosted on a CDN or otherwise cross-origin, creating a worker directly from the remote URL can fail due to CORS or because the worker must run as a module (no `importScripts`). The Blob-runner fallback addresses this by:

1. Fetching the original worker script text (`worker_persistent.js`) via `fetch()`.
2. Rewriting or fixing baseURL resolution so `locateFile` and `importScripts` calls resolve correctly.
3. If the worker contains `importScripts(...)` (classic pattern), fetching `solver.js` and inlining the runtime text before the worker body — ensuring the Emscripten `createModule()` factory is available inside the resulting blob.
4. Creating a Blob with the rewritten worker code and instantiating the worker using `new Worker(URL.createObjectURL(blob))`.

Benefits:
- Avoids cross-origin `new Worker(url)` restrictions.
- Allows classic-worker worker code (which uses `importScripts`) to run in environments where direct CDN-based worker loading would fail.
- Preserves cache-bust query parameters so CDN debugging remains easy.

Limitations:
- Inlining large JS may increase memory usage in the page/process.
- Blob-runner requires the helper to be able to fetch the worker and solver files (CORS must allow fetch).
- Environments with very strict CSP may still block execution.

## worker_persistent.js assumptions (what the worker expects)

- `worker_persistent.js` (the common worker entry) is written for the classic-worker environment and typically:
  - Calculates a `baseURL` from `self.location.href` (or via script path) and calls `importScripts(baseURL + 'solver.js')`.
  - Expects `solver.js` to expose a MODULARIZE factory (commonly `createModule()`), i.e. Emscripten was built with `-s MODULARIZE=1` and `-s EXPORT_NAME="createModule"`.
  - Calls `createModule()` inside the worker to get the Module object and then instantiates `PersistentSolver...` classes.
- If you host your own worker, ensure:
  - `solver.js` and `solver.wasm` are reachable from the worker via the same `baseURL`.
  - The server serves `solver.wasm` with correct MIME type (`application/wasm`) and appropriate CORS headers if needed.

## Recommendations / Best practices

- Use the Helper API (`solver-helper.js` / `solver-helper-node.js`) for most cases:
  - The Helper auto-detects when a CDN or cross-origin worker is being used and attempts the Blob-runner fallback automatically.
  - The Helper exposes structured options and normalizes apostrophe (`'`) to hyphen (`-`) for worker compatibility.

- For CDN-hosted distributions:
  - Prefer serving both `worker_persistent.js` and `solver.js` from the same CDN path.
  - When embedding via `<script src=".../solver-helper.js">`, allow the helper to compute the worker base path so query params for cache-busting propagate.

- If you control the hosting:
  - Host worker and solver on the same origin to allow direct `new Worker(worker_persistent.js)`.
  - Ensure `solver.wasm` is served with `Content-Type: application/wasm`.

- If you must use module workers:
  - Convert `worker_persistent.js` to use ES module imports instead of `importScripts`, and ensure the Emscripten glue is available as an importable ES module variant (requires build changes).
  - Alternatively, use the Helper’s Blob-runner to keep the worker in classic form.

## Debugging checklist

1. Console: look for `importScripts is not a function`, `SecurityError`, or `Wasm` instantiation errors.
2. Confirm network: `worker_persistent.js`, `solver.js`, and `solver.wasm` requests succeed (200) and are accessible from the page/worker context.
3. Check CORS: `Access-Control-Allow-Origin` must permit fetching resources if cross-origin.
4. Check MIME type: `solver.wasm` must be served as `application/wasm`.
5. Try Helper Blob-runner: use `solver-helper.js` and enable CDN mode — helper will attempt the inline fallback and log warnings on failures.
6. If using direct Worker: switch to same-origin hosting and verify `new Worker()` works in the page.

## Example: What Helper does when CDN worker fails

- Detects worker URL is cross-origin.
- Fetches the worker script; replaces the baseURL code so it points to CDN path.
- If `importScripts()` is present, it will fetch `solver.js` and prepend it to the worker script so `createModule()` is available.
- Creates a Blob from the resulting script and uses `new Worker(blobURL)`.

This pattern solves the most common CDN + `importScripts` incompatibilities without requiring rebuilds.

## Additional notes

- Node: the Node helper (`solver-helper-node.js`) runs the Emscripten module via `require()` / `import()` and does not use web Workers. Node-specific filesystem or loading errors are different; see Node examples in `dist/src/2x2solver/README.md`.
- CSP: very strict CSP can prevent Emscripten’s runtime from compiling; in those cases the only reliable option is to build artifacts that do not rely on `eval`/dynamic code or to control the hosting environment.

