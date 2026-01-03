# DEPRECATED - See WASM_INTEGRATION_GUIDE.md

**⚠️ This file has been superseded and describes LEGACY workflows.**

**Current Documentation**: [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md)

---

## ⚠️ Deprecation Notice (Updated 2026-01-03)

**This document describes the LEGACY heap measurement workflow** using:
- `build_wasm_unified.sh` → `solver_heap_measurement.{js,wasm}` → `wasm_heap_advanced.html`
- These files are now **archived** in `backups/`

**Current Production Approach** (stable_20260103):
- **Build**: `em++ solver_dev.cpp -o solver_dev.js` (direct compilation, no build script)
- **Output**: `solver_dev.{js,wasm}`
- **Test page**: `test_wasm_browser.html`
- **Worker**: `worker_test_wasm_browser.js`

**Why This Document Is Preserved**:
1. Understanding the methodology used to derive the 6-tier WASM bucket model
2. Historical reference for the measurement campaign (Dec 2025 - Jan 2026)
3. Reproducing measurements if needed for validation

**For Current Development**, see:
- [WASM_BUILD_GUIDE.md](WASM_BUILD_GUIDE.md) - Current build instructions
- [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md) - Production patterns
- [test_wasm_browser.html](../../test_wasm_browser.html) - Current test page

The new guide consolidates:
- Solver integration methodology (C++ to WASM compilation)
- HTML structure and trainer configuration
- Memory tier selection and auto-detection
- Measurement analysis and validation
- Troubleshooting and best practices
- Complete code samples for production use

---

## Original Content (For Historical Reference)

# WASM Heap Measurement Guide (Unified Build)

**Updated**: 2026-01-03  
**Approach**: Single WASM binary, runtime bucket configuration via UI

This directory contains tools for measuring actual WASM heap usage across different bucket configurations to guide deployment model selection.

## Quick Start (Recommended)

### 1. Build Unified WASM (~5-10 minutes)

```bash
# From workspace root
cd src/xxcrossTrainer
./build_wasm_unified.sh
```

Generates: `solver_heap_measurement.js` + `solver_heap_measurement.wasm` (~382 KB total)

### 2. Start Local Web Server

```bash
python3 -m http.server 8000
```

### 3. Run Measurements (~15 minutes for 6 configs)

1. Open http://localhost:8000/wasm_heap_measurement_unified.html
2. For each of 6 preset configurations:
   - Click preset button (e.g., "Mobile HIGH (4M/4M/4M/2M)")
   - Click "▶ Run Test"
   - Wait for completion (~20-60 seconds)
   - **Record peak heap from results table**
   - Click "💾 Download Results" to save CSV
3. Fill results into [wasm_heap_measurements.md](../../docs/developer/wasm_heap_measurements.md)

### 4. Update Configuration

Update [bucket_config.h](bucket_config.h) with measured WASM heap values.

---

## Key Benefits of Unified Build

✅ **Single build** (~5 min) instead of 6 builds (~30-60 min)  
✅ **Small deployment** (~382 KB) instead of ~2-4 MB  
✅ **No binary switching** - all configs testable from one HTML  
✅ **Flexible experimentation** - custom bucket sizes via UI inputs

---

## File Descriptions

### Build Scripts

| File | Purpose |
|------|---------|
| `build_wasm_heap_measurement.sh` | Single-config WASM builder with bucket parser |
| `build_all_configs.sh` | Batch build all 6 configurations |

### Test Harness

| File | Purpose |
|------|---------|
| `wasm_heap_test.html` | Browser-based measurement interface |

### Documentation

| File | Purpose |
|------|---------|
| `docs/developer/wasm_heap_measurement_plan.md` | Measurement strategy |
| `docs/developer/wasm_heap_measurements.md` | Results template (fill in) |
| `docs/developer/IMPLEMENTATION_PROGRESS.md` | Phase 7.3.3 tracking |

---

## Technical Details

### Why WASM Heap ≠ Native RSS

1. **Non-contiguous heap**: Emscripten doesn't allocate contiguous memory blocks
2. **JS overhead**: JavaScript engine metadata, V8 internals
3. **WASM linear memory**: Page-aligned, 64KB granularity
4. **Allocator differences**: dlmalloc vs system allocator
5. **JIT overhead**: WebAssembly JIT compilation artifacts

**Expected overhead**: 1.8x - 2.5x (varies by configuration size)

### Heap Monitoring API

Uses Emscripten's runtime API:
```cpp
size_t emscripten_get_heap_size();      // Total allocated heap
size_t emscripten_get_free_heap_size(); // Free within heap
```

**8 Monitoring Checkpoints**:
1. Phase 1 Complete (BFS depths 0-7)
2. Phase 2 Complete (Local expansion bucket 8)
3. Phase 3 Complete (Local expansion bucket 9)
4. **Phase 4 Peak** ← **CRITICAL** (depth8_set active, native 414 MB)
5. Phase 4 Complete (bucket 10 finished)
6. Phase 5 Start (before depth9_set)
7. Phase 5 After depth9_set
8. Final Cleanup Complete

**Peak typically occurs at Phase 4 Peak** (all major sets active simultaneously)

### Memory Growth Behavior

WASM heap has unique characteristics:
- **Grows**: Allocates 64KB pages on demand
- **Never shrinks**: Free doesn't return memory to OS
- **Final size = Peak size** (simplifies measurement)

This means we only need to record the final heap size after solver completes!

---

## Troubleshooting

### Build Errors

**Error: `emcc: command not found`**
```bash
# Activate Emscripten manually
source ~/emsdk/emsdk_env.sh
emcc --version  # Verify
```

**Error: `solver_dev.cpp: No such file`**
```bash
# Ensure you're in correct directory
# From workspace root
cd src/xxcrossTrainer
pwd  # Should end with /xxcrossTrainer
```

**Error: `--pre-js: file not found`**
- Fixed in latest version (uses temp file instead of process substitution)
- Re-download build script from repository

### Browser Errors

**Error: `Failed to load WASM module script`**
- Check browser console for actual file path
- Verify files exist: `ls solver_*.{js,wasm}`
- Confirm web server is running: `http://localhost:8000/`

**Error: `Module initialization failed`**
- Check for CORS errors (use proper web server, not file://)
- Verify WASM file size (should be >1 MB)
- Try in Chrome Incognito (disable extensions)

**Heap size shows 0 MB**
- Ensure WASM runtime initialized (check console for "✅ WASM Runtime initialized")
- Check for JavaScript errors in console
- Verify `__EMSCRIPTEN__` was defined during build

### Memory Issues

**Browser crashes/freezes**
- Desktop configs may exceed browser tab limits (~2 GB)
- Test smaller configs first (2M, 4M)
- Close other tabs
- Use Chrome with increased heap limit: `google-chrome --js-flags="--max-old-space-size=8192"`

**Heap usage higher than expected**
- Normal! WASM has 1.8-2.5x overhead
- Verify with Chrome DevTools Memory Profiler
- Compare across multiple runs (variance ±5% is normal)

---

## Expected Timeline

| Phase | Task | Duration |
|-------|------|----------|
| 1 | Build all configs | 30-60 min |
| 2 | Run 6 measurements | 10-15 min |
| 3 | Verify with DevTools | 20-30 min |
| 4 | Analyze results | 30-60 min |
| 5 | Update bucket_config.h | 15-30 min |
| 6 | Documentation | 30-60 min |
| **Total** | | **~2.5-4 hours** |

---

## Next Steps After Measurement

1. **Update bucket_config.h**
   - Replace placeholder WASM heap values
   - Add measured overhead factors
   - Update model selection logic

2. **Update MEMORY_BUDGET_DESIGN.md**
   - Add WASM deployment section
   - Document measured overhead trends
   - Provide deployment guidelines

3. **Test Model Selection**
   - Create test harness for automatic model selection
   - Validate fallback logic
   - Test edge cases (low-memory devices)

4. **Production Deployment**
   - Build production WASM with selected models
   - Add runtime heap monitoring
   - Implement graceful degradation

---

## References

- [WASM Heap Measurement Plan](docs/developer/wasm_heap_measurement_plan.md)
- [Memory Spike Investigation](docs/developer/depth_10_memory_spike_investigation.md)
- [Bucket Configuration Design](docs/developer/MEMORY_BUDGET_DESIGN.md)
- [Emscripten API Docs](https://emscripten.org/docs/api_reference/index.html)

---

**Questions?** See [IMPLEMENTATION_PROGRESS.md](docs/developer/IMPLEMENTATION_PROGRESS.md) Phase 7.3.3
