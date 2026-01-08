# XCross + Free Pair Trainer - Production Deployment Guide

**Last Updated**: 2026-01-08 00:40  
**Status**: ✅ READY FOR DEPLOYMENT

---

## Quick Start

### 1. Build Production WASM

```bash
cd src/xcross_free_pair_trainer/production
./build_production.sh
```

**Output**:
- `solver_prod.js` (~95 KB)
- `solver_prod.wasm` (~307 KB)

### 2. Production API

```javascript
// Initialize solver with tier configuration
const solver = new Module.xxcross_search(adjacent, modelName);

// Available model names:
// - "1M/1M/2M/2M" (default, recommended)
// - "1M/1M/1M/1M" (low-end mobile)
// - "1M/1M/2M/4M" (high-end mobile)
```

---

## Production Configurations

### Default: 1M/1M/2M/2M (Recommended)

**Memory**:
- Single heap: 331.88 MB
- Dual heap: 664 MB (adjacent + opposite)

**Target Devices**: Standard mobile devices

**Usage**:
```javascript
const solverAdj = new Module.xxcross_search(true, "1M/1M/2M/2M");
const solverOpp = new Module.xxcross_search(false, "1M/1M/2M/2M");
```

### Low-End: 1M/1M/1M/1M

**Memory**:
- Single heap: 331.88 MB
- Dual heap: 664 MB

**Target Devices**: Budget mobile devices

**Usage**:
```javascript
const solver = new Module.xxcross_search(true, "1M/1M/1M/1M");
```

### High-End: 1M/1M/2M/4M

**Memory**:
- Single heap: 398.25 MB
- Dual heap: 796 MB

**Target Devices**: Premium mobile devices

**Usage**:
```javascript
const solver = new Module.xxcross_search(true, "1M/1M/2M/4M");
```

---

## Legacy Compatibility

The following legacy model names are also supported:
- "MOBILE_LOW" (1M/1M/2M/4M)
- "MOBILE_MIDDLE" (2M/4M/4M/4M)
- "MOBILE_HIGH" (4M/4M/4M/4M)
- "DESKTOP_STD" (8M/8M/8M/8M)
- "DESKTOP_HIGH" (8M/16M/16M/16M)
- "DESKTOP_ULTRA" (16M/16M/16M/16M)

---

## Architecture

### Database Structure

**Phase 1 - BFS**: Depth 0-5
- Goal states: 17 (1 solved + 16 free pairs)
- Nodes at depth 5: 5,652,685

**Phase 2-5 - Local Expansion**: Depth 6-9
- Each depth: ~943K nodes (bucket-based)
- Depth 6: bucket_6 (1M nodes)
- Depth 7: bucket_7 (1M nodes)
- Depth 8: bucket_8 (2M nodes for standard config)
- Depth 9: bucket_9 (2M nodes for standard config)

### Free Pair Algorithms

16 free pair states generated from combinations of:
- **Algorithms**: `L U L'`, `L U' L'`, `B' U B`, `B' U' B`
- **AUF variants**: `""`, `U`, `U2`, `U'`
- **Total**: 4 algorithms × 4 AUF = 16 states

**Critical Property**: Free pair algorithms only affect F2L slots, NOT cross edges.

---

## Testing Checklist

### Before Deployment

- [ ] Build completes without errors
- [ ] WASM files generated (solver_prod.js + solver_prod.wasm)
- [ ] Test with "1M/1M/2M/2M" configuration
- [ ] Verify database construction completes
- [ ] Test scramble generation (should not return actual=-1)
- [ ] Check memory usage stays within expected limits
- [ ] Test on target mobile devices

### Verification

```javascript
// Test database construction
const solver = new Module.xxcross_search(true, "1M/1M/2M/2M");
// Should complete without errors in browser console

// Test scramble generation
const scramble = solver.get_xxcross_scramble("", 12);
console.log(scramble);
// Should return valid scramble, not "actual=-1"
```

---

## Bug Fixes Included

This production version includes all critical bug fixes:

1. **Free Pair Goal States** ✅
   - Cross edges no longer modified by free pair algorithms
   - Fixed prune1 values (no more prune1=6 errors)

2. **Bucket Configuration** ✅
   - Correct depth mapping (6-9 instead of 7-10)

3. **BFS Depth** ✅
   - BFS stops at depth 5 (not 6)

4. **Phase 4 Duplicate Checking** ✅
   - Removed incorrect depth_5_nodes check

5. **Move Table Indexing** ✅
   - Uses correct `table[index * 18 + m]` pattern

---

## Performance Expectations

### Database Construction Time

Typical browser performance:
- Depth 0-5 (BFS): 5-10 seconds
- Depth 6-9 (Local): 2-5 seconds per depth
- **Total**: ~20-30 seconds

### Scramble Generation

- **First attempt success rate**: >95%
- **Retry count**: Typically 0-2 retries
- **No more actual=-1 errors**

### Memory Usage

Measured heap usage (stable):
| Config | Single Heap | Dual Heap |
|--------|-------------|-----------|
| 1M/1M/1M/1M | 332 MB | 664 MB |
| 1M/1M/2M/2M | 332 MB | 664 MB |
| 1M/1M/2M/4M | 398 MB | 796 MB |

---

## File Structure

```
production/
├── bucket_config.h              # Tier configuration definitions
├── expansion_parameters.h       # Database parameters
├── memory_calculator.h          # Memory estimation utilities
├── solver_prod_stable_20260108.cpp  # Main production code
├── build_production.sh          # Build script
├── worker_prod.js               # Web Worker template
├── solver_prod.js               # Generated WASM JavaScript
├── solver_prod.wasm             # Generated WASM binary
└── tsl/                         # robin_set hash table library
```

---

## Troubleshooting

### Build Fails

1. Check Emscripten SDK is activated:
   ```bash
   source ~/emsdk/emsdk_env.sh
   ```

2. Verify compiler version:
   ```bash
   emcc --version
   ```

### Database Construction Fails

1. Check browser console for memory errors
2. Try lower tier configuration (1M/1M/1M/1M)
3. Ensure browser supports WASM with SIMD

### Scramble Generation Returns -1

1. Hard refresh browser (Ctrl+Shift+R)
2. Clear browser cache
3. Rebuild WASM module
4. Check that production version is being used

---

## Support

For issues or questions:
1. Check browser console for error messages
2. Verify WASM files are loaded correctly
3. Test with default configuration first
4. Review implementation_progress.md for details

**Last Production Build**: 2026-01-08 00:39  
**Compiler**: Emscripten 4.0.11  
**Optimization**: -O3 -msimd128 -flto
