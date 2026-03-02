# crossSolver — Cross / F2L / LL Persistent Solver

High-performance WebAssembly-based 3×3 solver covering cross, x-cross, xx-cross, xxx-cross, xxxx-cross, and last-layer substeps, all with persistent pruning tables for optimal batch solving.

## 🎯 Quick Start

### Local server

```bash
# Serve from the repo root (files reference each other relatively)
python3 -m http.server 8000

# Test worker directly:
# http://localhost:8000/dist/src/crossSolver/test/test-worker.html

# Test helper API:
# http://localhost:8000/dist/src/crossSolver/test/test-helper.html
```

---

## 📚 Available Solver Classes

| Class | C++ struct | Slots | Default maxLength |
|---|---|---|---|
| `PersistentCrossSolver` | `cross` | — | 8 |
| `PersistentXcrossSolver` | `xcross` | `slot` 0-3 | 10 |
| `PersistentXxcrossSolver` | `xxcross` | `slot1`, `slot2` 0-3 | 12 |
| `PersistentXxxcrossSolver` | `xxxcross` | `slot1`, `slot2`, `slot3` 0-3 | 14 |
| `PersistentXxxxcrossSolver` | `xxxxcross` | — | 16 |
| `PersistentLLSubstepsSolver` | `LLSubsteps` | — (+ `ll` string) | 16 |
| `PersistentLLSolver` | `LL` | — | 18 |
| `PersistentLLAUFSolver` | `LLAUF` | — | 20 |

**Slot convention** (xcross / xxcross / xxxcross):

| Value | F2L pair included |
|---|---|
| 0 | Back-Right (BR) |
| 1 | Back-Left  (BL) |
| 2 | Front-Left (FL) |
| 3 | Front-Right (FR) |

---

## 🚀 Helper API — Recommended

### Browser

```html
<script src="solver-helper.js"></script>
<script>
(async () => {
  const helper = new CrossSolverHelper();
  await helper.init();

  // Cross
  const cross = await helper.solveCross("R U R' U' R' F R2 U' R' U' R U R' F'", {
    maxSolutions: 3,
    maxLength: 8,
  });
  console.log('cross:', cross);

  // Xcross (F2L slot 0 = Back-Right)
  const xcross = await helper.solveXcross("R U R' U' R' F R2 U' R'", 0, {
    maxSolutions: 2,
    maxLength: 10,
  });
  console.log('xcross:', xcross);

  // LLSubsteps — OLL (array input)
  const oll = await helper.solveLLSubsteps(
    "F2 L2 F2 U2 R2 U' F2 U B2 U' F2 U R' B2 R' U2 F' L' F",
    ['CO', 'EO'],
    { maxSolutions: 2, allowedMoves: ['U',"U'",'U2','R',"R'",'R2','F',"F'",'F2'] }
  );
  console.log('OLL:', oll);

  // Full LL solve
  const ll = await helper.solveLL("R U R' U' R' F R2 U' R' U' R U R' F'", {
    maxSolutions: 2,
    allowedMoves: ['U',"U'",'U2','R',"R'",'R2','F',"F'",'F2'],
  });
  console.log('LL:', ll);

  // Cancel a long solve
  setTimeout(() => helper.cancel(), 500);
  const partial = await helper.solveXxxxcross("R U ...", {
    maxSolutions: 1,
    onCancel: (sols) => console.log('cancelled, partial:', sols),
  });

  helper.terminate();
})();
</script>
```

### Node.js

```javascript
const CrossSolverHelperNode = require('./solver-helper-node.js');

(async () => {
  const helper = new CrossSolverHelperNode();
  await helper.init();

  const sols = await helper.solveCross("R U R' U' R' F R2 U' R' U' R U R' F'", {
    maxSolutions: 3,
  });
  console.log(sols);

  // Xcross — prune table is built once and reused on subsequent calls
  const slot0 = await helper.solveXcross(scramble, 0, { maxSolutions: 1 });
  const slot0again = await helper.solveXcross(scramble2, 0, { maxSolutions: 1 }); // fast

  await helper.init(); // no-op if already ready
})();
```

---

## 📖 Helper API Reference

### `CrossSolverHelper` / `CrossSolverHelperNode`

Both expose the identical Promise-based API. The browser version wraps
`worker-persistent.js`; the Node.js version loads the WASM module directly.

#### Lifecycle

```javascript
const helper = new CrossSolverHelper(workerPath);  // workerPath optional
await helper.init();     // Load WASM (required before solve methods)
helper.cancel();         // Cancel in-flight solve (resolves with partial solutions)
helper.terminate();      // Terminate worker, free all resources
await helper.reset();    // terminate() + init()
helper.isReady();        // → boolean
```

#### Solve methods

All solve methods share the same `options` object (see [Options](#options)).
They return `Promise<string[]>` — an array of solution strings.

```javascript
helper.solveCross(scramble, options)
helper.solveXcross(scramble, slot, options)           // slot: 0-3
helper.solveXxcross(scramble, slot1, slot2, options)  // slot1/slot2: 0-3
helper.solveXxxcross(scramble, slot1, slot2, slot3, options)
helper.solveXxxxcross(scramble, options)
helper.solveLLSubsteps(scramble, ll, options)         // ll: string | string[]
helper.solveLL(scramble, options)
helper.solveLLAUF(scramble, options)
```

#### `ll` parameter for `solveLLSubsteps`

```
Accepted tokens: 'CP'  'CO'  'EP'  'EO'
Pass as a space-separated string or an array.
Default '' / [] → no LL conditions enforced (any LL state accepted).

Examples:
  'EO'              → orient LL edges only
  ['CO', 'EO']      → orient all LL pieces (OLL)
  'CP EP'           → permute all LL pieces (PLL)
  ['CP','CO','EP','EO'] → full LL solve
```

#### Options

| Option | Type | Default | Description |
|---|---|---|---|
| `maxSolutions` | number | 3 | Max solutions to return |
| `maxLength` | number | (solver-specific) | Max solution length |
| `allowedMoves` | `string \| string[]` | all 18 moves | Allowed move set. Array of tokens (e.g. `['U',"U'",'U2',...]`) or underscore-separated string (`'U_U2_U-_...'`, `'` → `-`). |
| `rotation` | string | `''` | Whole-cube rotation (e.g. `'y'`, `'x2'`) |
| `postAlg` | string | `''` | Post-solve moves appended to each solution |
| `centerOffset` | `string \| string[]` | `''` | Target face orientation(s). `''` = white-bottom. Pass `['', 'y', 'y2', "y'"]` to allow any y-rotation. |
| `maxRotCount` | number | 0 | Max rotation moves in solution |
| `moveAfterMove` | string | `''` | Move-after-move restriction |
| `moveCount` | string | `''` | Per-move count restriction |
| `onProgress` | `(depth: number) => void` | — | Called at each depth iteration |
| `onSolution` | `(solution: string) => void` | — | Called as each solution is found |
| `onCancel` | `(partialSols: string[]) => void` | — | Called when the solve is cancelled |

---

## ⚙️ Direct Worker API (Advanced)

For applications that need full control, use `worker-persistent.js` directly.

### Message format (→ Worker)

```javascript
// Solve request
worker.postMessage({
  type:        'solve',
  solverType:  'Cross',   // See table above
  scramble:    "R U R' U'",

  // Slot params (only for the respective solverType):
  slot:   0,              // Xcross only
  slot1:  0,              // Xxcross, Xxxcross
  slot2:  3,              // Xxcross, Xxxcross
  slot3:  1,              // Xxxcross only

  // LLSubsteps only:
  ll: ['CO', 'EO'],       // string | string[]

  // Common optional params:
  rotation:     '',
  maxSolutions: 3,
  maxLength:    8,
  allowedMoves: ['U',"U'",'U2','R',"R'",'R2','F',"F'",'F2'],  // string | string[]
  postAlg:      '',
  centerOffset: '',       // string | string[] (e.g. ['', 'y', 'y2', "y'"])
  maxRotCount:  0,
  moveAfterMove:'',
  moveCount:    '',
});

// Cancel
worker.postMessage({ type: 'cancel' });
```

### Message format (← Worker)

```javascript
{ type: 'ready'  }                    // module loaded, ready to solve
{ type: 'solution',  data: string }   // one solution string per message
{ type: 'depth',     data: string }   // e.g. 'depth=5'
{ type: 'done',      data: null   }   // search finished normally
{ type: 'cancelled', data: null   }   // search was cancelled
{ type: 'error',     data: string }   // error message
```

### Direct Worker example

```javascript
const worker = new Worker('./worker-persistent.js');
const solutions = [];

worker.onmessage = (e) => {
  const msg = e.data;
  if (msg.type === 'ready') {
    worker.postMessage({
      type: 'solve',
      solverType: 'Cross',
      scramble: "R U R' U' R' F R2 U' R' U' R U R' F'",
      maxSolutions: 3,
      maxLength: 8,
    });
  } else if (msg.type === 'solution') {
    solutions.push(msg.data);
  } else if (msg.type === 'done') {
    console.log('Solutions:', solutions);
    worker.terminate();
  } else if (msg.type === 'error') {
    console.error(msg.data);
  }
};
```

---

## 💡 Examples

### Example 1: Batch Cross Solving (Node.js)

```javascript
const CrossSolverHelperNode = require('./solver-helper-node.js');

(async () => {
  const helper = new CrossSolverHelperNode();
  await helper.init(); // Prune table built once

  const scrambles = [
    "R U R' U' R' F R2 U' R' U' R U R' F'",
    "R U L2 F B2 D R2 U F2 D2 B L2 U2 R B U D2 F R2 U",
    "F2 U' R2 D' B2 U R2 D' L2 D2 B' F2 D R U' L D2 F' R' D",
  ];

  for (const scr of scrambles) {
    const sols = await helper.solveCross(scr, { maxSolutions: 1 });
    console.log(scr, '->', sols[0]);
  }
})();
```

### Example 2: LLSubsteps — OLL then PLL (Node.js)

```javascript
const CrossSolverHelperNode = require('./solver-helper-node.js');
const URF = ['U',"U'",'U2','R',"R'",'R2','F',"F'",'F2'];

(async () => {
  const helper = new CrossSolverHelperNode();
  await helper.init();

  const scramble = "F2 L2 F2 U2 R2 U' F2 U B2 U' F2 U R' B2 R' U2 F' L' F";

  // OLL: orient all LL pieces
  const oll = await helper.solveLLSubsteps(scramble, ['CO', 'EO'], {
    maxSolutions: 3,
    allowedMoves: URF,
  });
  console.log('OLL solutions:', oll);

  // PLL: permute all LL pieces
  const pll = await helper.solveLLSubsteps(scramble, ['CP', 'EP'], {
    maxSolutions: 3,
    allowedMoves: URF,
  });
  console.log('PLL solutions:', pll);
})();
```

### Example 3: Progress and Solution Callbacks (Browser)

```html
<script src="solver-helper.js"></script>
<script>
(async () => {
  const helper = new CrossSolverHelper();
  await helper.init();

  const sols = await helper.solveXcross(
    "R U L2 F B2 D R2 U F2 D2 B L2 U2 R B U D2 F R2 U",
    0,
    {
      maxSolutions: 3,
      maxLength: 10,
      onProgress: (d) => console.log(`Searching depth ${d}…`),
      onSolution: (s) => console.log(`Found: ${s}`),
    }
  );
  console.log('All solutions:', sols);

  helper.terminate();
})();
</script>
```

### Example 4: Cancel Support (Browser)

```html
<script src="solver-helper.js"></script>
<script>
(async () => {
  const helper = new CrossSolverHelper();
  await helper.init();

  // Cancel after 500 ms (e.g. user pressed Stop)
  setTimeout(() => helper.cancel(), 500);

  const sols = await helper.solveXcross(
    "R U L2 F B2 D R2 U F2 D2 B L2 U2 R B U D2 F R2 U",
    2,
    {
      maxSolutions: 5,
      maxLength: 10,
      onCancel: (partial) => console.log('Cancelled — partial:', partial),
    }
  );
  // sols contains whatever was found before the cancel fired
  console.log('Solutions before cancel:', sols);

  helper.terminate();
})();
</script>
```

### Example 5: CDN Loading

```html
<!-- Auto-detects worker path from CDN -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/crossSolver/solver-helper.js"></script>
<script>
(async () => {
  const helper = new CrossSolverHelper();
  await helper.init();
  const sols = await helper.solveCross("R U R' U'");
  console.log(sols);
  helper.terminate();
})();
</script>
```

---

## 📁 File Structure

```
dist/src/crossSolver/

# Core implementation
solver.cpp              # C++ source with all 8 Persistent solver structs
solver.js               # Compiled WASM module (MODULARIZE, universal)
solver.wasm             # WebAssembly binary (~600 KB)
compile.sh              # Build script (Emscripten)

# Web Worker
worker-persistent.js    # Single worker supporting all 8 solver classes

# Helper APIs
solver-helper.js        # Browser Promise-based wrapper
solver-helper-node.js   # Node.js Promise-based wrapper

# Documentation
README.md               # This file
IMPLEMENTATION_NOTES.md # C++ architecture & implementation details

# Test pages (not published to CDN)
test/
  test-worker.html      # Direct worker tests (9 cases)
  test-helper.html      # Helper API tests (12 cases)
  test-node-helper.js   # Node.js helper tests (24 assertions)
  test-cancel-node-direct.js  # Cancel / table-reuse tests (Node.js)
```

---

## 🔨 Compilation

### Requirements

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (`~/emsdk/`)

### Build

```bash
source ~/emsdk/emsdk_env.sh
cd dist/src/crossSolver
./compile.sh
```

Produces:
- `solver.js`   (~51 KB) — MODULARIZE factory module
- `solver.wasm` (~600 KB) — WebAssembly binary

### Compilation flags

```bash
em++ solver.cpp -o solver.js \
  -O3 -msimd128 \
  -s ASYNCIFY=1 \
  -s INITIAL_MEMORY=50MB \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind
```

| Flag | Purpose |
|---|---|
| `-O3 -msimd128` | Maximum optimization + SIMD vectorization |
| `-s ASYNCIFY=1` | Cooperative cancel via `solver_yield()` (incompatible with `-flto`) |
| `-s INITIAL_MEMORY=50MB` | Sufficient for cross + xcross + xxcross simultaneously |
| `-s ALLOW_MEMORY_GROWTH=1` | LL-family solvers can expand heap as needed (no OOM) |
| `-s MODULARIZE=1` | Factory-function pattern — universal Node/Browser/Worker |
| `-s EXPORT_NAME="createModule"` | Exports `createModule()` factory |
| `--bind` | Embind C++ → JS bindings |

> **Note:** `ALLOW_MEMORY_GROWTH=1` replaces the fixed `TOTAL_MEMORY` used in
> the 2x2 solver. It is required here because multiple large prune tables may be
> live simultaneously (e.g. xxxcross ~22 MB × 3).

---

## 🔗 Additional Resources

- **Implementation details**: [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md)
- **General troubleshooting**: [dist/TROUBLESHOOTING.md](../../TROUBLESHOOTING.md)
- **Worker compatibility**: [dist/TROUBLESHOOTING_WORKER_COMPAT.md](../../TROUBLESHOOTING_WORKER_COMPAT.md)
- **Related solver**: [dist/src/2x2solver/README.md](../2x2solver/README.md)

---

## 📝 License

See top-level [LICENSE](../../../LICENSE).

