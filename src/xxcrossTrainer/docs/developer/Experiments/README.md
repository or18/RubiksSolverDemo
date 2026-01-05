# Experimental Findings Archive

**Version**: stable_20260103  
**Last Updated**: 2026-01-03

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../../USER_GUIDE.md)
>
> **Related**: [Implementation](../SOLVER_IMPLEMENTATION.md) | [Memory Monitoring](../MEMORY_MONITORING.md)

---

## Purpose

This directory contains **current experimental findings** from memory behavior investigations and WASM deployment measurements. Older comprehensive measurement campaigns have been archived to `docs/developer/_archive/Experiments_old/`.

---

## Current Documents

### 🌐 WASM Heap Measurement Campaign (2026-01-03) - LATEST ✅

**[wasm_heap_measurement_results.md](wasm_heap_measurement_results.md)** - **6-Tier WASM Bucket Model (Production-Ready)**
- **13 bucket configurations measured** (1M/1M/2M/4M to 16M/16M/16M/16M)
- **Platform-independent validation**: Mobile and Desktop browsers show identical heap usage
- **Final 6-tier model**: Mobile LOW/MIDDLE/HIGH, Desktop STD/HIGH/ULTRA
- **Key efficiency finding**: 8M/8M/8M/8M achieves 45,967 nodes/MB (best in class)
- **Heap anomaly explained**: Larger buckets reduce rehashing overhead (756 MB < 811 MB)
- **Dual-solver architecture**: Total heap = 2× single solver (adjacent + opposite F2L slots)
- **Deployment recommendations**: Desktop STD (1512 MB) for standard use, Mobile LOW (618 MB) for budget phones
- **Implementation complete**: bucket_config.h updated with WASM_MOBILE_LOW/MIDDLE/HIGH, WASM_DESKTOP_STD/HIGH/ULTRA

**[wasm_heap_measurement_data.md](wasm_heap_measurement_data.md)** - **Raw Data and Detailed Observations**
- Complete 13-configuration measurement table
- Per-configuration statistics (nodes, heap, efficiency)
- Measurement extraction scripts
- Detailed observation notes
- Data backup locations

**Production Status**: ✅ Complete - Ready for trainer integration (Phase 7.5)

---

### 🔧 Depth 10 Implementation Results (2026-01-02)

**[depth_10_implementation_results.md](depth_10_implementation_results.md)** - **Phase 5 depth expansion proof-of-concept**
- Successful implementation of depth 9→10 local expansion
- 4M/4M/4M/2M configuration tested: **1,887,437 nodes at depth 10**
- Coverage improvement: 30% → ~85% (+185% relative increase)
- Final RSS: **138 MB** (peak 378 MB during Phase 5)
- **Critical bug fixes documented**:
  - Segmentation fault (composite index decomposition)
  - Rehash detection error
  - Element vector management (attach timing fix)
- Performance metrics: ~15-20 seconds for Phase 5 expansion
- **Recommended for understanding depth 10 implementation**

**[depth_10_peak_rss_validation.md](depth_10_peak_rss_validation.md)** - **10ms monitoring validation**
- **Critical finding**: Actual peak RSS **442 MB** (not 378 MB from C++ checkpoints)
- Gap analysis: +64 MB (+17%) from transient allocations
- 10ms integrated monitoring captures spikes between checkpoints
- **WASM implication**: 4M/4M/4M/2M likely **exceeds 512MB mobile limit**
- Emscripten heap monitoring plan to measure actual WASM overhead
- **Required reading for production deployment planning**

**[depth_10_memory_spike_investigation.md](depth_10_memory_spike_investigation.md)** - **Memory spike elimination investigation** ✅
- **Comprehensive optimization attempts**: 5 techniques tested
  - Phase 4 vector pre-allocation (moved outside loop)
  - Phase 5 depth9_set direct insertion (eliminated 30 MB vector copy)
  - Bulk insert optimization
  - malloc_trim after Phase 4
  - Enhanced C++ checkpoints (periodic RSS during expansion loops)
- **Root cause discovery**: Phase 4 is actual theoretical peak (414 MB), not Phase 5
- **Resolution**: 64 MB "spike" was measurement artifact (Phase 5 checkpoint vs global 10ms peak)
- **Actual spike**: 28 MB system overhead (unavoidable kernel/allocator level)
- **WASM margin calculation**: Now accurate based on 414 MB theoretical peak
- **Pre-reserve pattern**: Eliminates all transient spikes for production deployment

**Key Achievement**: Depth 10 support validated across WASM tiers (Mobile LOW to Desktop ULTRA)

---

### 📊 Depth 10 Expansion Design (2026-01-02)

**[depth_10_expansion_design.md](depth_10_expansion_design.md)** - **Strategic design document**
- Analysis of depth 10 as volume peak (60-70% of all xxcross solutions)
- 3 implementation options evaluated
- Memory vs coverage trade-offs
- Recommended configurations for different platforms
- **Context for depth 10 implementation decisions**

---

### 🔬 Bucket Model RSS Measurement (2026-01-02)

**[bucket_model_rss_measurement.md](bucket_model_rss_measurement.md)** - **Primary reference for production deployment**
- Face-diverse expansion implementation and validation
- Three bucket models tested: 2M/2M/2M, 4M/4M/4M, 8M/8M/8M
- **Deep dive: Allocator cache investigation**
  - malloc_trim() analysis
  - True overhead <4% after cache cleanup
  - Phase-by-phase memory tracking
  - No memory leaks detected ✅
- Production recommendations based on malloc_trim findings
- **Best for understanding current implementation and deployment**

**Key Findings**:
- **Without malloc_trim()**: 8-20% overhead (allocator cache)
- **With malloc_trim()**: 0.9-3.3% overhead (true overhead)
- All models achieve 90% load factor consistently
- 8M/8M/8M recommended for production (225 MB after trim)

---

### ⚡ Peak RSS Optimization & Memory Spike Investigation (2026-01-02) - LATEST

**[peak_rss_optimization.md](peak_rss_optimization.md)** - **Comprehensive optimization and validation study**
- Phase 4 peak breakdown and analysis
- depth8_vec removal optimization (-58 MB, -8.8%)
- depth_6_nodes necessity validation
- Future optimization opportunities
- Emergency safety mechanisms (documented, not implemented)
- **NEW: 10ms Spike Investigation Results**
  - Integrated monitoring system (zero timing gap)

---

### 🌐 malloc_trim() WASM Compatibility (2026-01-02) - LATEST

**[malloc_trim_wasm_verification.md](malloc_trim_wasm_verification.md)** - **Cross-platform verification**
- Native vs WASM malloc_trim() behavior
- Test results (3 scenarios):
  - Simple allocation: 0 MB freed (already freed by delete[])
  - Vector resize: 0 MB freed (already freed by delete)
  - Multiple cycles: 10 MB freed ✅
- **WASM test execution results** (Emscripten 4.0.11)
- Platform-specific implementation (#ifndef __EMSCRIPTEN__)
- **NEW: disable_malloc_trim option for WASM-equivalent measurements**
  - Problem: Native WITH trim underestimates WASM by ~225 MB
  - Solution: Runtime option to skip malloc_trim() on native
  - Use case: Accurate WASM memory prediction
- Production deployment guidance

**Key Findings**:
- malloc_trim() reduces native RSS by 225 MB (50%)
- WASM cannot use malloc_trim() (Emscripten limitation)
- Native WASM-equivalent measurement: DISABLE_MALLOC_TRIM=1
- Allocator cache: ~16 MB (8M/8M/8M)

---

### 📖 disable_malloc_trim Usage Guide (2026-01-02) - NEW

**[disable_malloc_trim_usage.md](disable_malloc_trim_usage.md)** - **WASM deployment planning guide**
- Problem statement: Native measurements underestimate WASM memory
- Solution: WASM-equivalent measurement mode on native
- Usage examples and decision tree
- Memory comparison table (native vs WASM-equivalent vs WASM)
- Example workflow: WASM deployment planning
- Common mistakes and FAQ

**When to Use**:
- ✅ Measuring bucket models for WASM deployment
- ✅ Predicting WASM memory requirements
- ❌ Production native deployment (use default settings)

**Example**:
```bash
# WASM-equivalent measurement
DISABLE_MALLOC_TRIM=1 BUCKET_MODEL=8M/8M/8M ENABLE_CUSTOM_BUCKETS=1 ./solver_dev
# → RSS: 241 MB (matches WASM heap)

# Normal native production
BUCKET_MODEL=8M/8M/8M ENABLE_CUSTOM_BUCKETS=1 ./solver_dev
# → RSS: 225 MB (optimized)
```

---

### 🚀 Depth 10 Local Expansion Design (2026-01-02) - NEW

**[depth_10_expansion_design.md](depth_10_expansion_design.md)** - **Next major feature implementation**
- Background: Depth 10 is the volume peak (~60-70% of all solutions)
- Current limitation: depth 0-9 only (~30% coverage)
- Three implementation options:
  - **Option A**: Direct extension (simplest, 32M bucket, +256 MB)
  - **Option B**: Hybrid pruning (recommended, 16M bucket, +128 MB)
  - **Option C**: Progressive two-stage (most optimized, 8M bucket, +64 MB)
- Memory budget analysis:
  - 8M/8M/8M/32M: Native safe, **WASM exceeds 512MB limit** ❌
  - 8M/8M/8M/16M: Native safe, WASM tight (115%) ⚠️
  - **4M/4M/4M/16M: Both safe (96% WASM utilization)** ✅
- Implementation roadmap (3 phases)
- Risk assessment and mitigation strategies

**Key Insights**:
- Depth 10 is THE MOST IMPORTANT depth for xxcross search
- Need 4-depth bucket configuration (current: 3-depth)
- WASM deployment requires careful memory management
- Recommended: Start with Option A (proof of concept), optimize to Option B if needed

**Status**: Design phase complete, ready for implementation

---
  - 16,971 samples at 10ms intervals
  - C++ checkpoint accuracy validated: 656.69 MB vs 657 MB (0.05% difference)
  - 11 memory spikes detected (all normal allocation behavior)
  - **No transient spikes beyond checkpoint values**
  - Production safety confirmed: 657 MB true peak, 943 MB margin (59%)

**Key Findings (Spike Investigation 2026-01-02)**:
- **Hypothesis tested**: "C++ checkpoints may miss high-speed transient spikes"
- **Result**: FALSE - Checkpoints capture true peaks, not "ideal case" approximations
- **Validation method**: 10ms /proc-based monitoring (16,971 samples over 180s)
- **Largest spike**: +27 MB in 10ms during BFS depth 6 (normal bucket allocation)
- **Conclusion**: Checkpoint methodology is comprehensive and accurate ✅

**Production Status**:
- Peak RSS: 657 MB (8M/8M/8M, validated by 10ms monitoring)
- Final RSS: 225 MB (after malloc_trim)
- Safety margin: 943 MB (59% of 1600 MB limit)
- System stable, predictable, and thoroughly validated ✅
- **Ready for production deployment**

**Tools & Documentation**:
- [MEMORY_MONITORING.md](../MEMORY_MONITORING.md#integrated-monitoring-system-2026-01-02) - Monitoring infrastructure
- `tools/memory_monitoring/run_integrated_monitoring.py` - Integrated monitoring system
- `backups/logs/memory_monitoring_20260102/` - Detailed investigation logs

**Deprecated Documents** (consolidated into peak_rss_optimization.md):
- `memory_spike_analysis.md` → Moved to backups (VERBOSE checkpoint validation)
- `spike_investigation_summary.md` → Moved to backups (initial investigation notes)

---

### 🧪 malloc_trim() WASM Compatibility Verification (2026-01-02)

**[malloc_trim_wasm_verification.md](malloc_trim_wasm_verification.md)** - **WASM environment compatibility study**
- Test file: `test_malloc_trim_wasm.cpp`
- Native vs WASM malloc_trim() behavior comparison
- 3 test cases: simple allocation, vector resize, multiple cycles
- **Key Finding**: malloc_trim() reduces native RSS by 225 MB (50%)
- **WASM Status**: malloc_trim() NOT available (Emscripten limitation)
- **Conclusion**: Current #ifndef __EMSCRIPTEN__ guard is correct ✅

**Native Results**:
- malloc_trim() effective after multiple allocation cycles
- Reduces final RSS from ~450 MB to ~225 MB
- Critical for production native deployment

**WASM Results**:
- malloc_trim() not available in Emscripten
- Memory managed by Emscripten runtime
- Final heap ~450 MB (no trim benefit)
- Still acceptable for browser environment

**Implementation Status**: ✅ No changes needed - current code is correct

---

### 📊 Comprehensive Measurement Campaign (2025-12-30) - ARCHIVED

Older comprehensive measurement documents have been archived locally (not in git).

Historical documents from December 2025 measurement campaign:
- `MEASUREMENT_RESULTS_20251230.md` - 47-point comprehensive results
- `MEASUREMENT_SUMMARY.md` - Executive summary
- `MEMORY_THEORY_ANALYSIS.md` - Theoretical deep-dive
- `THEORETICAL_MODEL_REVISION.md` - Model evolution
- `bucket_model_rss_measurement_old.md` - Earlier bucket measurements

These archived documents are available for historical reference but are superseded by the current documents above.

---

## Document Relationships

```
Production Deployment
├─ ../../USER_GUIDE.md (Use this for deployment + Advanced Configuration)
│
Current Findings (2026-01-02)
├─ bucket_model_rss_measurement.md (Face-diverse expansion, malloc_trim analysis)
├─ peak_rss_optimization.md (Optimization + spike investigation)
└─ ../MEMORY_MONITORING.md (Monitoring infrastructure)

Archived Findings (2025-12-30)
└─ ../_archive/Experiments_old/
   ├─ MEASUREMENT_RESULTS_20251230.md
   ├─ MEASUREMENT_SUMMARY.md
   ├─ MEMORY_THEORY_ANALYSIS.md
   └─ THEORETICAL_MODEL_REVISION.md
```

---

## Reading Guide

### For Production Deployment
1. **[../../USER_GUIDE.md](../../USER_GUIDE.md)** - Complete production guide with WASM tiers
2. **[bucket_model_rss_measurement.md](bucket_model_rss_measurement.md)** - Current implementation details

### For Understanding Memory Behavior
1. **[peak_rss_optimization.md](peak_rss_optimization.md)** - Optimization strategies and spike validation
2. **[bucket_model_rss_measurement.md](bucket_model_rss_measurement.md)** - Allocator behavior and malloc_trim
3. **[../MEMORY_MONITORING.md](../MEMORY_MONITORING.md)** - Monitoring methodology

### For Historical Context

Earlier theoretical models and comprehensive measurement campaigns are available in local archives (not in git). Contact maintainers for access to historical documents if needed.

---

## Key Findings Summary

### Current Production Configuration (2026-01-02)

**8M/8M/8M Model** (Recommended):
- Peak RSS: 657 MB (validated with 10ms monitoring)
- Final RSS: 225 MB (after malloc_trim)
- Safety margin: 943 MB (59% of 1600 MB limit)
- System validated: Stable, predictable, production-ready ✅

**Validation**:
- C++ checkpoint accuracy: ±0.05%
- No transient spikes beyond checkpoints
- 11 allocation spikes detected (all normal rehashing)
- Integrated monitoring: 16,971 samples over 180s

**Optimization Achievements**:
- depth8_vec removal: -58 MB (-8.8%)
- Allocator cache cleanup: +225 MB reduction (after malloc_trim)
- True overhead: <4% (down from 8-20% before cleanup)

---

## Experimental Data Archive

Measurement data and experimental logs are archived in:
```
src/xxcrossTrainer/backups/logs/
├─ experiments_20260102/          (Latest experiments)
│  ├─ measurements_20260102_*/    (10ms monitoring runs)
│  ├─ spike_analysis_verbose/     (VERBOSE checkpoint validation)
│  ├─ spike_detection/            (Spike detection test runs)
│  ├─ model_*M.log               (Model measurement logs)
│  └─ *.csv                      (RSS trace data)
└─ memory_monitoring_20260102/    (Spike investigation archive)
   ├─ 10MS_SPIKE_INVESTIGATION.md
   ├─ memory_spike_analysis_deprecated.md
   └─ spike_investigation_summary_deprecated.md
```

**Note**: Experimental data is excluded from git repository (see .gitignore). Data remains available locally for reference.

---

## Related Documentation

### In Parent Directory
- **[../SOLVER_IMPLEMENTATION.md](../SOLVER_IMPLEMENTATION.md)** - Code implementation
- **[../MEMORY_MONITORING.md](../MEMORY_MONITORING.md)** - Monitoring techniques and tools
- **[../EXPERIMENT_SCRIPTS.md](../EXPERIMENT_SCRIPTS.md)** - Testing scripts
- **[../IMPLEMENTATION_PROGRESS.md](../IMPLEMENTATION_PROGRESS.md)** - Development tracking

### In Root docs/
- **[../../README.md](../../README.md)** - Developer entry point
- **[../../USER_GUIDE.md](../../USER_GUIDE.md)** - End-user guide and production deployment

---

**Document Purpose**: This README serves as a navigation guide to current experimental findings. Historical documents are preserved in local archives (not in git) for reference.

**Status**: Current findings documented (2026-01-02), older documents archived locally.
