# Peak RSS Optimization Investigation
**Date**: 2026-01-02  
**Objective**: Reduce peak RSS during database construction through careful object lifecycle management

---

## Current Peak RSS Analysis (8M/8M/8M Model)

### Phase-by-Phase RSS Progression

| Phase | Start RSS | Peak RSS | End RSS | Objects Active at Peak |
|-------|-----------|----------|---------|------------------------|
| **Phase 1** (BFS 0-6) | 60 MB | 196 MB | 196 MB | depth_6_nodes (robin_set) |
| **Phase 2** (depth 7) | 196 MB | 413 MB | 286 MB | depth_6_nodes + depth_7_nodes |
| **Phase 3** (depth 8) | 286 MB | 657 MB | 343 MB | depth_6_nodes + depth_8_nodes + depth7_set |
| **Phase 4** (depth 9) | 343 MB | **657 MB** ⚠️ | 241 MB | All sets active |
| **Post-trim** | 241 MB | - | **225 MB** | Final (after malloc_trim) |

**Peak: Phase 4 at 657 MB** (2.96x final RSS)

### Phase 4 Peak Memory Breakdown

```
RSS at Phase 4 peak: 657 MB
Components:
  - depth_6_nodes:     127 MB (robin_set, 4.17M nodes)
  - depth_9_nodes:       8 MB (robin_set, empty but allocated 8M buckets)
  - depth8_set:         89 MB (robin_set, 7.55M nodes, 8M buckets)
  - depth8_vec:         57 MB (vector, 7.55M nodes)
  - index_pairs[0-8]:  149 MB (vectors, depths 0-8)
  ---
  Theoretical sum:     430 MB
  Actual RSS:          657 MB
  Overhead:            227 MB (52.8%)
```

**Overhead Sources**:
1. Allocator cache and metadata: ~100-150 MB
2. robin_set internal structures (beyond buckets): ~50-80 MB
3. Vector capacity reserves: minimal (capacity ≈ size)

---

## Optimization Opportunities

### Option 1: Early Release of depth_6_nodes ⚡ HIGH IMPACT

**Current**: depth_6_nodes retained through Phase 2, 3, 4 (freed only after Phase 4)

**Hypothesis**: Phase 4 expansion from depth 8 → depth 9 cannot produce depth 6 states
- Depth is monotonically increasing in optimal solves
- Depth 8 + 1 move = depth 9 or duplicate of depth 7/8
- Depth 6 check may be redundant in Phase 4

**Test**:
1. Remove `depth_6_nodes.find()` check in Phase 4 loop
2. Measure if any duplicates were caught by this check
3. If zero duplicates from depth 6 in Phase 4 → safe to remove

**Expected Savings**: **-127 MB peak RSS** (19% reduction)

**Risk**: If depth 8 + 1 move can actually reach depth 6 (backwards move), removing check could cause:
- Duplicate insertions into depth_9_nodes
- Incorrect node counts
- Search quality degradation

**Validation**:
```cpp
// Add counter in Phase 4 loop
size_t caught_by_depth6 = 0;
if (depth_6_nodes.find(next_node123_d9) != depth_6_nodes.end()) {
    caught_by_depth6++;
    continue;  // Skip to next iteration
}
// After Phase 4:
std::cout << "Duplicates caught by depth_6_nodes check: " << caught_by_depth6 << std::endl;
```

---

### Option 2: Remove depth8_vec (Use index_pairs[8] directly) ⚡ MEDIUM IMPACT

**Current**: 
```cpp
std::vector<uint64_t> depth8_vec;
depth8_vec.assign(index_pairs[8].begin(), index_pairs[8].end());
// Random sampling from depth8_vec
```

**Alternative**:
```cpp
// Direct random access to index_pairs[8]
std::uniform_int_distribution<size_t> dist_d9(0, index_pairs[8].size() - 1);
size_t random_idx = dist_d9(gen);
uint64_t parent_node = index_pairs[8][random_idx];
```

**Savings**: **-57 MB** (8.7% peak RSS reduction)

**Trade-off**: None - index_pairs[8] is already a vector with random access

**Implementation**: Simple refactor, no algorithmic change

---

### Option 3: Reduce depth8_set Overhead 🔬 LOW IMPACT

**Current**: 
- depth8_set with max_load_factor = 0.95
- 8M buckets for 7.5M nodes
- ~89 MB theoretical, but with metadata overhead

**Alternative**: Use index_pairs[8] for lookup instead of separate set
```cpp
// Instead of: depth8_set.find(node) != depth8_set.end()
// Use: std::find(index_pairs[8].begin(), index_pairs[8].end(), node)
```

**Savings**: **-89 MB** (13.5% peak RSS reduction)

**Trade-off**: 
- O(n) lookup vs O(1) - severe performance degradation
- 7.5M linear searches per expansion = impractical
- **Not recommended** unless combined with other optimizations

**Better alternative**: Use sorted vector + binary search
```cpp
std::sort(index_pairs[8].begin(), index_pairs[8].end());
bool found = std::binary_search(index_pairs[8].begin(), index_pairs[8].end(), node);
```
- O(log n) lookup
- Savings: -89 MB
- Cost: One-time sort (~100ms for 7.5M elements)

---

### Option 4: Incremental malloc_trim() 🔬 EXPERIMENTAL

**Current**: malloc_trim() called once after database construction

**Alternative**: Call malloc_trim() after each Phase cleanup
```cpp
// After Phase 2
depth_7_nodes.swap(temp);
malloc_trim(0);  // Return freed memory to OS immediately

// After Phase 3
depth_8_nodes.swap(temp);
malloc_trim(0);

// After Phase 4
depth_9_nodes.swap(temp);
malloc_trim(0);
```

**Expected Impact**: 
- May reduce peak RSS by clearing allocator cache between phases
- Trade-off: Performance cost (~10-50ms per trim call)
- **Uncertain benefit** - needs measurement

---

## Recommended Optimization Strategy

### Phase 1: Validation & Low-Risk Optimizations

**Step 1**: Validate depth_6_nodes necessity in Phase 4
- Add duplicate counter
- Measure: how many duplicates caught by depth_6 check?
- If zero → proceed to Step 2

**Step 2**: Remove depth8_vec (safe refactor)
- Use index_pairs[8] for random sampling
- Savings: -57 MB peak RSS
- Risk: None

**Step 3**: Test incremental malloc_trim()
- Measure peak RSS with trim after each phase
- If significant reduction → keep
- If minimal impact → remove (performance cost not worth it)

### Phase 2: Advanced Optimizations (if needed)

**Step 4**: Early release depth_6_nodes (if validated safe)
- Free depth_6_nodes after Phase 3
- Savings: -127 MB peak RSS
- Implement only if Step 1 confirms safety

**Step 5**: Replace depth8_set with sorted vector + binary search
- One-time sort after Phase 3
- Savings: -89 MB peak RSS
- Trade-off: Slightly slower lookups (log n vs constant)

---

## Expected Total Savings

| Optimization | RSS Reduction | Cumulative | New Peak | Status |
|--------------|---------------|------------|----------|--------|
| Baseline | - | - | 657 MB | Current |
| Remove depth8_vec | -57 MB | -57 MB | 600 MB | ✅ Safe |
| Early free depth_6_nodes | -127 MB | -184 MB | 473 MB | ⚠️ Validate |
| Replace depth8_set | -89 MB | -273 MB | 384 MB | 🔬 Experimental |
| Incremental malloc_trim | -50 MB? | -323 MB? | 334 MB? | ❓ Unknown |

**Conservative target**: **-184 MB** (473 MB peak, 28% reduction)  
**Aggressive target**: **-273 MB** (384 MB peak, 41% reduction)

---

## Implementation Plan

1. **Create validation branch** with duplicate counters
2. **Test depth_6_nodes necessity** in Phase 4
3. **Implement depth8_vec removal** (guaranteed safe)
4. **Measure results** on all 3 bucket models
5. **Document findings** in bucket_model_rss_measurement.md
6. **Update IMPLEMENTATION_PROGRESS.md** with optimization results

---

**Status**: Investigation phase  
**Next**: Add validation counters and test depth_6_nodes necessity

---

## Future Safety Mechanisms (Ideas - Not Implemented)

### Emergency Memory Release System

**Concept**: Implement automatic safety mechanisms to handle unexpected memory spikes (e.g., unpredicted rehash, hash collision storms, implementation bugs).

#### Trigger Conditions

1. **RSS threshold exceeded**: `get_rss_kb() > SAFETY_THRESHOLD`
2. **Unexpected rehash detected**: `bucket_count()` changed without prediction
3. **Memory allocation failure**: `std::bad_alloc` or allocation errors

#### Emergency Response Actions

**Action 1: Immediate Cache Cleanup**
```cpp
if (get_rss_kb() > (MEMORY_LIMIT_MB * 1024 * 0.9)) {  // 90% threshold
    malloc_trim(0);  // Force return of allocator cache to OS
    if (verbose) {
        std::cout << "⚠️ Emergency malloc_trim triggered" << std::endl;
        std::cout << "RSS after trim: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
    }
}
```

**Action 2: Graceful Degradation with goto**
```cpp
// In Phase 2-4 expansion loops
if (get_rss_kb() > EMERGENCY_RSS_KB) {
    if (verbose) {
        std::cout << "⚠️ EMERGENCY: RSS limit approached!" << std::endl;
        std::cout << "Current RSS: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
        std::cout << "Limit: " << (EMERGENCY_RSS_KB / 1024.0) << " MB" << std::endl;
        std::cout << "Stopping expansion early and cleaning up..." << std::endl;
    }
    goto emergency_cleanup;
}

// ... normal expansion continues ...

emergency_cleanup:
    // Immediate cleanup
    {
        tsl::robin_set<uint64_t> temp;
        current_expansion_set.swap(temp);
    }
    malloc_trim(0);
    
    // Graceful exit
    if (verbose) {
        std::cout << "Emergency cleanup complete." << std::endl;
        std::cout << "Database construction stopped at reduced depth." << std::endl;
        std::cout << "RSS after cleanup: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
    }
    return partial_database;  // Return what we have so far
```

**Action 3: Temporary Object Ejection**
```cpp
// If RSS critical during Phase 4, temporarily drop depth_6_nodes
if (get_rss_kb() > (MEMORY_LIMIT_MB * 1024 * 0.95)) {  // 95% critical
    if (verbose) {
        std::cout << "⚠️ CRITICAL: Ejecting depth_6_nodes temporarily" << std::endl;
    }
    
    // Save depth_6_nodes to disk or drop (accept higher duplicate rate)
    {
        tsl::robin_set<uint64_t> temp;
        depth_6_nodes.swap(temp);
    }
    malloc_trim(0);
    
    use_depth6_check = false;  // Continue without depth_6 validation
}
```

#### Safety Threshold Calculation

```cpp
// Conservative thresholds with headroom
const size_t SAFETY_THRESHOLD_KB = MEMORY_LIMIT_MB * 1024 * 0.85;  // 85% - Warning
const size_t EMERGENCY_RSS_KB = MEMORY_LIMIT_MB * 1024 * 0.92;     // 92% - Action
const size_t CRITICAL_RSS_KB = MEMORY_LIMIT_MB * 1024 * 0.97;      // 97% - Abort
```

### Predictive Peak Estimation

**Concept**: Predict peak RSS before each phase and abort early if unsafe.

```cpp
// Before Phase 4
size_t predicted_peak_phase4 = 
    get_rss_kb() +                        // Current RSS
    (bucket_9 / 1024) +                   // depth_9_nodes allocation
    (index_pairs[8].size() * 32 / 1024);  // depth8_set allocation

if (predicted_peak_phase4 > (MEMORY_LIMIT_MB * 1024 * 0.95)) {
    std::cout << "⚠️ WARNING: Predicted Phase 4 peak " 
              << (predicted_peak_phase4 / 1024.0) << " MB exceeds safety threshold" << std::endl;
    std::cout << "Aborting Phase 4 expansion" << std::endl;
    std::cout << "Database will contain depths 0-8 only" << std::endl;
    goto skip_phase4;
}

// ... Phase 4 proceeds ...

skip_phase4:
    // Gracefully finalize database with what we have
    finalize_database(/* depth_limit = */ 8);
```

### Rehash Storm Detection

**Concept**: Detect pathological hash collision patterns and abort before catastrophic memory growth.

```cpp
// Track rehash frequency
size_t rehash_count = 0;
size_t last_bucket_count = current_set.bucket_count();

// Inside expansion loop
if (current_set.bucket_count() != last_bucket_count) {
    rehash_count++;
    if (rehash_count > 3) {  // More than 3 rehashes = storm
        std::cout << "⚠️ REHASH STORM DETECTED!" << std::endl;
        std::cout << "Multiple unexpected rehashes suggest hash collision issues" << std::endl;
        std::cout << "Aborting expansion to prevent memory explosion" << std::endl;
        goto emergency_cleanup;
    }
    last_bucket_count = current_set.bucket_count();
}
```

### Advantages

1. **Fail-safe operation**: Even with implementation bugs or unexpected behavior, system won't crash
2. **Graceful degradation**: Returns partial database instead of failing completely
3. **Production robustness**: Handles edge cases (weird scrambles, hash collisions, etc.)
4. **Developer safety**: Protects against mistakes during development/testing

### Trade-offs

1. **Complexity**: Additional code paths to test and maintain
2. **Performance overhead**: RSS checks on every iteration (mitigated by checking every N iterations)
3. **Reduced database quality**: Early abort means incomplete database
4. **Goto usage**: Some developers consider `goto` controversial (but justified for emergency cleanup)

### Implementation Status

**Current**: ❌ Not implemented (ideas only)

**Reason**: Current system is stable with predictable memory behavior after optimizations:
- Peak RSS well-characterized (599 MB for 8M/8M/8M)
- No unexpected rehashes observed
- Allocator cache understood and controllable

**When to implement**: If deploying in constrained environment (e.g., embedded systems, strict memory limits) or if unexpected memory spikes are observed in production.

### Alternative: Monitoring and Alerting

Instead of automatic emergency mechanisms, implement monitoring:

```cpp
// Periodic RSS logging
if (iteration_count % 10000 == 0) {
    size_t current_rss = get_rss_kb();
    if (current_rss > last_logged_rss * 1.1) {  // 10% jump
        std::cout << "ℹ️ RSS increased: " << (last_logged_rss / 1024.0) 
                  << " MB → " << (current_rss / 1024.0) << " MB" << std::endl;
        last_logged_rss = current_rss;
    }
}
```

This allows developer intervention without automatic goto-based bailout.


---

## Spike Investigation Results (2026-01-02)

### Motivation

**User's Critical Insight**: The RSS measured inside C++ may fail to capture extremely fast memory spikes because the sampling interval is long (i.e., the value measured here is the peak memory assuming no spikes—an idealized peak). Therefore, it is necessary to observe using a /proc-based system with 10 ms sampling.

**Hypothesis**: C++ checkpoints only measure post-allocation steady-state values. Transient spikes during robin_set bucket expansion or rehashing might exceed checkpoint values briefly, then settle to measured levels.

**Goal**: Use 10ms /proc monitoring to detect if actual peak RSS > checkpoint peak RSS.

### Test Configuration

- **Model**: 8M/8M/8M (largest bucket model)
- **Memory limit**: 1600 MB
- **Monitoring**: 10ms /proc/PID/status sampling (16,971 samples over 180s)
- **Implementation**: Integrated monitoring (solver launched as subprocess, zero timing gap)
- **Reference**: Based on stable-20251230 backup scripts

### Results Summary

| Metric | 10ms Monitoring | C++ Checkpoint | Difference |
|--------|----------------|----------------|------------|
| **Peak VmRSS** | **656.69 MB** | 657 MB | -0.31 MB (-0.05%) |
| Peak VmSize | 669.13 MB | N/A | - |
| Peak VmData | 662.96 MB | N/A | - |
| Peak time | t=61.11s | Phase 3 end | - |

**Memory Spikes Detected**: 11 spikes (>20 MB within 1s)

**Top 5 largest spikes**:

| Time (s) | RSS (MB) | Increase (MB) | Duration (ms) | Rate (MB/s) | Phase |
|----------|----------|---------------|---------------|-------------|-------|
| 32.85 | 255.0 | +27.1 | 10 | 2712.5 | BFS depth 6 |
| 44.92 | 413.1 | +25.0 | 11 | 2272.7 | Phase 3 start |
| 61.20 | 405.1 | +23.9 | 11 | 2170.5 | Phase 4 |
| 32.84 | 227.9 | +23.6 | 11 | 2147.7 | BFS depth 6 |
| 44.88 | 330.6 | +23.6 | 11 | 2147.7 | Phase 3 |

### Phase 3 Detailed Analysis

**C++ Checkpoints**:
```
RSS at Phase 3 start: 285.4 MB
RSS after depth_8_nodes creation (empty): 413.4 MB
RSS after Phase 3 completion: ~657 MB
```

**10ms Monitoring** (t=44.88-45.18s, 0.3 seconds):
```
t=44.88s: RSS = 306.9 MB   (+23.6 MB spike)
t=44.91s: RSS = 388.1 MB   (+21.6 MB spike)
t=44.92s: RSS = 413.1 MB   (+25.0 MB spike) ← matches checkpoint!
t=44.93s: RSS = 425.1 MB
t=44.94s: RSS = 434.3 MB
...
t=44.98s: RSS = 478.8 MB   (VmSize jump: 420→666 MB)
...
t=45.18s: RSS = 598.9 MB   (temporary peak)
t=46.73s: RSS = 599.1 MB   (stabilizes)
...
t=61.11s: RSS = 656.7 MB   (final peak)
```

**Observation**: 
- Checkpoint value 413.4 MB confirmed at t=44.92s
- Rapid allocation burst from 413→599 MB in 0.26 seconds (growth rate: ~715 MB/s)
- Final peak 656.7 MB matches C++ checkpoint (657 MB)
- **No transient spike above 657 MB**

### Spike Characterization

The detected "spikes" (>20 MB within 1s) are **normal allocation behavior**, not transient excursions:

1. **Bucket allocation spikes** (t=44.92-44.98s):
   - robin_set allocates 8M buckets = 32 MB (8M × 4 bytes)
   - Observed: +25 MB spike → **consistent with bucket allocation**

2. **Rehashing spikes** (t=44.98-45.18s):
   - Inserting 7.5M nodes triggers multiple rehashes
   - Each rehash temporarily doubles memory (old + new buckets)
   - Observed: continuous growth 478→599 MB → **normal rehashing**

3. **Data insertion spikes** (t=45.18-61.11s):
   - Gradual growth from 599→657 MB
   - Represents node data insertion without further bucket expansion
   - Observed: +58 MB over 16 seconds → **normal insertion**

### Conclusions

**C++ Checkpoint Accuracy**: ✅ **VALIDATED**
- 10ms monitoring peak: 656.69 MB
- C++ checkpoint peak: 657 MB  
- **Difference: 0.05%** (within measurement noise)

**Hypothesis Result**: ❌ **FALSE**
- **No transient spikes detected** beyond checkpoint values
- All 11 spikes occur DURING continuous memory growth
- Peak reached at t=61.11s = checkpoint peak
- No temporary excursions above 657 MB
- Checkpoint measurements are **comprehensive**, not "ideal case"

**Production Safety**: ✅ **CONFIRMED**
- Validated peak RSS: **657 MB** for 8M/8M/8M model
- 1600 MB limit provides **943 MB margin (59%)**
- No unexpected memory behavior detected

**Monitoring Infrastructure**: ✅ **VALIDATED**
- 10ms /proc-based system successfully captures rapid allocations
- Tools ready for future investigations if needed
- See [MEMORY_MONITORING.md](../MEMORY_MONITORING.md#integrated-monitoring-system-2026-01-02)

**Detailed Investigation Report**: `backups/logs/memory_monitoring_20260102/10MS_SPIKE_INVESTIGATION.md`