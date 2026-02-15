# Rubik's Cube Solver Libraries

High-performance WebAssembly-based Rubik's Cube solvers with persistent pruning tables for optimal batch solving.

## Overview

This library provides WebAssembly-compiled cube solvers with persistent state optimization. Solvers build expensive pruning tables once and reuse them across multiple solves, dramatically improving performance for batch operations.

## Quick Start

### Web Worker (Browser - Recommended)

**Local files:**

```javascript
const worker = new Worker('src/2x2solver/worker_persistent.js');

worker.onmessage = (e) => {
  const msg = e.data;
  if (msg.type === 'ready') {
    worker.postMessage({
      scramble: "R U R' U'",
      maxSolutions: 3,
      maxLength: 11,
      pruneDepth: 1,
      allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
    });
  } else if (msg.type === 'solution') {
    console.log('Solution:', msg.data);
  }
};
```

**From CDN (jsDelivr):**

```javascript
// Helper function to create worker from CDN (bypasses CORS)
async function createWorkerFromCDN(baseURL) {
  const res = await fetch(baseURL + 'worker_persistent.js');
  let code = await res.text();
  
  // Replace baseURL calculation with CDN URL
  const oldCode = `const scriptPath = self.location.href;
const baseURL = scriptPath.substring(0, scriptPath.lastIndexOf('/') + 1);`;
  const newCode = `const baseURL = '${baseURL}';`;
  code = code.replace(oldCode, newCode);
  
  const blob = new Blob([code], { type: 'application/javascript' });
  return new Worker(URL.createObjectURL(blob));
}

// Usage
const cdn = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/';
const worker = await createWorkerFromCDN(cdn);
worker.onmessage = (e) => { /* same as above */ };
```

See [src/2x2solver/cdn-test.html](src/2x2solver/cdn-test.html) for complete example.

### Node.js

```javascript
const createModule = require('./src/2x2solver/solver.js');

(async () => {
  const Module = await createModule();
  const solver = new Module.PersistentSolver2x2();
  
  globalThis.postMessage = (msg) => {
    if (msg !== 'Search finished.' && msg !== '') {
      console.log('Solution:', msg);
    }
  };
  
  solver.solve(
    "R U R' U'", '', 3, 11, 1,
    'U_U2_U-_R_R2_R-_F_F2_F-', '', '', ''
  );
})();
```

## Browser Requirements

- WebAssembly support
- Web Workers (for non-blocking operation)
- ES6 modules

All modern browsers (Chrome, Firefox, Safari, Edge) are supported.

---

## 2x2x2 Cube Solver

Optimal solver for 2x2x2 Rubik's Cube (Pocket Cube) with persistent pruning tables.

### ✨ Key Features

- **Persistent Tables**: Build pruning table once (~3-4s), reuse for all subsequent solves (<1s each)
- **URF Optimization**: Uses 9 URF moves for optimal 2x2x2 solving
- **Non-blocking**: Web Worker keeps UI responsive during computation
- **Universal Binary**: Single compilation works in Node.js, browser, and Web Worker

### 🚀 Quick Demo

```bash
cd dist/src/2x2solver
python3 -m http.server 8000
# Open http://localhost:8000/demo.html
```

### 📋 Basic Usage (Web Worker)

```javascript
const worker = new Worker('src/2x2solver/worker_persistent.js');

worker.onmessage = (e) => {
  const msg = e.data;
  
  if (msg.type === 'ready') {
    // Worker initialized, send solve request
    worker.postMessage({
      scramble: "R U R' U'",
      maxSolutions: 3,
      maxLength: 11,
      pruneDepth: 1,
      allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
    });
  } else if (msg.type === 'solution') {
    console.log('Solution:', msg.data);
  } else if (msg.type === 'depth') {
    console.log('Progress:', msg.data);
  } else if (msg.type === 'done') {
    console.log('Complete!');
  }
};
```

### 📋 Basic Usage (Node.js)

```javascript
const createModule = require('./src/2x2solver/solver.js');

(async () => {
  const Module = await createModule();
  const solver = new Module.PersistentSolver2x2();
  
  let solutions = [];
  globalThis.postMessage = (msg) => {
    if (msg !== 'Search finished.' && msg !== '') {
      solutions.push(msg);
    }
  };
  
  // First solve builds table, subsequent solves reuse it
  const scrambles = ["R U R' U'", "R2 U2", "U R2 U'"];
  for (const scr of scrambles) {
    solutions = [];
    solver.solve(scr, '', 3, 11, 1, 'U_U2_U-_R_R2_R-_F_F2_F-', '', '', '');
    console.log(`${scr}:`, solutions);
  }
})();
```

### 📊 Performance

| Solve | Scramble | Time | Notes |
|-------|----------|------|-------|
| **1st** | `R U R' U'` | ~3-4s | Builds pruning table (~84MB) |
| **2nd** | `R2 U2` | <1s | **Reuses table** |
| **3rd** | `U R2 U'` | <1s | **Reuses table** |
| **4th** | 10-move scramble | ~1-2s | **Reuses table** |

**Total**: ~5-6 seconds for 4 scrambles (vs. ~12-16s without persistence)

### ⚙️ Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `scramble` | string | *(required)* | Scramble sequence (e.g., `"R U R' U'"`) |
| `maxSolutions` | number | `3` | Maximum number of solutions to find |
| `maxLength` | number | `11` | Maximum solution length in moves |
| `pruneDepth` | number | `1` | Search depth (1 recommended with URF) |
| `allowedMoves` | string | `"U_U2_U-_R_R2_R-_F_F2_F-"` | Allowed move set (URF recommended) |
| `rotation` | string | `""` | Whole-cube rotation |
| `preMove` | string | `""` | Pre-move sequence |
| `moveOrder` | string | `""` | Move order constraints |
| `moveCount` | string | `""` | Move count limits |

### 📁 Files

### 📁 Files

- `solver.cpp` - C++ solver with PersistentSolver2x2
- `solver.js` + `solver.wasm` - Universal MODULARIZE binary
- `worker_persistent.js` - Web Worker wrapper (recommended)
- `demo.html` - Interactive demo with UI
- `compile.sh` - Compilation script

📖 **[Full Documentation](./src/2x2solver/README.md)** - Complete API reference, compilation guide, and advanced examples.

📖 **[Implementation Guide](./src/2x2solver/PERSISTENT_SOLVER_README.md)** - Detailed technical documentation.

---

## 🔧 Compilation

Each solver can be recompiled from C++ source:

```bash
cd dist/src/2x2solver

# Load Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Compile
./compile.sh
```

See individual solver directories for specific compilation instructions.

---

## Other Solvers

*(Additional solvers available in `src/` directory)*

- Cross Solver (`crossSolver/`)
- EOCross Solver (`EOCrossSolver/`)
- XCross Trainer (`xcrossTrainer/`)
- XXCross Trainer (`xxcrossTrainer/`)
- F2L Pairing Solver (`F2L_PairingSolver/`)

Each solver has its own README with usage instructions.

---

## License

See repository root for license information.

## Contributing

Contributions welcome! Please see the main repository for guidelines.
