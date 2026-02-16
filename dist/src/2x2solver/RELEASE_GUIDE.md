# 2x2 Solver Release Guide

2x2 Persistent Solver specific release checklist and verification steps.

**For general release workflow**, see [../../RELEASE_GUIDE.md](../../RELEASE_GUIDE.md).

---

## 📋 Pre-Release Checklist

### 1. Code Verification
- [ ] `solver.cpp` compiles without warnings
- [ ] `solver.js` + `solver.wasm` built with MODULARIZE
- [ ] `worker_persistent.js` works locally
- [ ] Helper APIs tested (`solver-helper.js`, `solver-helper-node.js`)
- [ ] `demo.html` runs all tests successfully (local + CDN modes)
- [ ] `example-helper.html` works (local + CDN modes)
- [ ] `node-example.js` executes successfully
- [ ] Documentation complete and accurate

### 2. Local Testing

**Test Suite (demo.html)**
```bash
cd dist/src/2x2solver
python -m http.server 8000
# Open http://localhost:8000/dist/src/2x2solver/demo.html
# Click "Run Test Suite (4 scrambles)"
# Expected: Total time ~5-6 seconds, all tests pass
```

**Helper API (example-helper.html)**
```bash
# Same server, open example-helper.html
# Test both "Local" and "CDN" modes
# Verify all solve buttons work correctly
```

**Node.js Direct Usage**
```bash
cd dist/src/2x2solver
node node-example.js
# Expected output:
# ✓ Found 3 solutions (R U R' U')
# ✓ Found 3 solutions (R2 U2, reusing prune table)
# === Complete! ===
```

### 3. File Organization

Production files ready for release:
```
# Core Implementation
solver.cpp              # C++ source with PersistentSolver2x2
solver.js               # Compiled WASM module (universal)
solver.wasm             # WebAssembly binary (~195KB)
compile.sh              # Build script

# Web Worker
worker_persistent.js    # Web Worker wrapper

# Helper APIs (Recommended)
solver-helper.js        # Browser Promise-based wrapper
solver-helper-node.js   # Node.js Promise-based wrapper

# Documentation
README.md               # User guide + API reference
IMPLEMENTATION_NOTES.md # C++ implementation details
TROUBLESHOOTING.md      # Common issues & solutions
RELEASE_GUIDE.md        # This file

# Demo & Examples
demo.html               # Interactive demo (local + CDN)
example-helper.html     # Helper API examples
node-example.js         # Node.js direct usage example

# Development Tools
purge-cdn-cache.sh      # CDN cache purging script
backups/                # Old test files & deprecated docs
```

**Verify all files exist and are up-to-date** ✅

---

## 📦 Post-Release Verification

After following [../../RELEASE_GUIDE.md](../../RELEASE_GUIDE.md) to create the release:

### CDN Testing

Wait 2-5 minutes after creating GitHub release, then test CDN availability:

**Using demo.html** (Easiest)
```bash
cd dist/src/2x2solver
python -m http.server 8000
# Open http://localhost:8000/demo.html
```

Test sequence:
1. ✅ **Local Mode** (default)
   - Uncheck "🌐 Use CDN"
   - Solve a scramble → Verify it works

2. ✅ **CDN Mode (@main)**
   - Check "🌐 Use CDN (jsDelivr)"
   - Uncheck "🔄 Cache Busting"
   - Solve a scramble → Verify CDN works

3. ✅ **Tagged Version**
   - Update demo.html CDN URL to `@2x2-solver-v1.0.0`
   - Or test via curl:
   ```bash
   curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js"
   # Should return: HTTP/2 200
   ```

### Performance Benchmarks

Verify performance matches expectations:

| Solve | Scramble | Expected Time | Notes |
|-------|----------|---------------|-------|
| 1st | `R U R' U'` | ~3-4s | Builds 84MB prune table |
| 2nd | `R2 U2` | <1s | Reuses table |
| 3rd | `U R2 U'` | <1s | Reuses table |
| 4th | `U' F2 U R' U' R F' R F2 U` | ~1-2s | 10-move, reuses table |

**Total**: ~5-6 seconds for 4 scrambles

---

## 🔄 Compilation

Only required if modifying C++ source code:

```bash
cd dist/src/2x2solver
./compile.sh

# Verify output
ls -lh solver.js solver.wasm
# solver.js  ~45KB
# solver.wasm ~195KB
```

**Requirements**:
- Emscripten SDK installed
- See [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md) for details

---

## 📝 2x2 Solver Release Notes Template

Use this when creating GitHub releases (copy to release description):

### v1.0.0 (Initial Release)

```markdown
## ✨ Features

- **Persistent Solver**: Builds 84MB pruning table once, reuses across all subsequent solves
- **URF Move Optimization**: 9 moves (U/U2/U'/R/R2/R'/F/F2/F') optimized for 2x2x2
- **Web Worker Support**: Non-blocking UI for browser applications
- **CDN Distribution**: Load directly via jsDelivr without installation
- **Helper APIs**: Promise-based wrappers for browser and Node.js
- **Batch Processing**: Efficient for solving multiple scrambles

## 📊 Performance

- Memory: ~150MB (84MB prune table + 66MB overhead)
- First solve: ~3-4s (includes table initialization)
- Subsequent solves: <1s each
- 4-scramble test suite: ~5-6s total

## 🔧 Technical Specifications

- **Compiled with**: Emscripten 3.1.69 (MODULARIZE mode)
- **Target**: WebAssembly + JavaScript glue code
- **Prune Depth**: 0-20 (default: 1 for URF)
- **Max Solutions**: 1 to ∞ (default: 3)
- **Max Length**: 1-30 (default: 11)
- **Allowed Moves**: URF (9 moves) - optimal for 2x2x2
```

### v1.1.0 (Minor Update - Example)

```markdown
## ✨ New Features

- Added `moveOrder` parameter for move sequence constraints
- Added `moveCount` parameter to limit specific move types

## 🔧 Specification Changes

- **maxSolutions**: Removed upper limit (previously capped at 100)
- **pruneDepth**: Range extended to 0-20 (was 0-11)
```

### v1.0.1 (Patch Release - Example)

```markdown
## 🐛 Bug Fixes

- Fixed Worker path detection in CDN dynamic loading
- Corrected parameter format documentation (moveOrder, moveCount)
- Fixed cache busting propagation to Worker script

## 📖 Documentation Updates

- Added Node.js non-helper usage example
- Clarified moveOrder format: `'ROW~COL|ROW~COL|...'`
- Clarified moveCount format: `'MOVE:COUNT_MOVE:COUNT_...'`

No API changes - fully backward compatible with v1.0.0.
```

---

## 📚 Additional Resources

- **General Release Process**: [../../RELEASE_GUIDE.md](../../RELEASE_GUIDE.md)
- **Implementation Details**: [IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md)
- **Troubleshooting**: [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **API Reference**: [README.md](README.md)
