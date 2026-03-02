# Troubleshooting Guide

This guide covers common issues encountered when using the solvers distributed under `dist/`. It consolidates cross-solver practical guidance and references solver-specific troubleshooting where appropriate.

## Table of Contents

1. [CDN Cache Issues](#cdn-cache-issues)
2. [Worker Loading Problems](#worker-loading-problems)
3. [CORS and Cross-Origin Issues](#cors-and-cross-origin-issues)
4. [Browser Compatibility](#browser-compatibility)
5. [Build and Compilation Errors](#build-and-compilation-errors)
6. [C++ Implementation Bugs (notes)](#c-implementation-bugs)
7. [Multi-Solver Worker Issues (crossSolver)](#multi-solver-worker-issues-crosssolver)
   - [Problem 7: Slot instances and caching](#problem-7-do-i-need-a-separate-class-instance-or-separate-worker-for-each-slot-pair)
   - [Problem 8: Cancel behavior and timing](#problem-8-cancel-does-not-take-effect-quickly--cancel-during-prune-table-build)

---

## CDN Cache Issues

### Problem: Old Files Loading from CDN After Push

**Symptom:**
- Pushed new code to GitHub
- CDN still serves old version

**Cause:** jsDelivr caches files for a short period; new pushes don't always appear immediately.

**Solution:**

1. Purge CDN cache for the affected paths (see your distribution scripts, e.g. `dist/src/2x2solver/purge-cdn-cache.sh`).
2. Wait a minute and re-check.
3. Use cache-busting parameters during development (e.g. `?v=<timestamp>`) instead of purging production caches where possible.

**Verification:**

```bash
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"
```

---

## Worker Loading Problems

### Common causes

- Worker path resolution errors (relative paths vs script location)
- Cross-origin restrictions when creating workers from CDN URLs
- Module vs classic worker execution model differences (`importScripts()` not available in module workers)

### Common fixes and diagnostic steps

- Use the Helper API (`solver-helper.js` / `solver-helper-node.js`) — it auto-detects worker paths, propagates cache-bust parameters, and implements a Blob-runner fallback for CDN scenarios.
- If you must create workers manually, host `worker_persistent.js`, `solver.js`, and `solver.wasm` on the same origin and ensure correct MIME/CORS headers.

#### Worker path 404 (relative path vs script location)

Symptoms: `GET /worker_persistent.js 404` because the browser resolves the worker path relative to the page, not the helper script.

Fixes:

1. Use the Helper API (auto-detects script location).
2. Provide an explicit worker path: `new Solver2x2Helper('./dist/src/2x2solver/worker_persistent.js')`.

# Troubleshooting Guide

This guide covers common issues encountered when developing with the 2x2x2 Persistent Solver, with solutions learned from implementing this solver. Useful reference for future solvers in this repository.

## Table of Contents

1. [CDN Cache Issues](#cdn-cache-issues)
2. [Worker Loading Problems](#worker-loading-problems)
3. [CORS and Cross-Origin Issues](#cors-and-cross-origin-issues)
4. [Performance Problems](#performance-problems)
5. [Browser Compatibility](#browser-compatibility)
6. [Build and Compilation Errors](#build-and-compilation-errors)
7. [C++ Implementation Bugs](#c-implementation-bugs)

---

## CDN Cache Issues

### Problem 1: Old Files Loading from CDN After Push

**Symptom:**
```
- Pushed new code to GitHub
- CDN still serves old version
- Files show old line counts or missing features
```

**Cause:** jsDelivr caches files for 7 days by default. New pushes don't automatically update the cache.

**Solution:**

**Step 1: Purge CDN Cache**
```bash
cd dist/src/2x2solver
./purge-cdn-cache.sh
```

This script calls jsDelivr's Purge API to clear:
- `solver-helper.js`
- `worker_persistent.js`
- `solver.js`
- `solver.wasm`

**Step 2: Verify Purge Success**
```bash
# Check response (should see "Cache cleared successfully")
curl -X POST "https://purge.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"
```

**Step 3: Wait 1-2 Minutes**

jsDelivr cache purge is not instantaneous. Wait before testing.

**Step 4: Hard Refresh Browser**
- Windows/Linux: `Ctrl + Shift + R` or `Ctrl + F5`
- Mac: `Cmd + Shift + R`

**Verification:**
```bash
# Check line count (should match your local file)
curl "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js" | wc -l

# Compare with local
wc -l dist/src/2x2solver/solver-helper.js
```

### Problem 2: Browser Caching Old HTML Page

**Symptom:**
- CDN files updated ✅
- HTML page still loads old scripts ❌
- DevTools shows old URLs

**Cause:** Browser cached the HTML page itself, not just the JS files.

**Solution:**

1. **Disable Browser Cache (DevTools)**
	 - Open DevTools (F12)
	 - Network tab → Check "Disable cache"
	 - Keep DevTools open while testing

2. **Use Incognito/Private Window**
	 - Chrome: `Ctrl + Shift + N`
	 - Firefox: `Ctrl + Shift + P`

3. **Clear Site Data**
	 - DevTools → Application → Clear storage → "Clear site data"

### Problem 3: Cache Busting During Development

**Symptom:**
- Need to test CDN changes frequently
- Don't want to purge CDN (affects all users)
- Hard refresh doesn't always work

**Solution:** Use cache busting parameters

**Option A: UI Toggle (demo.html, example-helper.html)**
1. Enable "🔄 Enable Cache Busting" checkbox
2. Reload page
3. All CDN files loaded with `?v=<timestamp>`

**Option B: Manual Cache Busting**
```html
<script>
const timestamp = Date.now();  // e.g., 1771166366412
const script = document.createElement('script');
script.src = `https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js?v=${timestamp}`;
document.head.appendChild(script);
</script>
```

**Option C: Use Versioned Tags (Production)**
```
<!-- Development (may be cached) -->
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/...

<!-- Production (stable, permanent cache) -->
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/...
```

### Problem 4: Cache Bust Not Propagating to Worker/WASM

**Symptom:**
- `solver-helper.js?v=123` loads correctly ✅
- `worker_persistent.js` loads from CDN without `?v=` ❌
- `solver.wasm` loads from CDN without `?v=` ❌

**Cause:** Cache busting parameter not propagated to worker.

**Solution:** The helper API automatically propagates cache bust params:

```javascript
// solver-helper.js (lines 32-35)
if (document.currentScript && document.currentScript.src) {
	const scriptUrl = document.currentScript.src;
	const queryParams = scriptUrl.includes('?') ? ... : '';
	this.workerPath = baseUrl + 'worker_persistent.js' + queryParams;  // ✅ Propagates ?v=
}
```

**Verification:**
```javascript
const helper = new Solver2x2Helper();
console.log(helper.workerPath);  // Should include ?v= if present
console.log(helper.cacheBustParams);  // Should log "?v=1234..." if detected
```

---

## Worker Loading Problems

### Problem 1: Worker Path Incorrect (404 Not Found)

**Symptom:**
```
GET http://127.0.0.1:3000/worker_persistent.js 404 (Not Found)
```

**Cause:** Worker path is relative to HTML page, not script location.

**Solution:**

**Option A: Use Helper API with Auto-Detection**
```javascript
// ✅ Auto-detects worker path from script location
const helper = new Solver2x2Helper();
```

**Option B: Specify Worker Path Explicitly**
```javascript
const helper = new Solver2x2Helper('./dist/src/2x2solver/worker_persistent.js');
```

**Option C: Use CDN** (auto-detection works)
```html
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
<script>
	// Auto-detects CDN worker path
	const helper = new Solver2x2Helper();
</script>
```

### Problem 2: Worker Path Wrong with Dynamic Script Loading

**Symptom:**
```javascript
const script = document.createElement('script');
script.src = 'https://cdn/solver-helper.js';
document.head.appendChild(script);

// Later: Worker tries to load from local path ❌
GET http://127.0.0.1:3000/worker_persistent.js 404
```

**Cause:** `document.currentScript` is `null` after dynamic loading.

**Solution:** The helper searches DOM for script tags:

```javascript
// solver-helper.js (lines 38-47)
else if (typeof document !== 'undefined') {
	const scripts = document.getElementsByTagName('script');
	for (let i = 0; i < scripts.length; i++) {
		if (scripts[i].src && scripts[i].src.includes('solver-helper.js')) {
			scriptUrl = scripts[i].src;  // ✅ Finds CDN URL
			break;
		}
	}
}
```

**Fallback:** Specify workerPath manually:
```javascript
const helper = new Solver2x2Helper('https://cdn.../worker_persistent.js');
```

### Problem 3: Worker Initialized but Not Responding

**Symptom:**
```javascript
const helper = new Solver2x2Helper();
await helper.init();  // ⏳ Hangs forever
```

**Debugging:**

1. **Check Worker is Created**
	 ```javascript
	 console.log(helper.worker);  // Should be Worker instance, not null
	 ```

2. **Listen to Worker Messages**
	 ```javascript
	 helper.worker.onmessage = (e) => {
		 console.log('Worker message:', e.data);
	 };
	 helper.worker.onerror = (e) => {
		 console.error('Worker error:', e);
	 };
	 ```

3. **Check WASM Loading**
	 - Open DevTools → Network tab
	 - Filter: "wasm"
	 - Verify `solver.wasm` loads (195KB)
	 - Check status code (should be 200)

4. **Check for Errors in Worker Context**
	 - Some errors only appear in Worker console
	 - Look for red error messages in main console

---

## CORS and Cross-Origin Issues

### Problem 1: Cannot Load Worker Directly from CDN

**Symptom:**
```javascript
const worker = new Worker('https://cdn.jsdelivr.net/.../worker_persistent.js');
// ❌ DOMException: Failed to construct 'Worker'
// Cross-origin workers are not allowed
```

**Cause:** Browsers block cross-origin Workers for security reasons.

**Solution:** Use Blob URL technique (implemented in `solver-helper.js`):

```javascript
async function createWorkerFromCDN(workerUrl) {
	// 1. Fetch worker code from CDN
	const response = await fetch(workerUrl);
	let code = await response.text();
  
	// 2. Modify code to point to CDN for solver.js/solver.wasm
	const baseURL = workerUrl.substring(0, workerUrl.lastIndexOf('/') + 1);
	code = code.replace(
		`const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`,
		`const baseURL = '${baseURL}';`
	);
  
	// 3. Create Worker from Blob URL (same-origin)
	const blob = new Blob([code], { type: 'application/javascript' });
	return new Worker(URL.createObjectURL(blob));
}
```

**What this does:**
- Downloads Worker code from CDN via `fetch()` (allowed)
- Modifies code to use CDN paths for dependencies
- Creates Worker from Blob URL (same-origin, allowed)

See [demo.html](./src/2x2solver/demo.html) lines 470-530 for complete implementation.

### Problem 2: WASM Loading from CDN Fails

**Symptom:**
```
CompileError: WebAssembly.instantiate(): Wasm decoding failed
```

**Cause:** Incorrect MIME type or CORS headers on WASM file.

**Solution:**

jsDelivr automatically serves correct headers. Verify:
```bash
curl -I "https://cdn.jsdelivr.net/.../solver.wasm"
# Should show:
# content-type: application/wasm
# access-control-allow-origin: *
```

If using custom CDN, ensure server sends:
- `Content-Type: application/wasm`
- `Access-Control-Allow-Origin: *`

---

## Performance Problems

### Problem 1: First Solve Takes Too Long (>10 seconds)

**Symptom:**
```javascript
await helper.solve("R U R' U'");  // Takes 15+ seconds
```

**Diagnosis:**

Check parameters:
```javascript
const solutions = await helper.solve("R U R' U'", {
	pruneDepth: 8,  // ⚠️ Too deep for URF moves
	allowedMoves: 'U_U2_U-_D_D2_D-_L_L2_L-_R_R2_R-_F_F2_F-_B_B2_B-'  // ⚠️ Too many moves
});
```

**Solution:** Use optimal settings for 2x2x2:
```javascript
const solutions = await helper.solve("R U R' U'", {
	pruneDepth: 1,  // ✅ Sufficient for URF
	allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'  // ✅ URF complete system
});
```

### Problem 2: Subsequent Solves Still Slow

**Symptom:**
```javascript
await helper.solve("R U R' U'");  // 4s (OK)
await helper.solve("R2 U2");       // 4s (should be <1s!)
```

**Cause:** Helper instance not reusing pruning table.

**Diagnosis:**
```javascript
// Check if using separate instances
const helper1 = new Solver2x2Helper();
await helper1.solve("R U R' U'");
helper1.terminate();  // ❌ Destroys table

const helper2 = new Solver2x2Helper();  // ❌ New table
await helper2.solve("R2 U2");
```

**Solution:** Reuse the same helper instance:
```javascript
const helper = new Solver2x2Helper();
await helper.init();

// All solves reuse the same table ✅
await helper.solve("R U R' U'");
await helper.solve("R2 U2");
await helper.solve("U R2 U'");

// Terminate once at the end
helper.terminate();
```

### Problem 3: Browser UI Freezing During Solve

**Symptom:**
- Button clicks don't respond
- Page scroll is laggy
- Browser shows "Page Unresponsive" warning

**Cause:** Solver running on main thread instead of Worker.

**Solution:** Always use Web Worker (via Helper API or worker_persistent.js):

```javascript
// ❌ BAD: Main thread (blocks UI)
const Module = await createModule();
const solver = new Module.PersistentSolver2x2();
solver.solve(...);  // Freezes UI

// ✅ GOOD: Web Worker (non-blocking)
const helper = new Solver2x2Helper();
await helper.init();
await helper.solve(...);  // UI stays responsive
```

---

## Browser Compatibility

### Problem 1: Safari/iOS Not Working

**Symptom:**
- Works in Chrome ✅
- Works in Firefox ✅
- Fails in Safari ❌

**Cause:** Safari doesn't support ES6 features or WASM settings.

**Solution:**

1. **Check WASM Support**
	 ```javascript
	 if (typeof WebAssembly === 'undefined') {
		 console.error('WASM not supported');
	 }
	 ```

2. **Check Worker Support**
	 ```javascript
	 if (typeof Worker === 'undefined') {
		 console.error('Web Workers not supported');
	 }
	 ```

3. **Use Modern Safari** (iOS 11.3+ supports WASM + Workers)

### Problem 2: Module Loading Error in Older Browsers

**Symptom:**
```
SyntaxError: Unexpected token 'export'
```

**Cause:** Using ES6 modules in non-module context.

**Solution:**

Ensure `<script>` tag has correct type:
```html
<!-- ❌ BAD: Will fail with export/import -->
<script src="solver-helper.js"></script>

<!-- ✅ GOOD: solver-helper.js uses global exports -->
<script src="solver-helper.js"></script>
<script>
	const helper = new Solver2x2Helper();  // Available on window
</script>
```

`solver-helper.js` uses UMD pattern (works everywhere):
```javascript
if (typeof module !== 'undefined' && module.exports) {
	module.exports = Solver2x2Helper;
} else {
	window.Solver2x2Helper = Solver2x2Helper;
}
```

---

## Build and Compilation Errors

### Problem 1: Emscripten Not Found

**Symptom:**
```bash
./compile.sh
# em++: command not found
```

**Solution:**
```bash
# Activate Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Verify
which em++  # Should show path

# Then compile
./compile.sh
```

### Problem 2: WASM Binary Too Large

**Symptom:**
```
solver.wasm is 2MB (expected ~195KB)
```

**Cause:** Missing optimization flags.

**Solution:** Use correct compile script:
```bash
em++ solver.cpp -o solver.js \
	-O3 \              # Maximum optimization ✅
	-msimd128 \        # SIMD ✅
	-flto \            # Link-time optimization ✅
	-s TOTAL_MEMORY=150MB \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="createModule" \
	--bind
```

### Problem 3: MODULARIZE Compilation Breaks Worker

**Symptom:**
- `solver.js` works in browser ✅
- Fails in Worker: `createModule is not defined` ❌

**Cause:** Worker needs `importScripts()` before using factory.

**Solution:** Ensure worker_persistent.js uses correct pattern:
```javascript
// worker_persistent.js
importScripts('solver.js');  // Loads createModule() factory

createModule().then(Module => {
	// Use Module here
	const solver = new Module.PersistentSolver2x2();
});
```

---

## C++ Implementation Bugs

### Problem 1: move_restrict Accumulation Bug

**Symptom:**
- First solve: OK (1-2s)
- Second solve: Slow (5-10s)
- Third solve: Very slow (30s+)
- Fourth solve: Timeout or extreme delay

**Cause:** `move_restrict` vector not cleared between solves, causing exponential growth.

**Solution:** Clear vector in `start_search_persistent()`:
```cpp
void start_search_persistent(..., bool reuse = false) {
	sol.clear();             // Clear solutions
	move_restrict.clear();   // ✅ CRITICAL: Clear move restrictions
  
	// Rebuild move_restrict from arg_restrict
	for (std::string name : restrict) {
		// ...
	}
}
```

**Impact:** Performance improved from minutes to seconds for multi-solve scenarios.

**Reference:** [IMPLEMENTATION_NOTES.md](./src/2x2solver/IMPLEMENTATION_NOTES.md) for full details.

### Problem 2: prune_table_initialized Flag Not Set

**Symptom:**
- Table created on first solve ✅
- Table recreated on second solve ❌ (should reuse)

**Cause:** Forgot to set `prune_table_initialized = true`.

**Solution:**
```cpp
struct search {
	bool prune_table_initialized;
  
	search() {
		prune_table = std::vector<unsigned char>(88179840, 255);
		prune_table_initialized = false;
	}
};

void start_search_persistent(..., bool reuse = false) {
	if (!reuse) {
		create_prune_table(...);
		prune_table_initialized = true;  // ✅ Set flag
	} else if (!prune_table_initialized) {
		create_prune_table(...);
		prune_table_initialized = true;  // ✅ Set flag
	}
}
```

### Problem 3: postMessage Signature Mismatch

**Symptom:**
- Node.js: Works ✅
- Browser: Hangs or freezes ❌

**Cause:** `window.postMessage(msg)` requires 2 arguments, but EM_JS calls with 1.

**Solution:** Override `globalThis.postMessage` before loading module:

```javascript
// Browser
globalThis.postMessage = function(msg) {
	console.log(msg);  // Or custom handler
};

// Then load
const Module = await createModule();
```

Worker automatically overrides this in `worker_persistent.js`:
```javascript
// worker_persistent.js
let currentPostMessageHandler = (msg) => {
	postMessage({ type: 'solution', data: msg });
};

globalThis.postMessage = function(msg) {
	currentPostMessageHandler(msg);
};
```

---

## Debugging Tips

### Enable Verbose Logging

```javascript
// Helper API
const helper = new Solver2x2Helper();
helper.worker.onmessage = (e) => {
	console.log('[DEBUG] Worker message:', e.data);
};
helper.worker.onerror = (e) => {
	console.error('[DEBUG] Worker error:', e);
};
```

### Check CDN File Versions

```bash
# Check file exists
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"

# Check line count
curl -s "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js" | wc -l

# Search for version string
curl -s "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js" | grep "@version"
```

### Test Worker in Isolation

```javascript
// Test worker directly
const worker = new Worker('worker_persistent.js');

worker.onmessage = (e) => {
	console.log('Message:', e.data);
};

worker.onerror = (e) => {
	console.error('Error:', e.message, e.filename, e.lineno);
};

worker.postMessage({
	scramble: "R U R' U'",
	maxSolutions: 1,
	maxLength: 11
});
```

### Performance Profiling

```javascript
console.time('solve');
const solutions = await helper.solve("R U R' U'");
console.timeEnd('solve');
// → solve: 3421.5 ms
```

---

## Quick Reference: Common Solutions

| Problem | Quick Fix |
|---------|-----------|
| CDN serving old files | Run `./purge-cdn-cache.sh`, wait 2 min |
| Worker 404 error | Use `Solver2x2Helper()` with auto-detection |
| CORS error for Worker | Use Helper API (uses Blob URL) |
| First solve >10s | Use URF moves + pruneDepth=1 |
| Second solve still slow | Reuse same helper instance |
| UI freezing | Use Web Worker (Helper API) |
| Safari not working | Update to iOS 11.3+ / Safari 11.1+ |
| Build error | Run `source /path/to/emsdk/emsdk_env.sh` |

---

## Multi-Solver Worker Issues (crossSolver)

This section covers problems specific to `dist/src/crossSolver/` which uses a single Web Worker (`worker-persistent.js`) serving eight solver classes.

### Problem 7: Do I Need a Separate Class Instance (or Separate Worker) for Each Slot Pair?

**Question:** `solveXcross` has a `slot` parameter (0–3). Do I need to create a separate `CrossSolverHelper` or launch a new Worker for each slot?

**Answer:** No. A single `CrossSolverHelper` instance (and the single Worker it manages) covers all solver types and slot combinations.

`worker-persistent.js` maintains an internal cache of C++ instances keyed by `"SolverType:slot1:slot2:..."`. For example:

| First call | Cache key | Action |
|---|---|---|
| `solveXcross(scr, 0, ...)` | `"Xcross:0"` | Creates `PersistentXcrossSolver(0)`, builds prune table |
| `solveXcross(scr, 0, ...)` | `"Xcross:0"` | Reuses cached instance — **fast** |
| `solveXcross(scr, 1, ...)` | `"Xcross:1"` | Creates new `PersistentXcrossSolver(1)`, builds prune table |
| `solveXxcross(scr, 0, 1, ...)` | `"Xxcross:0:1"` | Creates new `PersistentXxcrossSolver(0,1)`, builds table |

**Memory implications:** Each unique combination holds its own prune table in WASM heap memory for the lifetime of the Worker. Calling all 4 Xcross slots allocates 4 tables (~5.5 MB each). If memory is tight, terminate the helper and create a new one to free all tables.

**When to use separate Workers:** Only if you need true parallel execution (one Worker per CPU core) or want to isolate a failure in one solver from affecting others. In that case, create multiple `CrossSolverHelper` instances — each manages its own Worker and WASM module.

---

### Problem 8: Cancel Does Not Take Effect Quickly / Cancel During Prune-Table Build

#### 8a: Cancel fires slowly during prune-table build (crossSolver)

**Symptom:**
```javascript
const helper = new CrossSolverHelper();
await helper.init();
setTimeout(() => helper.cancel(), 50);    // cancel after 50 ms
const sols = await helper.solveXxxcross(scramble, { maxSolutions: 1 });
// Promise resolves after several seconds, not after 50 ms
```

**Cause:** In `crossSolver`, `solver_yield()` (the ASYNCIFY yield point that allows the Worker message loop to detect a cancel) is **not** placed inside `create_prune_table`. This is a deliberate BFS optimization (see `crossSolver/IMPLEMENTATION_NOTES.md` §6). As a result, a cancel request posted while the pruning table is being built **will not take effect until the BFS finishes**.

Approximate prune-table build times (Node.js WASM):

| Solver | Build time |
|---|---|
| Cross | ~50 ms |
| Xcross | ~870 ms |
| Xxcross | ~2–4 s |
| Xxxcross / Xxxxcross / LL / LLAUF | ~5–32 s |

**Note:** Once the prune table is already built (second and subsequent calls with the same solver/slot), cancel fires quickly — during the IDA\* search phase, `solver_yield()` is called at the start of each depth iteration.

**Workaround:** If you need fast cancel responsiveness during a long prune-table build, consider:
1. Pre-warming the solver at application startup (`await helper.init()` is not enough — send one dummy solve to trigger table construction before the user fires real solves).
2. Choosing a solver/slot combination whose table is already built.

#### 8b: Cancel in 2x2solver is faster during BFS

In `2x2solver`, `solver_yield()` IS still present inside `create_prune_table` (per-depth outer-loop yield + every 100,000 inner-loop iterations). So cancel during prune-table build in the 2x2 solver responds within roughly one BFS depth.

#### 8c: Partial table is discarded after cancel during BFS (crossSolver)

**Symptom:**
```
First solve (while building) → cancelled → 5 s
Second solve → still takes 5 s, not instant
```

**Cause:** When cancel fires during `create_prune_table` in `crossSolver`, `prune_table_initialized` is **not** set to `true`. The next `solve()` call rebuilds the table from scratch. This is correct behavior — a partially built table is unusable.

**Solution:** Either wait for the table to be fully built (avoid cancelling during BFS), or accept that the next first-solve will also be slow.

#### 8d: Cancel flag bleeds into next solve

**Symptom:**
```javascript
helper.cancel();          // cancel current solve
const sols = await helper.solveCross(scr, ...); // immediately resolve with []
```

**Cause:** `Module._cancelRequested` was left `true` from the previous cancel.

**Diagnosis:** This should **not** happen — each C++ `PersistentXxxSolver::solve()` begins with `solver_clear_cancel()` which resets the flag. If it does happen, check that `worker-persistent.js` routes to the correct solver instance and that `solver_clear_cancel()` is called.

---

## Getting Help

If you encounter an issue not covered here:

1. **Check console** for error messages
2. **Test with demo.html** to verify setup
3. **Compare with example-helper.html** for reference implementation
4. **Review [README.md](README.md)** for API usage
5. **Check [IMPLEMENTATION_NOTES.md](./src/2x2solver/IMPLEMENTATION_NOTES.md)** for C++ details
6. **Open GitHub issue** with reproducible example

---

## Version-Specific Notes

## Version Information

### v1.0.0 (Current)
- CDN auto-detection (static + dynamic loading)
- Cache busting propagation to Worker/WASM
- Full feature set included in initial release