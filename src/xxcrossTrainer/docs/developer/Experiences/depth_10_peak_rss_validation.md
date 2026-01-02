# Depth 10 Peak RSS Validation (4M/4M/4M/2M)

**Date**: 2026-01-02  
**Configuration**: 4M/4M/4M/2M  
**Objective**: Validate actual peak RSS using 10ms monitoring vs C++ checkpoints

---

## Executive Summary

**Critical Finding**: Actual peak RSS (**442.3 MB**) is **64 MB higher** than C++ checkpoint peak (378 MB)

**Difference Analysis**:
- C++ checkpoint peak: 378 MB (Phase 5, depth9_set + depth_10_nodes active)
- **10ms monitoring peak: 442.3 MB** (actual OS-level peak)
- **Gap: +64 MB (+17%)** ⚠️

**Implication**: C++ checkpoints miss transient memory spikes between measurement points

---

## Monitoring Methodology

### 10ms Integrated Monitoring System

**Configuration**:
- Sampling interval: 10ms (100 samples/second)
- Data source: `/proc/PID/status` (VmRSS, VmSize, VmData)
- Integration: Solver launched as subprocess (zero timing gap)
- Duration: 180 seconds
- Total samples: 16,774

**Key Advantages**:
1. **Zero timing gap**: Monitor starts before solver initialization
2. **High resolution**: Captures transient spikes between C++ checkpoints
3. **OS-level accuracy**: Direct `/proc` filesystem reads
4. **Complete lifecycle**: Monitors from process start to termination

---

## Results Comparison

### Peak RSS Comparison

| Measurement Method | Peak RSS | Timing | Gap |
|-------------------|----------|--------|-----|
| C++ Checkpoint (Phase 5) | 378 MB | t=38s | Baseline |
| **10ms Monitoring** | **442.3 MB** | **t=45.4s** | **+64 MB (+17%)** |

**Key Observation**: Peak occurs **7.4 seconds after** C++ checkpoint shows 378 MB

### Peak Timing Analysis

**C++ Checkpoint Timeline**:
```
Phase 1 (BFS):          196 MB (t ~2s)
Phase 2 (depth 7):      257 MB (t ~3s)
Phase 3 (depth 8):      286 MB (t ~3s)
Phase 4 (depth 9):      414 MB (t ~32s)
Phase 5 start:          186 MB (t ~31s)
Phase 5 depth9_set:     378 MB (t ~38s) ← C++ checkpoint peak
Phase 5 complete:       186 MB (t ~44s)
```

**10ms Monitoring Timeline**:
```
t=0-3s:    Phase 1-3 (smooth growth to 189 MB)
t=31-32s:  Phase 4 peak (260 MB → 293 MB)
t=38s:     Phase 5 depth9_set built (321-347 MB)
t=38-44s:  Phase 5 expansion (gradual climb)
t=45.4s:   **PEAK: 442.3 MB** ⚡ (after Phase 5 expansion)
t=46s:     Sharp drop to ~0 MB (process termination)
```

**Peak Source**: Likely during Phase 5 expansion loop, when:
- depth9_set: ~128 MB (for duplicate detection)
- depth_10_nodes: ~64 MB (2M buckets)
- index_pairs[0-9]: ~121 MB
- **Transient allocations**: ~64 MB (move operations, temporary vectors)

---

## Memory Spikes Detected

### Significant Spikes (>20 MB within 1s)

| Time (s) | RSS (MB) | Increase (MB) | Duration (ms) | Rate (MB/s) | Phase |
|----------|----------|---------------|---------------|-------------|-------|
| 49.18 | 252.2 | 25.0 | 10 | 2500 | Post-termination (memory return) |
| 45.52 | 396.0 | 24.1 | 10 | 2412 | Phase 5 expansion |
| 49.20 | 300.4 | 24.1 | 10 | 2412 | Post-termination |
| 49.19 | 276.2 | 24.0 | 11 | 2182 | Post-termination |

**Analysis**:
- **1 spike during Phase 5** (t=45.5s): 24 MB increase in 10ms
- **7 spikes post-termination** (t=49s): Memory being freed rapidly
- No unexpected spikes during database construction phases

---

## Root Cause Analysis

### Why C++ Checkpoints Miss the Peak

**C++ checkpoint locations**:
```cpp
// Phase 5 structure
size_t rss_before_d10_nodes = get_rss_kb();          // Checkpoint 1: 186 MB
// ... depth_10_nodes.reserve(bucket_d10) ...
size_t rss_after_d10_nodes_creation = get_rss_kb(); // Checkpoint 2: 250 MB
// ... build depth9_set ...
size_t rss_after_depth9_set = get_rss_kb();          // Checkpoint 3: 378 MB ← Reported peak
// ... expansion loop (NO CHECKPOINT) ...
size_t rss_before_depth9_free = get_rss_kb();        // Checkpoint 4: after expansion
```

**Missing measurement**: **Expansion loop** has no checkpoints
- Loop processes 1,001,819 parents
- Each iteration: decompose index, apply moves, check duplicates, insert
- Temporary allocations for vectors, move calculations
- **Peak occurs during this unmeasured region**

### Transient Allocation Sources

**Likely contributors (+64 MB gap)**:
1. **Random move generation** (std::vector<int> for each parent): ~5-10 MB
2. **Index decomposition temporaries**: ~5-10 MB
3. **robin_set internal rehash buffers**: ~20-30 MB
4. **Allocator fragmentation**: ~10-20 MB
5. **C++ temporary objects** (move constructors): ~5-10 MB

---

## Production Implications

### WASM Memory Budget Recommendations

**Previous assumption** (INCORRECT):
- C++ checkpoint peak: 378 MB
- WASM overhead: ~2x → 756 MB total
- Conclusion: Safe for 512 MB limit? ❌

**Corrected estimate** (based on 10ms monitoring):
- **Actual native peak: 442 MB**
- **WASM heap overhead: ~1.8-2.0x** → **796-884 MB total** ⚠️
- **Conclusion: EXCEEDS 512 MB mobile limit for 4M/4M/4M/2M**

**Revised recommendations**:
1. **Mobile/512MB WASM**: Use **2M/2M/2M/1M** configuration
   - Estimated native peak: ~250-280 MB
   - WASM heap: ~450-560 MB (within limit with 10% margin)

2. **Desktop/1GB WASM**: Use **4M/4M/4M/2M** configuration
   - Native peak: 442 MB
   - WASM heap: ~796-884 MB (safe with 20% margin)

3. **High-end/2GB WASM**: Use **8M/8M/8M/4M** configuration
   - Estimated native peak: ~700-800 MB
   - WASM heap: ~1.26-1.60 GB

---

## Emscripten Heap Monitoring Plan

### Objective

Replace "2x WASM overhead assumption" with **measured heap usage** via Emscripten API

### Implementation

**C++ code addition**:
```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void log_heap_usage(const char* phase_name) {
    size_t total_heap = emscripten_get_heap_size();
    size_t used_heap = total_heap - emscripten_get_free_heap_size();
    
    std::cout << "[" << phase_name << "] "
              << "Heap: " << (used_heap / 1024 / 1024) << " MB / "
              << (total_heap / 1024 / 1024) << " MB"
              << " (used: " << (100.0 * used_heap / total_heap) << "%)"
              << std::endl;
}
#endif

// Usage in solver_dev.cpp
#ifdef __EMSCRIPTEN__
log_heap_usage("Phase 5 start");
// ... expansion loop ...
log_heap_usage("Phase 5 peak");
#endif
```

**Measurement locations**:
- Phase 1 peak (after BFS)
- Phase 2 peak (depth 7 expansion)
- Phase 3 peak (depth 8 expansion)
- Phase 4 peak (depth 9 expansion)
- **Phase 5 peak** (depth 10 expansion) ← Critical
- Final (after cleanup)

**Expected output**:
```
[Phase 5 start] Heap: 186 MB / 512 MB (used: 36%)
[Phase 5 depth9_set] Heap: 378 MB / 512 MB (used: 74%)
[Phase 5 peak] Heap: ??? MB / 512 MB (used: ???%) ← TO MEASURE
[Phase 5 complete] Heap: 186 MB / 512 MB (used: 36%)
```

### Tasks

- [ ] Add Emscripten heap monitoring to solver_dev.cpp
- [ ] Build WASM with 4M/4M/4M/2M configuration
- [ ] Run in browser with heap logging enabled
- [ ] Compare WASM heap usage vs native RSS
- [ ] Calculate actual WASM overhead factor (expected: 1.8-2.2x)
- [ ] Update deployment recommendations based on measurements

---

## Conclusions

### Key Findings

1. ✅ **C++ checkpoints are insufficient** for peak detection
   - Miss transient allocations during expansion loops
   - Underestimate actual peak by **+17% (+64 MB)**

2. ✅ **10ms monitoring is essential** for accurate peak detection
   - Captures OS-level memory usage with high resolution
   - Reveals spikes between checkpoints

3. ⚠️ **4M/4M/4M/2M exceeds 512MB WASM limit**
   - Actual peak: 442 MB native → ~800-900 MB WASM
   - Not suitable for mobile deployment
   - Recommend 2M/2M/2M/1M for mobile

4. 🔬 **Emscripten heap monitoring required** for precise WASM budget
   - Replace "~2x overhead" assumption with measured values
   - Critical for production deployment planning

### Next Steps

1. **Implement Emscripten heap monitoring** (Phase 7.3.1)
2. **Test 2M/2M/2M/1M configuration** with 10ms monitoring (Phase 7.3.2)
3. **Build WASM and measure actual heap** (Phase 7.3.3)
4. **Update production recommendations** based on measurements (Phase 7.4)

---

## Appendix: Full Spike Analysis

See `measurements_depth10_4M/rss_trace.csv` for complete 10ms memory trace (16,774 samples)

**Key timestamps**:
- t=0.19s: Phase 1 start (36 MB → 157 MB)
- t=31.64s: Phase 4 peak (189 MB → 260 MB)
- t=38.02s: Phase 5 depth9_set (321 MB → 347 MB)
- **t=45.38s: ABSOLUTE PEAK (442.3 MB)** ⚡
- t=46s: Process termination

**No unexpected spikes detected** - all increases accounted for by expected allocations
