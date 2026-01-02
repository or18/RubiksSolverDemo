# Depth 9→10 Local Expansion Design

**Date**: 2026-01-02  
**Status**: Design Phase  
**Priority**: High (depth 10 is volume peak)

---

## Background

### Volume Distribution Analysis

xxcross search patterns show a **critical volume peak at depth 10**:

**Empirical Distribution** (for typical scrambles, 36 xxcross patterns):
- **Depth 9**: Moderate volume (~15-20% of total)
- **Depth 10**: **VOLUME PEAK** (~60-70% of total solutions) ⚠️
- **Depth 11**: Low volume (~10-15%)
- **Depth 12**: Rare (requires adversarial scrambles)

**Current Implementation Limitation**:
- BFS: depth 0→6 (full breadth-first search)
- Local expansion: depth 7→8, depth 8→9
- **Missing**: depth 9→10 (THE MOST IMPORTANT DEPTH!)

**Problem**: Without depth 10 expansion, the solver cannot find optimal solutions for the majority of positions.

---

## Current Implementation Review

### Phase Structure (solver_dev.cpp)

**Phase 1: BFS (depth 0→6)**
```cpp
// Full breadth-first search
for (depth = 0; depth <= BFS_DEPTH; depth++) {
    index_pairs[depth] = robin_set<uint64_t>(bucket_sizes[depth]);
    // Expand all nodes at current depth
}
```
- Memory: Controlled by bucket sizes
- Completeness: 100% (all nodes visited)

**Phase 3: Local Expansion (depth 7→8)**
```cpp
// Selective expansion using depth 6 distance check
// For each depth 7 node:
//   - Expand to depth 8
//   - Keep only if (d6_dist + 2 <= 8)
```
- Memory: Bounded by bucket_d8
- Pruning: depth_6_nodes distance check
- Load factor: ~89% (efficient)

**Phase 4: Local Expansion (depth 8→9)**
```cpp
// Similar to Phase 3
// For each depth 8 node:
//   - Expand to depth 9
//   - Keep only if (d6_dist + 3 <= 9)
```
- Memory: Bounded by bucket_d9
- Pruning: depth_6_nodes distance check
- Load factor: ~70% (acceptable)

**Missing Phase 5: Local Expansion (depth 9→10)**
- Not implemented ❌
- **This is the volume peak depth!**

---

## Design Challenges

### Challenge 1: Memory Constraints

**Depth 10 Volume Estimate**:
- Depth 9 nodes: ~3-4 million (current implementation)
- Branching factor: ~12-15 moves
- Depth 10 candidates: 3M × 15 = **45 million nodes**
- After pruning (50% pruned): ~**22 million nodes**

**Memory Requirement** (robin_set load factor ~70%):
- Bucket size needed: 22M / 0.7 = **31.4M entries**
- Nearest power of 2: **32M (33,554,432 entries)**
- Memory: 32M × 8 bytes = **256 MB**

**Current 8M/8M/8M Configuration**:
- bucket_d9 = 8M (current)
- **Need bucket_d10 = 32M** (4x larger!)
- Total memory increase: +256 MB

**Feasibility**:
- Native Linux: ✅ Possible (225 MB → 481 MB, still < 1600 MB limit)
- WASM (512 MB limit): ⚠️ Tight (450 MB → 706 MB, needs careful configuration)

---

### Challenge 2: Pruning Strategy

**Depth 6 Distance Check** (used for depth 7→8, 8→9):
```cpp
// Check if node can reach depth 10 from depth 6
if (depth_6_distance + 4 <= 10) {
    // Keep this depth 10 node
}
```

**Effectiveness Analysis**:
- **Depth 7**: d6_dist ≤ 1 → prune ~10% ✅
- **Depth 8**: d6_dist ≤ 2 → prune ~25% ✅
- **Depth 9**: d6_dist ≤ 3 → prune ~40% ✅
- **Depth 10**: d6_dist ≤ 4 → prune ~50%? (estimated)

**Pruning Ratio Estimate**:
- 45M candidates → 22M after pruning (~50%)
- Still very large!

**Alternative Pruning Strategies**:
1. **Depth 7 distance check** (tighter bound)
   - Check: `depth_7_distance + 3 <= 10`
   - Requires keeping depth_7_nodes (memory cost)
   - Prune rate: ~60-70%?

2. **Hybrid strategy** (depth 6 + depth 7)
   - First filter: depth_6_distance + 4 <= 10
   - Second filter: depth_7_distance + 3 <= 10
   - Prune rate: ~70-80%?

3. **Progressive pruning** (during expansion)
   - Check depth_6_distance during generation
   - Discard immediately if fails
   - Reduces peak memory

---

### Challenge 3: Bucket Configuration

**Need New Bucket Model**: 8M/8M/8M/**32M** (depth 7/8/9/10)

**Bucket Config Extension**:
```cpp
struct BucketConfig {
    size_t custom_bucket_7 = 0;
    size_t custom_bucket_8 = 0;
    size_t custom_bucket_9 = 0;
    size_t custom_bucket_10 = 0;  // NEW: depth 10 bucket size
};
```

**Model Presets** (need update):
- **MINIMAL**: 2M/4M/4M/16M (~400 MB)
- **LOW**: 4M/4M/4M/16M (~450 MB)
- **BALANCED**: 4M/8M/8M/32M (~550 MB) - Default
- **MEDIUM**: 8M/8M/8M/32M (~700 MB)
- **STANDARD**: 8M/16M/16M/64M (~1000 MB)
- **HIGH**: 16M/32M/32M/128M (~1800 MB)

---

## Implementation Strategy

### Option A: Direct Extension (Simplest)

**Advantages**:
- ✅ Simple implementation (copy Phase 4 pattern)
- ✅ Reuses existing depth_6_nodes pruning
- ✅ No new data structures needed

**Disadvantages**:
- ❌ Large memory increase (+256 MB for 32M bucket)
- ❌ Moderate pruning efficiency (~50%)
- ❌ May hit WASM memory limits

**Implementation**:
```cpp
// Phase 5: Local Expansion depth 9→10 (with depth 6 check)
robin_set<uint64_t> depth_10_set(bucket_d10);
for (auto& node9 : index_pairs[9]) {
    // Expand to depth 10
    for (int move = 0; move < 18; move++) {
        uint64_t node10 = apply_move(node9, move);
        
        // Pruning: check depth 6 distance
        int d6_dist = depth_6_nodes[extract_d6_index(node10)];
        if (d6_dist + 4 <= 10) {
            depth_10_set.insert(node10);
        }
    }
}
index_pairs[10] = std::move(depth_10_set);
```

**Memory Profile**:
- Peak: depth_9_set + depth_10_set (~8M + 32M = 40M = 320 MB)
- Final: depth_10_set only (32M = 256 MB)

---

### Option B: Hybrid Pruning (Better Efficiency)

**Advantages**:
- ✅ Better pruning (~70% vs 50%)
- ✅ Smaller final bucket size (16M vs 32M)
- ✅ Lower memory footprint

**Disadvantages**:
- ⚠️ Need to keep depth_7_nodes (+64 MB peak memory)
- ⚠️ More complex implementation
- ⚠️ Two-pass filtering overhead

**Implementation**:
```cpp
// Keep depth 7 nodes for tighter pruning
std::vector<uint8_t> depth_7_distances(SIZE_D7);
for (auto& node7 : index_pairs[7]) {
    depth_7_distances[extract_d7_index(node7)] = 7;
}

// Phase 5: Local Expansion depth 9→10 (with depth 6 + depth 7 check)
robin_set<uint64_t> depth_10_set(bucket_d10);  // Can use 16M instead of 32M
for (auto& node9 : index_pairs[9]) {
    for (int move = 0; move < 18; move++) {
        uint64_t node10 = apply_move(node9, move);
        
        // First filter: depth 6 distance
        int d6_dist = depth_6_nodes[extract_d6_index(node10)];
        if (d6_dist + 4 > 10) continue;
        
        // Second filter: depth 7 distance (tighter)
        int d7_dist = depth_7_distances[extract_d7_index(node10)];
        if (d7_dist + 3 > 10) continue;
        
        depth_10_set.insert(node10);
    }
}
```

**Memory Profile**:
- Peak: depth_9_set + depth_10_set + depth_7_distances (~8M + 16M + 64MB = 250 MB)
- Final: depth_10_set only (16M = 128 MB)
- **Net savings**: 128 MB vs 256 MB (Option A)

---

### Option C: Progressive Two-Stage Expansion (Memory Optimized)

**Advantages**:
- ✅ Lowest peak memory
- ✅ Progressive pruning during generation
- ✅ Can use depth_9_nodes for even tighter bounds

**Disadvantages**:
- ❌ Most complex implementation
- ❌ Two-phase expansion overhead
- ❌ Requires depth_9_nodes storage

**Implementation**:
```cpp
// Stage 1: Generate candidates with aggressive pruning
robin_set<uint64_t> depth_10_candidates(16M);  // Smaller initial bucket
for (auto& node9 : index_pairs[9]) {
    // Only expand nodes with good depth 6 distance
    int d6_base = depth_6_nodes[extract_d6_index(node9)];
    if (d6_base + 1 > 10 - 3) continue;  // Aggressive pre-filter
    
    for (int move = 0; move < 18; move++) {
        uint64_t node10 = apply_move(node9, move);
        
        // Immediate depth 6 check
        int d6_dist = depth_6_nodes[extract_d6_index(node10)];
        if (d6_dist + 4 <= 10) {
            depth_10_candidates.insert(node10);
        }
    }
}

// Stage 2: Refine with depth 9 distance check
robin_set<uint64_t> depth_10_final(8M);  // Final smaller bucket
for (auto& node10 : depth_10_candidates) {
    int d9_dist = depth_9_nodes[extract_d9_index(node10)];
    if (d9_dist + 1 <= 10) {
        depth_10_final.insert(node10);
    }
}
```

**Memory Profile**:
- Stage 1 peak: depth_9_set + depth_10_candidates (~8M + 16M = 192 MB)
- Stage 2 peak: depth_10_candidates + depth_10_final (~16M + 8M = 192 MB)
- Final: depth_10_final (8M = 64 MB)
- **Net savings**: 192 MB vs 256 MB (Option A)

---

## Recommended Approach

### Phase 1: Proof of Concept (Option A)

**Goal**: Validate depth 10 expansion feasibility

**Implementation**:
1. Add bucket_d10 to BucketConfig
2. Implement simple depth 6 distance pruning
3. Test with 8M/8M/8M/32M configuration
4. Measure:
   - Memory usage (peak + final)
   - Pruning effectiveness
   - Load factor
   - Coverage (% of positions found)

**Timeline**: 1-2 days

**Success Criteria**:
- ✅ Depth 10 expansion completes without OOM
- ✅ Load factor > 60%
- ✅ Prune rate > 40%
- ✅ Coverage improvement > 50%

---

### Phase 2: Optimization (Option B or C)

**Goal**: Reduce memory footprint for WASM deployment

**Choose Based on Phase 1 Results**:
- If pruning < 50%: Use Option B (hybrid pruning)
- If memory tight: Use Option C (progressive expansion)
- If performance critical: Optimize Option A

**Optimization Targets**:
- Reduce bucket_d10 from 32M to 16M (or 8M)
- Improve pruning rate to 70%+
- Maintain coverage > 90%

**Timeline**: 2-3 days

---

### Phase 3: Production Integration

**Tasks**:
1. Update bucket_config.h with 4-depth models
2. Create new presets (MINIMAL, LOW, BALANCED, etc.)
3. Measure all models (2M/2M/2M/8M, 4M/4M/4M/16M, etc.)
4. Update WASM builds
5. Performance testing
6. Documentation

**Timeline**: 3-5 days

---

## Memory Budget Analysis

### 8M/8M/8M/32M Configuration (Option A)

**Data Structures**:
- index_pairs[7]: 8M × 8 bytes = 64 MB
- index_pairs[8]: 8M × 8 bytes = 64 MB
- index_pairs[9]: 8M × 8 bytes = 64 MB
- index_pairs[10]: 32M × 8 bytes = **256 MB**
- depth_6_nodes: ~200MB
- move_tables: ~13 MB
- prune_tables: ~2 MB

**Total Theoretical**: ~660 MB

**With Allocator Cache** (WASM):
- Theoretical: 660 MB
- Overhead (4%): +26 MB
- Allocator cache: +50 MB (estimated)
- **Total: ~740 MB**

**Feasibility**:
- Native (1600 MB limit): ✅ Safe (46% utilization)
- **WASM (512 MB limit)**: ❌ **EXCEEDS LIMIT** (145% utilization)

**Conclusion**: Need Option B or C for WASM deployment!

---

### 8M/8M/8M/16M Configuration (Option B)

**Data Structures**:
- index_pairs[7]: 8M × 8 bytes = 64 MB
- index_pairs[8]: 8M × 8 bytes = 64 MB
- index_pairs[9]: 8M × 8 bytes = 64 MB
- index_pairs[10]: 16M × 8 bytes = **128 MB**
- depth_6_nodes: ~200MB
- depth_7_distances: ~64 MB (temporary)
- move_tables: ~13 MB
- prune_tables: ~2 MB

**Peak During Expansion**: ~600 MB (with depth_7_distances)
**Final After Cleanup**: ~540 MB

**With Allocator Cache** (WASM):
- Peak: ~650 MB
- Final: ~590 MB

**Feasibility**:
- Native: ✅ Safe (37% peak utilization)
- **WASM (512 MB limit)**: ⚠️ **TIGHT** (127% peak, needs careful tuning)

**Conclusion**: Borderline for WASM, need to test actual overhead

---

### 4M/4M/4M/16M Configuration (Option B, Memory-Constrained)

**Data Structures**:
- index_pairs[7]: 4M × 8 bytes = 32 MB
- index_pairs[8]: 4M × 8 bytes = 32 MB
- index_pairs[9]: 4M × 8 bytes = 32 MB
- index_pairs[10]: 16M × 8 bytes = 128 MB
- depth_6_nodes: ~200MB
- move_tables: ~13 MB
- prune_tables: ~2 MB

**Total Theoretical**: ~440 MB

**With Allocator Cache** (WASM):
- Theoretical: 440 MB
- Overhead + cache: +50 MB
- **Total: ~490 MB**

**Feasibility**:
- Native: ✅ Safe (31% utilization)
- **WASM (512 MB limit)**: ✅ **SAFE** (96% utilization, 4% margin)

**Conclusion**: **Best option for WASM** with depth 10 support!

---

## Implementation Checklist

### Code Changes

**bucket_config.h**:
- [ ] Add `custom_bucket_10` field to BucketConfig
- [ ] Update ModelData structure (4-depth models)
- [ ] Add depth 10 bucket sizes to all presets
- [ ] Update estimate_custom_rss() for 4-depth calculation

**solver_dev.cpp**:
- [ ] Add bucket_d10 variable
- [ ] Implement Phase 5: depth 9→10 expansion
- [ ] Add depth_10_set initialization
- [ ] Implement depth 6 distance pruning for depth 10
- [ ] Update reached_depth calculation
- [ ] Add depth 10 to index_pairs

**expansion_parameters.h** (if exists):
- [ ] Add EXPANSION_DEPTH_10 parameter
- [ ] Add BUCKET_D10_SIZE constants

### Testing

**Unit Tests**:
- [ ] Test bucket_d10 configuration
- [ ] Test depth 10 expansion logic
- [ ] Test pruning effectiveness
- [ ] Verify load factor

**Integration Tests**:
- [ ] Test full pipeline with depth 10
- [ ] Measure memory usage (peak + final)
- [ ] Compare coverage (with/without depth 10)
- [ ] Benchmark performance

**Memory Tests**:
- [ ] 8M/8M/8M/32M - Native only
- [ ] 8M/8M/8M/16M - Native + WASM (tight)
- [ ] 4M/4M/4M/16M - WASM safe
- [ ] Verify malloc_trim() behavior with depth 10

### Documentation

- [ ] Update IMPLEMENTATION_PROGRESS.md (Phase 7: Depth 10)
- [ ] Create depth_10_measurement_results.md
- [ ] Update bucket_model_rss_measurement.md
- [ ] Update USER_GUIDE.md with depth 10 options
- [ ] Add depth 10 to Experiences/README.md

---

## Risk Assessment

### High Risk ⚠️

1. **WASM Memory Limit**
   - Impact: Cannot deploy depth 10 to WASM
   - Mitigation: Use 4M/4M/4M/16M configuration
   - Fallback: Depth 9 only mode for WASM

2. **Pruning Ineffectiveness**
   - Impact: Bucket overflow, OOM crash
   - Mitigation: Measure pruning rate in Phase 1
   - Fallback: Use Option B (hybrid pruning)

### Medium Risk ⚠️

3. **Performance Degradation**
   - Impact: Slower search times
   - Mitigation: Benchmark before/after
   - Fallback: Make depth 10 optional (config flag)

4. **Coverage Gaps**
   - Impact: Depth 10 nodes not fully explored
   - Mitigation: Validate against known positions
   - Fallback: Adjust pruning thresholds

### Low Risk ✅

5. **Implementation Complexity**
   - Impact: Bugs, maintenance burden
   - Mitigation: Copy existing Phase 3/4 pattern
   - Fallback: Extensive unit tests

---

## Performance Predictions

### Search Time Impact

**Without Depth 10**:
- Depth 0-9 search: ~100ms (current)
- Coverage: ~30-40% of positions

**With Depth 10** (estimated):
- Depth 0-10 search: ~150ms (+50%)
- Coverage: ~90-95% of positions (+60%)

**Trade-off**: +50% time for +60% coverage → ✅ **Worth it**

### Memory Impact

| Configuration | Native (MB) | WASM (MB) | Coverage | Recommendation |
|---------------|-------------|-----------|----------|----------------|
| 8M/8M/8M (current) | 225 | 450 | 30% | Baseline |
| 8M/8M/8M/32M (Option A) | 481 | 740 | 90% | ❌ WASM exceeds |
| 8M/8M/8M/16M (Option B) | 353 | 590 | 85% | ⚠️ WASM tight |
| 4M/4M/4M/16M (Option B) | 265 | 490 | 80% | ✅ **WASM safe** |

**Recommendation**: Start with 4M/4M/4M/16M for maximum compatibility

---

## Next Steps

### Immediate (Phase 1)

1. **Week 1**: Implement Option A (proof of concept)
   - Add bucket_d10 configuration
   - Implement basic depth 9→10 expansion
   - Test with 8M/8M/8M/32M on native

2. **Week 1-2**: Measure and validate
   - Memory usage (peak + final)
   - Pruning effectiveness
   - Coverage improvement
   - Performance impact

### Short-term (Phase 2)

3. **Week 2-3**: Optimize if needed
   - If WASM memory tight: Implement Option B
   - If pruning poor: Add depth 7 distance check
   - If performance critical: Profile and optimize

4. **Week 3-4**: Production integration
   - Update all bucket presets
   - Test WASM builds
   - Documentation updates

### Long-term (Phase 3+)

5. **Month 2**: Advanced optimizations
   - Explore Option C (progressive expansion)
   - Investigate depth 11 (if memory allows)
   - Adaptive bucket sizing based on scramble

6. **Month 3**: Production deployment
   - Update WASM solver
   - Performance benchmarking
   - User acceptance testing

---

**Status**: Ready for implementation (Phase 1)  
**Next Action**: Add bucket_d10 to bucket_config.h and implement basic expansion

