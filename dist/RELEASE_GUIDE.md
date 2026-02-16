# dist/ Release Guide

General release workflow for distributing solver updates via jsDelivr CDN.

---

## 🚀 Standard Release Flow

### 1. Commit Changes

```bash
cd /workspaces/RubiksSolverDemo

# Stage all changes in dist/
git add dist/

# Commit with descriptive message
git commit -m "Release: <solver-name> v<version>

<Brief description of changes>
"

# Push to main branch
git push origin main
```

### 2. Create Git Tag

**Tag Naming Convention: `<solver-name>-v<version>`**

This repository uses **solver-specific tags** to allow independent versioning of each solver:

```bash
2x2-solver-v1.0.0      # 2x2 solver initial release
2x2-solver-v1.1.0      # 2x2 solver minor update
cross-solver-v1.0.0    # Future: cross solver initial release
xcross-solver-v1.0.0   # Future: xcross solver initial release
```

**Benefits:**
- ✅ **Independent versioning** - Each solver can be updated without affecting others
- ✅ **Clear CDN URLs** - `@2x2-solver-v1.0.0/dist/src/2x2solver/`
- ✅ **Flexible updates** - Bug fix for one solver doesn't require re-releasing all solvers
- ✅ **Clear history** - Easy to track changes per solver

**For this release (2x2-solver v1.0.0):**

```bash
# Create annotated tag
git tag -a 2x2-solver-v1.0.0 -m "2x2 Persistent Solver v1.0.0"

# Push tag to GitHub
git push origin 2x2-solver-v1.0.0
```

### 3. Create GitHub Release

#### Option A: GitHub Web UI (Recommended)

1. Navigate to repository: https://github.com/or18/RubiksSolverDemo
2. Click **"Releases"** → **"Draft a new release"**
3. Fill in release details (see template below)
4. Click **"Publish release"**

#### Option B: GitHub CLI

```bash
gh release create 2x2-solver-v1.0.0 \
  --title "2x2 Persistent Solver v1.0.0" \
  --notes-file RELEASE_NOTES.md
```

### 4. Verify jsDelivr CDN

Wait 2-5 minutes for jsDelivr to index the new release:

```bash
# Test CDN availability
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js"

# Should return: HTTP/2 200
```

**Stable CDN URLs** (after tag release):
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver-helper.js
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/worker_persistent.js
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.wasm
```

---

## 📝 Release Notes Template

Use this template when creating GitHub releases. **Only include Features and specification changes**.

### Example: v1.0.0 (Initial Release)

```markdown
# 2x2x2 Persistent Solver v1.0.0

High-performance WebAssembly 2x2x2 Rubik's Cube solver with persistent pruning tables.

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

## 📦 Distribution

### CDN (Recommended)
```javascript
const cdn = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/';
```

### Files
- `solver.js` (~45KB, gzipped ~12KB)
- `solver.wasm` (~195KB, gzipped ~65KB)
- `worker_persistent.js` (Web Worker wrapper)
- `solver-helper.js` (Browser helper API)
- `solver-helper-node.js` (Node.js helper API)

## 📖 Documentation

- [User Guide](https://github.com/or18/RubiksSolverDemo/blob/2x2-solver-v1.0.0/dist/src/2x2solver/README.md)
- [API Reference](https://github.com/or18/RubiksSolverDemo/blob/2x2-solver-v1.0.0/dist/README.md)
- [Implementation Details](https://github.com/or18/RubiksSolverDemo/blob/2x2-solver-v1.0.0/dist/src/2x2solver/IMPLEMENTATION_NOTES.md)
- [Troubleshooting](https://github.com/or18/RubiksSolverDemo/blob/2x2-solver-v1.0.0/dist/src/2x2solver/TROUBLESHOOTING.md)

## 🌐 Live Demos

- [Interactive Demo](https://or18.github.io/RubiksSolverDemo/dist/src/2x2solver/demo.html)
- [Helper API Examples](https://or18.github.io/RubiksSolverDemo/dist/src/2x2solver/example-helper.html)

## 📄 License

See [LICENSE](https://github.com/or18/RubiksSolverDemo/blob/main/LICENSE)
```

### Example: v1.1.0 (Minor Update)

```markdown
# 2x2x2 Persistent Solver v1.1.0

## ✨ New Features

- Added `moveOrder` parameter for move sequence constraints
- Added `moveCount` parameter to limit specific move types
- Increased `pruneDepth` maximum from 11 to 20

## 🔧 Specification Changes

- **maxSolutions**: Removed upper limit (previously capped at 100)
- **pruneDepth**: Range extended to 0-20 (was 0-11)

## 📖 Documentation

Full changelog: https://github.com/or18/RubiksSolverDemo/compare/2x2-solver-v1.0.0...2x2-solver-v1.1.0
```

**Command:**
```bash
git tag -a 2x2-solver-v1.1.0 -m "2x2 Solver v1.1.0 - New features"
git push origin 2x2-solver-v1.1.0
```

### Example: v1.0.1 (Patch Release)

```markdown
# 2x2x2 Persistent Solver v1.0.1

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

**Command:**
```bash
git tag -a 2x2-solver-v1.0.1 -m "2x2 Solver v1.0.1 - Bug fixes"
git push origin 2x2-solver-v1.0.1
```

---

## 📋 Versioning Strategy

Follow [Semantic Versioning 2.0.0](https://semver.org/) **for each solver independently**:

**`<solver-name>-vMAJOR.MINOR.PATCH`**

- **MAJOR** (e.g., 2x2-solver-v2.0.0): Breaking changes
  - API signature changes
  - Removal of deprecated features
  - Different parameter meanings
  - Example: Changing `solve()` parameter order

- **MINOR** (e.g., 2x2-solver-v1.1.0): New features (backward compatible)
  - New parameters (with defaults)
  - New helper functions
  - Performance improvements
  - Example: Adding `moveOrder` parameter

- **PATCH** (e.g., 2x2-solver-v1.0.1): Bug fixes only
  - No new features
  - No API changes
  - Documentation corrections
  - Example: Fixing Worker path detection bug

**Note:** Each solver maintains its own version number independently. For example, you can have:
- `2x2-solver-v1.0.0` and `cross-solver-v2.3.1` simultaneously

---

## ⚠️ CDN Cache Management

### Development Branch (`@main`)

```javascript
// Development CDN URL
'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/'
```

- ✅ **Can purge cache**: Use `purge-cdn-cache.sh`
- ✅ **Can use cache busting**: `?v=${Date.now()}`
- ⚠️ **May break**: Not recommended for production
- 🎯 **Use for**: Testing, development, demos

### Tagged Release (`@2x2-solver-v1.0.0`)

```javascript
// Stable CDN URL
'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/'
```

- ❌ **Never purge cache**: Permanent URLs
- ❌ **Never modify tag**: Immutable releases
- ✅ **Always stable**: Production-ready
- 🎯 **Use for**: End users, production apps

---

## 🔄 Quick Command Reference

**For 2x2 Solver v1.0.0:**

```bash
# 1. Commit and push changes
git add dist/src/2x2solver/
git commit -m "Release: 2x2 Solver v1.0.0"
git push origin main

# 2. Create and push tag
git tag -a 2x2-solver-v1.0.0 -m "2x2 Persistent Solver v1.0.0"
git push origin 2x2-solver-v1.0.0

# 3. Verify CDN (wait 2-5 minutes)
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js"

# 4. Purge cache (only for @main development)
cd dist/src/2x2solver
./purge-cdn-cache.sh
```

**For future solvers (example):**
```bash
# Cross solver v1.0.0
git tag -a cross-solver-v1.0.0 -m "Cross Solver v1.0.0"
git push origin cross-solver-v1.0.0
```

---

## 📚 Additional Resources

- **Solver-Specific Guides**: See `dist/src/<solver-name>/RELEASE_GUIDE.md`
- **GitHub Releases**: https://docs.github.com/en/repositories/releasing-projects-on-github
- **jsDelivr Docs**: https://www.jsdelivr.com/
- **Semantic Versioning**: https://semver.org/
