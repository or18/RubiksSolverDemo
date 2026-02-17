# Rubik's Cube Solver Libraries

High-performance WebAssembly-based Rubik's Cube solvers with persistent pruning tables for optimal batch solving.

## Overview

This library provides WebAssembly-compiled cube solvers with persistent state optimization. Solvers build expensive pruning tables once and reuse them across multiple solves, dramatically improving performance for batch operations.

**Key Features:**
- ⚡ **Persistent Tables** - Build once, reuse across solves (~75% faster for batch operations)
- 🌐 **CDN Ready** - Load directly from jsDelivr (no installation required)
- 🎯 **Promise-based API** - Modern async/await patterns
- 🔧 **Web Worker** - Non-blocking UI in browser applications
- 📦 **Universal Binary** - Single WASM build for Node.js + Browser

---

## 🚀 Quick Start

All examples use the **Helper API** - a Promise-based wrapper that handles Web Worker communication automatically.

### Browser (via CDN) - Single Solve

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>2x2 Solver Demo</title>
</head>
<body>
  <h1>2x2 Cube Solver</h1>
  <button onclick="solveCube()">Solve</button>
  <div id="output"></div>

  <!-- Load solver from CDN -->
  <script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
  
  <script>
    let helper = null;
    
    async function solveCube() {
      // Initialize solver (only once)
      if (!helper) {
        helper = new Solver2x2Helper();
        await helper.init();
        console.log('✅ Solver ready!');
      }
      
      // Solve scramble
      const solutions = await helper.solve("R U R' U'", {
        maxSolutions: 5,
        maxLength: 11,
        pruneDepth: 1
      });
      
      // Display results
      document.getElementById('output').innerHTML = 
        '<strong>Solutions:</strong><br>' +
        solutions.map((s, i) => `${i+1}. ${s}`).join('<br>');
    }
  </script>
</body>
</html>
```

**Key points:**
- Helper instance created once, reused for multiple solves
- First solve builds pruning table (~1s for depth=1)
- Subsequent solves reuse table (<1s)

### Node.js - Batch Processing with Persistence

For batch solving multiple scrambles efficiently:

```javascript
const Solver2x2HelperNode = require('./dist/src/2x2solver/solver-helper-node.js');

async function batchSolve() {
  const helper = new Solver2x2HelperNode();
  
  // Initialize once (builds table on first solve)
  await helper.init();
  console.log('✅ Solver initialized\n');
  
  // Solve multiple scrambles (table persists)
  const scrambles = [
    "R U R' U'",
    "R2 U2",
    "U R2 U'",
    "F R F' R'"
  ];
  
  for (const scramble of scrambles) {
    console.time(`Solve: ${scramble}`);
    
    const solutions = await helper.solve(scramble, {
      maxSolutions: 3,
      maxLength: 11,
      pruneDepth: 1,
      onProgress: (depth) => process.stdout.write(`\r  Depth: ${depth}`),
      onSolution: (sol) => console.log(`\n  ✅ ${sol}`)
    });
    
    console.timeEnd(`\nSolve: ${scramble}`);
    console.log(`  Total: ${solutions.length} solutions\n`);
  }
  
  // Helper automatically maintains pruning table for all solves
  // No need to manually manage state!
}

batchSolve().catch(console.error);
```

**Expected output:**
```
✅ Solver initialized

Solve: R U R' U'
  Depth: depth=0
  Depth: depth=1
  ✅ U R U' R'
  ✅ R' U' R' U R U
  ...
Solve: R U R' U': 1.2s  ← First solve (builds table)
  Total: 3 solutions

Solve: R2 U2
  Depth: depth=0
  ✅ U2 R2
  ...
Solve: R2 U2: 0.3s  ← Subsequent solves (reuse table)
  Total: 3 solutions

...
```

**Performance:**
- **First solve**: ~1s (builds table, depth=1)
- **Subsequent solves**: 0.1-0.5s (reuses table)
- **Total for 4 scrambles**: ~2-3s (vs. ~4-5s without persistence)

### Node.js - Simple Usage (via CDN or Local)

```javascript
// Using local files
const Solver2x2HelperNode = require('./dist/src/2x2solver/solver-helper-node.js');

// OR download from CDN first:
// curl -O https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper-node.js
// curl -O https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver.js
// curl -O https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver.wasm

async function solve() {
  const helper = new Solver2x2HelperNode();
  await helper.init();
  
  const solutions = await helper.solve("R U R' U'", {
    maxSolutions: 5
  });
  
  console.log('Solutions:', solutions);
}

solve();
```

### Browser - Local Files

```html
<!-- Serve files locally with: python3 -m http.server 8000 -->
<script src="dist/src/2x2solver/solver-helper.js"></script>
<script>
  (async () => {
    const helper = new Solver2x2Helper();
    await helper.init();
    
    const solutions = await helper.solve("R U R' U'");
    console.log(solutions);
  })();
</script>
```

### Advanced: Direct Worker Control

For advanced users who need full control over the Web Worker, see the [2x2 Solver Full Documentation](./src/2x2solver/README.md) for Worker API details.

---

## ⚙️ Configuration

### Parameter Reference

| Parameter | Range | Default | Format | Description |
|-----------|-------|---------|--------|-------------|
| maxSolutions | 1 to ∞ | 3 | number | Maximum solutions to find |
| maxLength | 1 to 30 | 11 | number | Maximum solution length |
| pruneDepth | 0 to 20 | 1 | number | Pruning table search depth |
| allowedMoves | string | URF (9 moves) | `'MOVE1_MOVE2_...'` | Allowed move set (underscore-separated) |
| rotation | string | '' | `'x'`, `'y'`, `'z'`, `'x2'`, etc. | Whole-cube rotation (empty string for none) |
| preMove | string | '' | `'R U R\' U\''` | Pre-move sequence (space-separated) |
| moveOrder | string | '' | `'ROW~COL\|ROW~COL\|...'` | Move ordering constraints (see notes) |
| moveCount | string | '' | `'U:2_R:3_F:1'` | Move count limits (see notes) |

**Important Notes:**
- **allowedMoves**: Moves must be underscore-separated (e.g., `'U_U2_U-_R_R2_R-'`). Use `-` for inverse (e.g., `U-` = U')
- **rotation**: Only rotations defined in the solver are recognized. Undefined rotations are ignored
- **preMove**: Standard cube notation with spaces (e.g., `'R U R\' U\''`)
- **moveOrder**: Advanced feature for move sequence constraints (rarely used)
- **moveCount**: Limits specific move types (rarely used)

### Optimal Settings for 2x2x2

**Move Set**: **URF (9 moves) - RECOMMENDED**
```javascript
allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
```
- URF generates complete coverage for 2x2x2
- More efficient than full 18-move set (UDLRFB)
- Same optimal solutions, faster search

**Prune Depth**: **1 - RECOMMENDED with URF**
```javascript
pruneDepth: 1
```
- Sufficient for URF complete system
- Faster initialization (~1s for first solve)
- Deeper values (8-11) provide minimal improvement with URF but slower initialization

**Example usage:**
```javascript
const solutions = await helper.solve("R U R' U'", {
  maxSolutions: 5,
  maxLength: 11,
  pruneDepth: 1,  // Fast initialization
  allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-',  // URF moves
  rotation: 'y',  // Optional rotation
  onProgress: (depth) => console.log(`Searching: ${depth}`)
});
```

---

## 📊 Performance

### Memory Usage

- **Maximum Memory**: ~150MB (includes WASM buffer and pruning table)
- **Typical Usage**: ~100MB for depth=1 URF table
- **After Solve**: Memory persists until solver instance destroyed

### Solve Times

**First Solve (builds pruning table):**

| Configuration | Initialization Time | Notes |
|---------------|-------------------|-------|
| `pruneDepth: 1` (URF) | ~1s | **Recommended** for 2x2x2 |
| `pruneDepth: 8-9` | ~3-5s | Deeper search, slower init |
| `moveOrder: '<x,y,z>'` | ~5-10s | Custom table structure |

**Subsequent Solves (reuses table):**

| Configuration | Solve Time | Notes |
|---------------|------------|-------|
| `pruneDepth: 1` | 0.1-3s | Depends on scramble complexity |
| `pruneDepth: 8+` | <0.1s | Very fast with deep tables |

**Batch Example (4 scrambles):**

```
Configuration: pruneDepth=1, URF moves

Solve 1: 1.2s  ← Builds table
Solve 2: 0.3s  ← Reuses table
Solve 3: 0.4s  ← Reuses table
Solve 4: 0.5s  ← Reuses table
─────────────
Total:   2.4s  (vs. ~5s without persistence)
```

### Performance Tips

1. **Reuse Helper Instance** - Don't create new helper for each solve
2. **Choose pruneDepth Wisely**:
   - `depth=1`: Fast init, slightly slower solves (good for occasional use)
   - `depth=8+`: Slower init, very fast solves (good for batch processing)
3. **Use URF Moves** - Optimal for 2x2x2, complete coverage
4. **Keep Browser Tab Active** - Workers may throttle in background tabs

---

## 🎮 Interactive Demos

Try the solver directly in your browser:

```bash
cd dist/src/2x2solver
python3 -m http.server 8000
# Open http://localhost:8000/demo.html
```

**Demo Features:**
- ✅ **Full Parameter Control** - Configure all 9 solver parameters
- ♻️ **Reuse Pruning Table** - Toggle persistence for performance testing
- 🌐 **CDN Toggle** - Switch between local files and jsDelivr CDN
- 🔄 **Cache Busting** - Development option for testing latest CDN changes
- 📊 **Real-time Results** - Solutions display as they're found

Both [demo.html](src/2x2solver/demo.html) and [example-helper.html](src/2x2solver/example-helper.html) support local and CDN modes.

---

## 🌐 CDN Usage

**Production (Recommended)** - Use versioned tags:
```html
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver-helper.js"></script>
```

**Development** - Use `@main` branch:
```html
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
```

**Cache Busting** (Development):
```html
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js?v=20260215"></script>
```

### CDN Cache Management (Developers)

When updating code, jsDelivr may cache old versions.

**Interactive Tools:**
- [demo.html](src/2x2solver/demo.html) and [example-helper.html](src/2x2solver/example-helper.html) have built-in CDN controls
- Enable "🌐 Use CDN" and "🔄 Cache Busting" for testing

**Instant Purge** (affects all users):
```bash
cd dist/src/2x2solver
./purge-cdn-cache.sh
```

For detailed troubleshooting, see **[Troubleshooting Guide](./src/2x2solver/TROUBLESHOOTING.md)**.

---

## 📁 File Structure & Documentation

### 2x2x2 Solver Files

For detailed file structure, API reference, and advanced usage, see:

📖 **[2x2 Solver Documentation](./src/2x2solver/README.md)** - Complete guide including:
- File structure and organization
- Full API reference (Helper + Worker)
- Advanced Worker control
- Compilation instructions
- Implementation details

📖 **[Implementation Notes](./src/2x2solver/IMPLEMENTATION_NOTES.md)** - C++ implementation details for developers

📖 **[Troubleshooting Guide](./src/2x2solver/TROUBLESHOOTING.md)** - Common issues and solutions:
- CDN cache problems
- Worker loading errors
- CORS issues
- Performance optimization
- Browser compatibility

📖 **[Release Guide](./src/2x2solver/RELEASE_GUIDE.md)** - Publishing and versioning workflow

---

## 🐛 Troubleshooting

For common issues and detailed debugging:

👉 **See [Troubleshooting Guide](./src/2x2solver/TROUBLESHOOTING.md)** for:
- CDN cache debugging
- Worker path problems
- CORS and cross-origin issues
- Performance problems
- Browser compatibility
- Build errors
- C++ implementation bugs

**Quick Fixes:**

| Issue | Solution |
|-------|----------|
| CDN serving old files | Run `./purge-cdn-cache.sh`, wait 2 min |
| Worker not found error | Use Helper API (auto-detection) |
| Browser hanging | Use Web Worker (Helper API) |
| Solutions not found | Increase `maxLength`, check `allowedMoves` |

---

## Browser Requirements

- **WebAssembly support** (all modern browsers)
- **Web Workers** (for non-blocking operation)
- **Fetch API** (for CDN loading)

**Supported Browsers:**
- Chrome 57+ ✅
- Firefox 52+ ✅
- Safari 11+ ✅
- Edge 16+ ✅

**Not supported:**
- Internet Explorer ❌
- Safari < 11 (iOS < 11.3) ❌

Note on strict Content Security Policy (CSP) environments
-------------------------------------------------------
- Some hosting environments and CMS platforms (for example, certain shared WordPress setups, restrictive managed hosting, or sites that disallow `unsafe-eval` and similar script sources) may prevent the Emscripten/WASM glue code from compiling or instantiating. In those cases you will see clear console errors such as WebAssembly.instantiate / CompileError or messages indicating `unsafe-eval` is blocked.
- A possible technical fallback is to build and ship asm.js (no-WASM) artifacts, but that has large performance costs and is not implemented in this distribution.
- We cannot reliably support every restrictive CSP configuration. If you must run in such an environment, consider either loosening the CSP to allow the required runtime features, hosting the solver binaries on a server you control, or using the C++ source directly and compiling in an environment that you control.

---

## 📖 Advanced Documentation

### For Developers

- **[Release Guide](./RELEASE_GUIDE.md)** - Versioning, tagging, CDN distribution workflow
- **[Multi-Class Worker Pattern](./MULTI_CLASS_WORKER.md)** - Managing multiple solver classes in a single worker for memory efficiency

### Solver-Specific Guides

- **2x2 Solver**:
  - [User Guide & API](./src/2x2solver/README.md)
  - [Implementation Details](./src/2x2solver/IMPLEMENTATION_NOTES.md)
  - [Troubleshooting](./src/2x2solver/TROUBLESHOOTING.md)
  - [Release Checklist](./src/2x2solver/RELEASE_GUIDE.md)

---

# 📄 License

See repository root for license information.

## 🤝 Contributing

Contributions welcome! Please see the main repository for guidelines.

