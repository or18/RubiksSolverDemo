# 2x2 Solver Release Guide

Complete step-by-step guide for releasing the 2x2 Persistent Solver.

## 📋 Pre-Release Checklist

### 1. Code Verification ✅
- [x] solver.cpp compiles without warnings
- [x] solver.js + solver.wasm built with MODULARIZE
- [x] worker_persistent.js works locally
- [x] demo.html runs all 4 tests successfully
- [x] cdn-test.html works with jsDelivr CDN
- [x] Documentation complete (README.md, PERSISTENT_SOLVER_README.md)

### 2. Testing ✅
Run demo.html and verify:
```bash
cd dist/src/2x2solver
python -m http.server 8000
# Open http://localhost:8000/demo.html
# Click "Run Test Suite (4 scrambles)"
# Verify: Total time ~5-6 seconds, all tests pass
```

### 3. File Organization ✅
Production files in `dist/src/2x2solver/`:
```
├── solver.cpp              # Source code
├── solver.js + solver.wasm # Compiled WASM
├── worker_persistent.js    # Web Worker wrapper
├── demo.html               # Interactive demo
├── cdn-test.html           # CDN testing page
├── compile.sh              # Build script
├── purge-cdn-cache.sh      # CDN cache management
├── README.md               # User documentation
└── PERSISTENT_SOLVER_README.md  # Technical docs
```

---

## 🚀 Release Process

### Step 1: Commit All Changes

```bash
cd /workspaces/RubiksSolverDemo

# Check status
git status

# Add all 2x2 solver files
git add dist/src/2x2solver/
git add dist/README.md

# Commit with descriptive message
git commit -m "Release: 2x2 Persistent Solver v1.0.0

- Persistent solver with pruning table reuse
- URF move optimization (9 moves, prune_depth=1)
- Web Worker implementation for non-blocking UI
- CDN support via jsDelivr
- Complete documentation and examples
- Test suite with 4 scrambles (~5-6s total)
"

# Push to GitHub
git push origin main
```

### Step 2: Create GitHub Release

#### Option A: Via GitHub Web Interface (Recommended for first time)

1. **Go to GitHub Repository**
   - Navigate to: https://github.com/or18/RubiksSolverDemo

2. **Create New Release**
   - Click "Releases" (right sidebar)
   - Click "Draft a new release"

3. **Tag Configuration**
   - **Tag version**: `2x2-solver-v1.0.0`
   - **Target**: `main` branch
   - Choose a tag: Type `2x2-solver-v1.0.0` and select "Create new tag"

4. **Release Details**
   - **Release title**: `2x2 Persistent Solver v1.0.0`
   - **Description**:
     # 2x2x2 Persistent Solver v1.0.0
     
     High-performance WebAssembly 2x2x2 Rubik's Cube solver with persistent pruning tables.
     
     ## Features
     - ⚡ Persistent solver: Builds 84MB pruning table once, reuses across solves
     - 🎯 URF optimization: 9 moves (U/U2/U'/R/R2/R'/F/F2/F'), optimal for 2x2x2
     - 🔧 Web Worker: Non-blocking UI for browser applications
     - 🌐 CDN Ready: Load via jsDelivr without local installation
     - 📊 Fast: ~5-6 seconds for 4 scrambles (including table build on first solve)
     
     ## Quick Start
     
     ### Browser (CDN)
     ```javascript
     // See cdn-test.html for complete example
     const cdn = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/';
     ```
     
     ### Browser (Local)
     ```javascript
     const worker = new Worker('worker_persistent.js');
     worker.postMessage({
       scramble: "R U R' U'",
       maxSolutions: 3,
       maxLength: 11,
       pruneDepth: 1,
       allowedMoves: 'U_U2_U-_R_R2_R-_F_F2_F-'
     });
     ```
     
     ## Documentation
     - 📖 [User Guide](dist/src/2x2solver/README.md)
     - 🔬 [Technical Details](dist/src/2x2solver/PERSISTENT_SOLVER_README.md)
     - 🎮 [Interactive Demo](dist/src/2x2solver/demo.html)
     - 🌐 [CDN Test Page](dist/src/2x2solver/cdn-test.html)
     
     ## Files
     - `solver.cpp` - C++ source code
     - `solver.js` + `solver.wasm` - Compiled WASM module
     - `worker_persistent.js` - Web Worker wrapper
     - `demo.html` - Interactive demo
     - `cdn-test.html` - CDN loading example
     - `compile.sh` - Build script
     
     ## Performance
     - Test 1 (7-move): 1390ms, 3 solutions
     - Test 2 (8-move): 1439ms, 3 solutions  
     - Test 3 (9-move): 2153ms, 3 solutions
     - Test 4 (10-move): 3198ms, 3 solutions
     - **Total: ~5-6 seconds** (includes 84MB table build on first solve)
     
     ## Requirements
     - Browser: Modern browser with WebAssembly support
     - Node.js: v14+ (for Node.js usage)
     
     ## License
     See [LICENSE](LICENSE) file in repository root.

5. **Publish Release**
   - Click "Publish release"
   - ✅ GitHub creates the tag `2x2-solver-v1.0.0`

#### Option B: Via Command Line (Advanced)

```bash
# Create annotated tag
git tag -a 2x2-solver-v1.0.0 -m "2x2 Persistent Solver v1.0.0"

# Push tag to GitHub
git push origin 2x2-solver-v1.0.0

# Then create release on GitHub web interface
# (or use GitHub CLI: gh release create)
```

### Step 3: Verify CDN Availability

After publishing release, jsDelivr automatically picks it up:

```bash
# Wait 2-5 minutes, then test
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js"

# Should return: HTTP/2 200
```

**CDN URLs for v1.0.0:**
```
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/worker_persistent.js
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js
https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.wasm
```

### Step 4: Update Documentation for Users

Create a user-facing example in main README:

## Installation

### From CDN (Recommended)
```html
<script>
  async function loadSolver() {
    const cdn = 'https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/';
    // See cdn-test.html for complete implementation
  }
</script>
```

---

## 📦 Post-Release

### 1. Test CDN Version

**Interactive Demo Pages** (Easiest)

Both [demo.html](demo.html) and [example-helper.html](example-helper.html) have built-in CDN testing:

```bash
cd dist/src/2x2solver
python -m http.server 8000
# Open http://localhost:8000/demo.html
```

Testing steps:
1. ✅ **Test Local Mode First** (default)
   - Uncheck "🌐 Use CDN"
   - Solve a scramble
   - Verify it works

2. ✅ **Test CDN Mode (@main branch)**
   - Check "🌐 Use CDN (jsDelivr)"
   - Uncheck "🔄 Cache Busting" (test production-like mode)
   - Solve a scramble
   - Verify CDN version works

3. ✅ **Test with Cache Busting** (development)
   - Check "🔄 Cache Busting"
   - Solve a scramble
   - Verify latest changes are loaded (bypasses CDN cache)

**Manual URL Testing** (Advanced)

After creating Git tag `2x2-solver-v1.0.0`:
```html
<!-- Test stable version -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver-helper.js"></script>

<!-- Test development version -->
<script src="https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@main/dist/src/2x2solver/solver-helper.js"></script>
```

### 2. Announce Release
- Update main README.md with installation instructions
- Share CDN URLs with users
- Document breaking changes (if any)

### 3. Continue Development on @main
```bash
# After release, continue development on main branch
git checkout main

# @main = development version (can purge cache, frequent updates)
# @2x2-solver-v1.0.0 = stable version (never purge, permanent cache)
```

---

## 🔄 Future Updates

### Patch Release (v1.0.1 - Bug fixes only)
```bash
# Fix bug, test, commit
git commit -m "Fix: ..."
git tag -a 2x2-solver-v1.0.1 -m "Bug fixes"
git push origin 2x2-solver-v1.0.1
```

### Minor Release (v1.1.0 - New features, backward compatible)
```bash
# Add feature, test, commit
git commit -m "Feature: ..."
git tag -a 2x2-solver-v1.1.0 -m "New features"
git push origin 2x2-solver-v1.1.0
```

### Major Release (v2.0.0 - Breaking changes)
```bash
# Breaking change, test, commit
git commit -m "BREAKING: ..."
git tag -a 2x2-solver-v2.0.0 -m "Major update with breaking changes"
git push origin 2x2-solver-v2.0.0
```

---

## ⚠️ Important Notes

### CDN Cache Management

**Development (@main):**
- ✅ Can use `purge-cdn-cache.sh`
- ✅ Can use cache busting
- ⚠️ May break for users (use for testing only)

**Production (@2x2-solver-v1.0.0):**
- ❌ Never purge cache
- ❌ Never modify released tag
- ✅ Permanent, stable URLs
- ✅ Recommended for end users

### Versioning Strategy

Use [Semantic Versioning](https://semver.org/):
- **MAJOR** (v2.0.0): Breaking API changes
- **MINOR** (v1.1.0): New features, backward compatible
- **PATCH** (v1.0.1): Bug fixes only

### Tag Naming

Options:
1. `2x2-solver-v1.0.0` (clear, specific)
2. `v1.0.0` (simple, if repository is only for 2x2 solver)

For multi-solver repository, use prefix: `2x2-solver-v1.0.0`

---

## 📚 Resources

- [GitHub Releases Documentation](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [jsDelivr Documentation](https://www.jsdelivr.com/)
- [Semantic Versioning](https://semver.org/)

---

## ✅ Current Status

- **Development Branch**: `main`
  - URL: `@main/dist/src/2x2solver/`
  - Status: Active development
  - Can purge CDN cache

- **Latest Release**: Not yet released
  - Next version: `2x2-solver-v1.0.0`
  - Ready to release: ✅ Yes

---

## 🎯 Quick Commands Reference

```bash
# 1. Final commit
git add -A
git commit -m "Release: 2x2 Persistent Solver v1.0.0"
git push origin main

# 2. Create tag (after GitHub release is created)
git tag -a 2x2-solver-v1.0.0 -m "2x2 Persistent Solver v1.0.0"
git push origin 2x2-solver-v1.0.0

# 3. Test CDN (wait 2-5 minutes after release)
curl -I "https://cdn.jsdelivr.net/gh/or18/RubiksSolverDemo@2x2-solver-v1.0.0/dist/src/2x2solver/solver.js"

# 4. Purge cache (only for @main development)
cd dist/src/2x2solver
./purge-cdn-cache.sh
```
