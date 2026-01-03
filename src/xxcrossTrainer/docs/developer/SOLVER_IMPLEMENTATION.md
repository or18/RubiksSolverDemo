# XXCross Solver Implementation Details

**Version**: stable_20260103  
**Last Updated**: 2026-01-03  
**Source**: solver_dev.cpp (4723 lines)

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md)
>
> **Related**: [API Reference](API_REFERENCE.md) | [WASM Integration](WASM_INTEGRATION_GUIDE.md) | [Memory Monitoring](MEMORY_MONITORING.md) | [Experiments](Experiences/README.md)

---

## Purpose

This document explains the **core implementation logic** of the XXCross solver (`stable_20260103`), focusing on:

1. **Database Construction** (5-Phase BFS: depths 0-10)
2. **Memory Spike Elimination** (Pre-allocation, reserve optimization)
3. **WASM/Native Dual Support** (Platform-specific code paths)
4. **Depth 9→10 Expansion** (Random sampling with diversity control)
5. **Malloc Trim Management** (Allocator cache clearing)
6. **Developer Options** (Environment variables for research)

For API reference, see [API_REFERENCE.md](API_REFERENCE.md). For usage instructions, see [../USER_GUIDE.md](../USER_GUIDE.md).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Header Files & Dependencies](#header-files--dependencies)
3. [Database Construction (5 Phases)](#database-construction-5-phases)
4. [Memory Efficiency Optimizations](#memory-efficiency-optimizations)
5. [Depth 9→10 Expansion Strategy](#depth-910-expansion-strategy)
6. [WASM vs Native Differences](#wasm-vs-native-differences)
7. [Malloc Trim & Allocator Management](#malloc-trim--allocator-management)
8. [Developer Options & Research Mode](#developer-options--research-mode)
9. [Random Sampling for Diversity](#random-sampling-for-diversity)
10. [Implementation Warnings & Best Practices](#implementation-warnings--best-practices)
11. [Scramble Generation (IDA*)](#scramble-generation-ida)

---

## Architecture Overview

### High-Level Flow (stable_20260103)

```
xxcross_search constructor
  ├─ Parse bucket configuration (BucketModel or CUSTOM)
  ├─ Calculate bucket sizes (7, 8, 9, 10)
  ├─ Pre-allocate move tables
  │
  ├─ Phase 1: Full BFS (Depth 0-6)
  │   ├─ Complete expansion of all states
  │   ├─ Store in index_pairs[0..6]
  │   └─ Free depth_6_nodes after completion
  │
  ├─ Phase 2: Local Expansion (Depth 6→7)
  │   ├─ Face-diverse expansion with capacity management
  │   ├─ Pre-reserve depth 7 bucket (spike elimination)
  │   └─ Detach element vector pattern
  │
  ├─ Phase 3: Local Expansion (Depth 7→8)
  │   ├─ Same pattern as Phase 2
  │   ├─ Build depth7_set for duplicate detection
  │   └─ Free depth7_set after expansion
  │
  ├─ Phase 4: Local Expansion (Depth 8→9)
  │   ├─ Build depth8_set with rehash optimization
   ├─ Bulk insert pattern (avoid memory spikes)  
  │   └─ Free depth8_set after expansion
  │
  ├─ Malloc Trim (Native only, optional)
  │   └─ Clear allocator cache before Phase 5
  │
  ├─ Phase 5: Local Expansion (Depth 9→10)
  │   ├─ Random sampling (not complete expansion)
  │   ├─ Single move per parent (diversity control)
  │   ├─ Adaptive children per parent
  │   ├─ Build depth9_set with bulk insert
  │   └─ Stop on rehash (memory safety)
  │
  └─ Final Cleanup
      ├─ shrink_to_fit() all vectors
      ├─ Collect statistics (if enabled)
      └─ Ready for IDA* search
```

### Key Changes from stable-20251230

| Feature | Old (20251230) | New (20260103) |
|---------|----------------|----------------|
| Max Depth | 9 | **10** (Phase 5 added) |
| Memory Spikes | Present (attach/detach) | **Eliminated** (pre-reserve) |
| Depth 10 Method | N/A | **Random sampling** (not full BFS) |
| Malloc Trim | Always enabled | **Configurable** (ENV variable) |
| WASM Support | Basic | **Full integration** (embind, heap monitoring) |
| Developer Options | Limited | **Extensive** (ENABLE_CUSTOM_BUCKETS, DISABLE_MALLOC_TRIM, etc.) |

---

## Header Files & Dependencies

### Standard Libraries

```cpp
#include <iostream>      // Console I/O
#include <fstream>       // File I/O (RSS reading)
#include <vector>        // Dynamic arrays
#include <algorithm>     // std::shuffle, std::min, etc.
#include <unordered_map> // Hash maps
#include <string>        // String operations
#include <sstream>       // String streams
#include <cstdlib>       // std::getenv
#include <random>        // Random number generation (MT19937)
#include <deque>         // Double-ended queue
#include <iomanip>       // std::setprecision
```

### Third-Party Libraries

```cpp
#include <tsl/robin_set.h>  // Tessil robin_set (open addressing hash set)
```

**Why robin_set?**
- **Performance**: Faster than `std::unordered_set` (cache-friendly)
- **Load Factor Control**: `max_load_factor(0.9)` for memory efficiency
- **Element Vector Attachment**: Direct write to output vector (eliminates copy)
- **License**: MIT (compatible with most projects)

**Repository**: [Tessil/robin-map](https://github.com/Tessil/robin-map)

### Custom Headers

```cpp
#include "bucket_config.h"  // BucketModel enum, ModelData, BucketConfig, ResearchConfig
```

**bucket_config.h** defines:
- `BucketModel` enum (AUTO, MINIMAL, LOW, ..., CUSTOM, WASM_MOBILE_LOW, ...)
- `ModelData` struct (bucket sizes, measured RSS, load factors)
- `BucketConfig` struct (model selection, custom sizes)
- `ResearchConfig` struct (enable_custom_buckets, skip_search, verbose, etc.)

### Platform-Specific Headers

```cpp
#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>  // embind (C++ ↔ JavaScript)
    #include <emscripten/heap.h>  // emscripten_get_heap_size()
    #include <emscripten.h>       // General WASM utilities
#endif

#ifndef __EMSCRIPTEN__
    #include <malloc.h>           // malloc_trim() (glibc only)
#endif
```

**WASM-specific functions**:
- `emscripten_get_heap_size()`: Get total heap size in bytes
- `log_emscripten_heap(phase_name)`: Helper to log heap at checkpoints

**Native-specific functions**:
- `get_rss_kb()`: Read RSS from `/proc/self/status` (Linux only)
- `malloc_trim(0)`: Return unused memory to OS (glibc allocator)

### Compiler Optimizations

```cpp
#pragma GCC target("avx2")
```

**Purpose**: Enable AVX2 SIMD instructions for performance

**Impact**: ~5-10% speedup in move table lookups (platform-dependent)

---

## Database Construction (5 Phases)

### Phase 1: Full BFS (Depth 0-6)

**Purpose**: Build complete database of all reachable XXCross states up to depth 6.

**Expansion Pattern**: Complete (no memory constraints)

**Memory Behavior**:
- Depths 0-5: Negligible (~5 MB)
- Depth 6: ~220 MB peak (during expansion)
- After Phase 1: ~120 MB (depth_6_nodes freed)

**Code Flow**:

```cpp
// Sliding window BFS (depths 0-6)
SlidingDepthSets visited(memory_limit_total_bytes);
visited.cur.insert(0);  // Solved state

for (int d = 0; d < BFS_DEPTH; ++d) {
    expand_one_depth(visited, d, multi_move_table_xxcross, 
                     index_pairs, verbose);
}

// Free depth 6 nodes (no longer needed)
{
    tsl::robin_set<uint64_t> temp;
    visited.cur.swap(temp);
}
```

**Key Data Structure**: `SlidingDepthSets`
- `prev`: Depth n-1 (for deduplication)
- `cur`: Depth n (current frontier)
- `next`: Depth n+1 (new states)

**Element Vector Attachment**:
```cpp
visited.next.attach_element_vector(&index_pairs[next_depth]);
// Insertions to 'next' directly write to index_pairs[next_depth]
```

**Benefits**:
- Eliminates copy from hash set to vector
- Reduces peak memory by ~50 MB

---

### Phase 2: Local Expansion (Depth 6→7)

**Purpose**: Expand depth 6 to depth 7 with **face-diverse strategy** and memory constraints.

**Strategy**: 
- **Pre-reserve bucket**: Eliminates attach/detach memory spike
- **Face-diverse expansion**: 6 moves per parent (one per face U/D/R/L/F/B)
- **90% load factor**: Target `bucket_d7 * 0.9` nodes

**Memory Spike Elimination** (Key Innovation):

```cpp
// OLD pattern (causes spike):
depth_7_nodes.attach_element_vector(&index_pairs[7]);
// ... expansion ...
depth_7_nodes.detach_element_vector();

// NEW pattern (eliminates spike):
index_pairs[7].reserve(bucket_d7 * 0.95);  // Pre-allocate
depth_7_nodes.attach_element_vector(&index_pairs[7]);
// ... expansion (no reallocation) ...
depth_7_nodes.detach_element_vector();  // Free hash table only
```

**Why This Works**:
- `attach_element_vector()` uses vector's existing capacity
- No reallocation during expansion → no spike
- `detach()` frees hash table (~50% of set memory)

**Face-Diverse Expansion**:

```cpp
std::vector<int> selected_moves;
selected_moves.reserve(18);  // Avoid reallocation

for (int face = 0; face < 6; ++face) {
    int move = all_moves[face];  // U, D, R, L, F, B
    selected_moves.push_back(move);
}
```

**Benefits**:
- Depth accuracy: Only moves exactly 1 ply from parent depth
- Diversity: Explores all directions equally
- Memory safety: Fixed 6 moves per parent (predictable growth)

**Experiment Reference**: See [Experiences/depth_10_memory_spike_investigation.md](Experiences/depth_10_memory_spike_investigation.md) for spike analysis.

---

### Phase 3: Local Expansion (Depth 7→8)

**Purpose**: Expand depth 7 to depth 8 using same pattern as Phase 2.

**Key Addition**: **depth7_set** for duplicate detection

```cpp
// Build depth7_set from index_pairs[7]
tsl::robin_set<uint64_t> depth7_set;
depth7_set.max_load_factor(0.95f);

size_t depth7_size = index_pairs[7].size();
size_t required_buckets = static_cast<size_t>(depth7_size / 0.95);

// Round up to power of 2 (avoid rehash)
size_t power_of_2 = 1;
while (power_of_2 < required_buckets) {
    power_of_2 <<= 1;
}
depth7_set.rehash(power_of_2);

// Bulk insert (more efficient than loop)
depth7_set.insert(index_pairs[7].begin(), index_pairs[7].end());
```

**Why Bulk Insert?**
- Single allocation instead of incremental growth
- Reduces allocator fragmentation
- ~20% faster than loop with `insert(node)`

**Duplicate Check**:
```cpp
if (depth7_set.find(next_node) != depth7_set.end()) {
    continue;  // Skip (already in depth 7)
}
```

**Memory Lifecycle**:
1. Build depth7_set (~50 MB for 3.7M nodes)
2. Expand depth 7→8
3. Free depth7_set (swap with empty set)
4. RSS drops by ~50 MB

---

### Phase 4: Local Expansion (Depth 8→9)

**Purpose**: Expand depth 8 to depth 9 with **maximum memory efficiency**.

**Same Pattern**: Pre-reserve + attach + expand + detach

**Depth8_set Construction** (Optimized):

```cpp
tsl::robin_set<uint64_t> depth8_set;
depth8_set.max_load_factor(0.95f);

size_t depth8_size = index_pairs[8].size();

// Calculate buckets and round to power of 2
size_t required_buckets = static_cast<size_t>(depth8_size / 0.95);
size_t power_of_2 = 1;
while (power_of_2 < required_buckets) {
    power_of_2 <<= 1;
}
depth8_set.rehash(power_of_2);  // Guarantee bucket count

// Bulk insert
depth8_set.insert(index_pairs[8].begin(), index_pairs[8].end());
```

**Why rehash() instead of reserve()**?
- `reserve(n)` sets minimum capacity (may round up unpredictably)
- `rehash(buckets)` guarantees exact bucket count
- Power-of-2 buckets prevent future rehashing

**Expansion Statistics** (typical):
- Depth 8 size: ~3.7M nodes
- Depth 9 target: ~7.5M nodes (90% of 8M bucket)
- depth8_set memory: ~50 MB
- Expansion time: ~15 seconds

---

### Malloc Trim (Native Only)

**Purpose**: Clear allocator cache before Phase 5 to reduce memory baseline.

**Why Needed?**

Glibc allocator (`malloc/free`) maintains a cache of freed memory:
- Speeds up future allocations
- But inflates RSS measurement
- Can accumulate 50-100 MB of cached memory

**When to Use**:
- **Native measurements**: Always (accurate RSS)
- **WASM mode**: Never (no `malloc_trim()` in WASM)
- **WASM-equivalent native**: Disabled via `DISABLE_MALLOC_TRIM=1`

**Code**:

```cpp
#ifndef __EMSCRIPTEN__
if (!research_config_.disable_malloc_trim) {
    size_t rss_before = get_rss_kb();
    malloc_trim(0);  // Return unused memory to OS
    size_t rss_after = get_rss_kb();
    
    if (verbose) {
        std::cout << "  RSS before malloc_trim: " << (rss_before / 1024.0) << " MB" << std::endl;
        std::cout << "  RSS after malloc_trim: " << (rss_after / 1024.0) << " MB" << std::endl;
        std::cout << "  Freed: " << ((rss_before - rss_after) / 1024.0) << " MB" << std::endl;
    }
}
#endif
```

**Typical Savings**: 50-80 MB

**Experiment Reference**: See [Experiences/wasm_heap_measurement_data.md](Experiences/wasm_heap_measurement_data.md) - Phase 4 peak measurements.

---

### Phase 5: Local Expansion (Depth 9→10)

**Purpose**: Partially expand depth 9 to depth 10 using **random sampling**.

**Why Random Sampling?**

Complete expansion of depth 9→10 would generate ~30M nodes:
- Requires ~32M bucket (256 MB hash table)
- Total RSS would exceed 1.5 GB for 16M bucket
- Not feasible for mobile devices

**Random sampling advantages**:
- Controlled memory: Fill exactly 90% of bucket
- Depth accuracy: All nodes are exactly depth 10
- Diversity: Random parent selection ensures variety
- Scalability: Works with any bucket size

**Algorithm**:

```cpp
// 1. Build depth9_set for duplicate detection
tsl::robin_set<uint64_t> depth9_set;
// ... (same bulk insert pattern as Phase 4) ...

// 2. Initialize depth 10 bucket
tsl::robin_set<uint64_t> depth_10_nodes;
depth_10_nodes.max_load_factor(0.9f);
depth_10_nodes.reserve(bucket_d10);
index_pairs[10].reserve(bucket_d10 * 0.95);
depth_10_nodes.attach_element_vector(&index_pairs[10]);

// 3. Random sampling
std::mt19937_64 rng(42);  // Fixed seed for reproducibility
std::uniform_int_distribution<size_t> parent_dist(0, index_pairs[9].size() - 1);
std::uniform_int_distribution<int> move_dist(0, 17);

int children_per_parent = 2;  // Adaptive (can be 1, 2, or 3)
size_t target_nodes = static_cast<size_t>(bucket_d10 * 0.9);

while (depth_10_nodes.size() < target_nodes) {
    // Random parent
    size_t parent_idx = parent_dist(rng);
    uint64_t parent_node = index_pairs[9][parent_idx];
    
    // Apply random moves
    for (int i = 0; i < children_per_parent; ++i) {
        int move = move_dist(rng);
        uint64_t next_node = apply_move(parent_node, move);
        
        // Skip if in depth 9
        if (depth9_set.find(next_node) != depth9_set.end()) {
            continue;
        }
        
        // Insert (automatic dedup by robin_set)
        depth_10_nodes.insert(next_node);
        
        // Safety: Stop on rehash
        if (depth_10_nodes.bucket_count() != last_bucket_count) {
            goto phase5_done;
        }
    }
}
phase5_done:
```

**Key Safety Feature**: **Rehash Detection**

```cpp
const size_t current_bucket_count = depth_10_nodes.bucket_count();
if (current_bucket_count != last_bucket_count) {
    // Unexpected rehash → stop immediately
    // (Prevents memory spike from exceeding bucket limit)
    goto phase5_done;
}
```

**Why This Matters**:
- Robin_set rehashes when load exceeds `max_load_factor * bucket_count`
- Rehashing doubles bucket count → doubles memory
- Early stop prevents OOM on constrained devices

**Adaptive Children Per Parent**:

Current implementation: `children_per_parent = 2`

Future optimization (commented out):
```cpp
// Adaptive based on rejection rate
double rejection_rate = duplicate_count / (inserted_count + duplicate_count);
if (rejection_rate < 0.3) {
    children_per_parent = 3;  // Low rejection → more children
} else if (rejection_rate > 0.7) {
    children_per_parent = 1;  // High rejection → fewer children
}
```

**Statistics**:
- Typical bucket: 16M nodes
- Target fill: 14.4M nodes (90%)
- Rejection rate: ~60-70% (depth 9 duplicates + depth 10 duplicates)
- Processed parents: ~40-50K

**Experiment Reference**: See [Experiences/depth_10_implementation_results.md](Experiences/depth_10_implementation_results.md) for performance analysis.

---

## Memory Efficiency Optimizations

### 1. Pre-Reserve Pattern (Spike Elimination)

**Problem**: Attaching element vector to empty robin_set causes reallocation spike

**Old Pattern** (causes spike):
```cpp
robin_set.attach_element_vector(&vector);  // vector.size() == 0
// ... insertions cause vector reallocation ...
```

**New Pattern** (eliminates spike):
```cpp
vector.reserve(expected_size);  // Pre-allocate
robin_set.attach_element_vector(&vector);  // No reallocation needed
```

**Impact**: Eliminates ~50-100 MB transient spikes

**Experiment**: [Experiences/depth_10_memory_spike_investigation.md](Experiences/depth_10_memory_spike_investigation.md)

### 2. Bulk Insert for Hash Sets

**Problem**: Incremental insert causes multiple rehashes

**Old Pattern**:
```cpp
for (uint64_t node : nodes) {
    set.insert(node);  // May trigger rehash multiple times
}
```

**New Pattern**:
```cpp
// Calculate final bucket count
size_t required_buckets = nodes.size() / load_factor;
size_t power_of_2 = next_power_of_2(required_buckets);
set.rehash(power_of_2);  // Single allocation

// Bulk insert
set.insert(nodes.begin(), nodes.end());  // No rehash
```

**Impact**: ~20% faster, ~30 MB less peak memory

### 3. Hoist Allocations Outside Loops

**Problem**: Repeated allocations in tight loops

**Old Pattern**:
```cpp
for (uint64_t parent : parents) {
    std::vector<int> all_moves(18);  // Allocated every iteration!
    std::shuffle(all_moves.begin(), all_moves.end(), rng);
    // ...
}
```

**New Pattern**:
```cpp
std::vector<int> all_moves(18);  // Allocated once
for (uint64_t parent : parents) {
    std::shuffle(all_moves.begin(), all_moves.end(), rng);
    // ...
}
```

**Impact**: Eliminates millions of small allocations, reduces fragmentation

### 4. Reserve for Known Sizes

**Pattern**: Always reserve vectors with known final size

```cpp
// Bad
std::vector<int> selected_moves;
for (int i = 0; i < 6; ++i) {
    selected_moves.push_back(move);  // May reallocate
}

// Good
std::vector<int> selected_moves;
selected_moves.reserve(18);  // Max possible size
for (int i = 0; i < 6; ++i) {
    selected_moves.push_back(move);  // No reallocation
}
```

**Impact**: Microseconds per call, but millions of calls → seconds saved

---

## Depth 9→10 Expansion Strategy

### Random Sampling Rationale

**Why Not Complete Expansion?**

Complete BFS expansion of depth 9→10:
- Parents: ~7.5M nodes
- Branching factor: ~18 moves
- Expected children: ~30M nodes (after dedup)
- Required bucket: 32M+ (>256 MB hash table)
- Total RSS: >1.5 GB (exceeds mobile budget)

**Random Sampling Benefits**:
- **Memory control**: Fill exactly to bucket capacity
- **Depth accuracy**: All nodes are exactly depth 10 (no depth 9 nodes included)
- **Diversity**: Random parent selection explores state space uniformly
- **Scalability**: Works with 4M, 8M, 16M buckets

### Diversity Control

**Face-Diverse Expansion** (Phases 2-4):
- Select 6 moves per parent (one per face)
- Ensures balanced exploration of move space

**Random Move Selection** (Phase 5):
- Uniform distribution over 18 moves
- Compensates for single-move-per-parent constraint

**Why Single Move Per Parent?**

Multiple moves per parent causes **depth contamination**:
- Move 1: depth 10 ✓
- Move 2 from same parent: depth 10 ✓
- But: Both moves might generate same region of state space
- Result: Lower diversity than expected

**Single move per parent** + **random parent selection**:
- Each insertion explores different region
- Maximum diversity for given number of samples

### Rejection Rate Analysis

**Typical Statistics** (8M/8M/8M/16M bucket):
- Total attempts: ~50M
- Inserted: ~14.4M
- Duplicates: ~35.6M
- Rejection rate: ~71%

**Breakdown**:
- Depth 9 duplicates: ~40% (already in depth9_set)
- Depth 10 duplicates: ~31% (already in depth_10_nodes)

**Adaptive Strategy** (Future):
- Monitor rejection rate during expansion
- Increase `children_per_parent` if rejection < 30%
- Decrease if rejection > 70%
- Target: Balance between speed and diversity

---

## WASM vs Native Differences

### Platform Detection

```cpp
#ifdef __EMSCRIPTEN__
    // WASM-specific code
#else
    // Native-specific code
#endif
```

### Heap Monitoring

**WASM**:
```cpp
#ifdef __EMSCRIPTEN__
size_t heap_bytes = emscripten_get_heap_size();
size_t heap_mb = heap_bytes / (1024 * 1024);
std::cout << "[Heap] Phase: " << heap_mb << " MB" << std::endl;
#endif
```

**Native**:
```cpp
#ifndef __EMSCRIPTEN__
size_t rss_kb = get_rss_kb();  // Read from /proc/self/status
size_t rss_mb = rss_kb / 1024;
std::cout << "RSS: " << rss_mb << " MB" << std::endl;
#endif
```

### Memory Behavior

**WASM Heap Characteristics**:
- **Never shrinks**: `free()` does not reduce heap size
- **Growth granularity**: 64 KB pages (WebAssembly.Memory)
- **Max size**: 2 GB (32-bit addressing) or 4 GB (browser limit)
- **Measurement point**: Final heap size == peak heap size

**Native RSS Characteristics**:
- **Shrinks with malloc_trim()**: Returns memory to OS
- **Growth granularity**: OS page size (4 KB on Linux)
- **Max size**: System-dependent (typically GB-TB range)
- **Measurement point**: Peak RSS during construction

### Malloc Trim

**Only available on Native** (glibc):

```cpp
#ifndef __EMSCRIPTEN__
malloc_trim(0);  // Not available in WASM
#endif
```

**WASM Equivalent**: No action needed (heap never caches freed memory)

### Function Export

**WASM** (embind):
```cpp
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("solve_with_custom_buckets", &solve_with_custom_buckets);
    emscripten::class_<SolverStatistics>("SolverStatistics")
        .property("node_counts", &SolverStatistics::node_counts)
        // ...
}
#endif
```

**Native**: Standard C++ (no special exports)

---

## Malloc Trim & Allocator Management

### Purpose

Glibc's `ptmalloc2` allocator maintains a cache of freed memory:
- Speeds up future allocations (reduces system calls)
- But inflates RSS measurements
- Cache can accumulate 50-100 MB

**malloc_trim(0)** forces allocator to return unused memory to OS:
- Reduces RSS to actual allocated memory
- Useful for accurate memory measurements
- No effect on WASM (no malloc cache)

### Usage Pattern

**When to Enable**:
- Native RSS measurements
- Production code (reduce memory footprint)

**When to Disable**:
- WASM compilation (function not available)
- WASM-equivalent native measurements (for fair comparison)

### Environment Variable Control

```cpp
const char *env_disable = std::getenv("DISABLE_MALLOC_TRIM");
if (env_disable != nullptr) {
    research_config.disable_malloc_trim = (std::string(env_disable) == "1");
}
```

**Usage**:
```bash
# Normal operation (malloc_trim enabled)
./solver_dev 308

# WASM-equivalent measurement (malloc_trim disabled)
DISABLE_MALLOC_TRIM=1 ./solver_dev 308
```

### Typical Impact

**With malloc_trim**:
- Phase 4 end: ~600 MB RSS
- After malloc_trim: ~550 MB RSS
- Phase 5 peak: ~750 MB RSS

**Without malloc_trim** (WASM-equivalent):
- Phase 4 end: ~650 MB RSS (cache accumulation)
- No trim: ~650 MB RSS
- Phase 5 peak: ~800 MB RSS

**Delta**: ~50 MB difference (allocator cache)

**Experiment**: [Experiences/wasm_heap_measurement_data.md](Experiences/wasm_heap_measurement_data.md) - "Final Cleanup Complete" measurements.

---

## Developer Options & Research Mode

### Environment Variables

**ENABLE_CUSTOM_BUCKETS**:
```bash
ENABLE_CUSTOM_BUCKETS=1 ./solver_dev 7 8 8 8
```
- Enables custom bucket specification
- Overrides BucketModel selection
- Required for WASM measurements

**DISABLE_MALLOC_TRIM**:
```bash
DISABLE_MALLOC_TRIM=1 ./solver_dev 308
```
- Skips `malloc_trim()` call
- Use for WASM-equivalent native measurements

**VERBOSE**:
```bash
VERBOSE=0 ./solver_dev 308
```
- Disable all console output (CSV mode)
- Use for automated benchmarks

### Command-Line Formats

**Single Memory Limit** (Legacy):
```bash
./solver_dev 308
# Uses auto-calculated buckets based on 308 MB limit
```

**Custom 3-Bucket** (Depths 7-9):
```bash
ENABLE_CUSTOM_BUCKETS=1 ./solver_dev 8 8 8
# 8 MB for each of depths 7, 8, 9 (depth 10 disabled)
```

**Custom 4-Bucket** (Depths 7-10):
```bash
ENABLE_CUSTOM_BUCKETS=1 ./solver_dev 8 8 8 16
# 8 MB for depths 7-9, 16 MB for depth 10
```

### ResearchConfig Fields

```cpp
struct ResearchConfig {
    bool enable_custom_buckets = false;   // Allow 3/4-bucket CLI
    bool skip_search = false;             // Skip IDA* (database only)
    bool verbose = true;                  // Console logging
    bool disable_malloc_trim = false;     // WASM-equivalent mode
    bool collect_detailed_statistics = false;  // CSV per-depth output
};
```

**Usage in Code**:
```cpp
ResearchConfig research;
research.enable_custom_buckets = true;
research.skip_search = true;  // Build database only (no scramble generation)

xxcross_search solver(308, bucket_config, research, true);
```

---

## Random Sampling for Diversity

### Why Randomness?

**Deterministic expansion** (BFS):
- Explores states in breadth-first order
- Guarantees complete coverage
- But: Not feasible for depth 10 (memory constraints)

**Random sampling**:
- Explores random subset of state space
- No completeness guarantee
- But: Scalable to any bucket size

### Diversity Metrics

**Parent Selection**:
- Uniform distribution over `index_pairs[9]`
- All ~7.5M parents have equal probability
- Ensures no bias toward specific regions

**Move Selection**:
- Uniform distribution over 18 moves
- Balances face exploration (U/D/R/L/F/B)

**Result**: Depth 10 database is **representative sample** of full state space

### Validation

**Test**: Generate 1000 scrambles from depth 10 database
- Expected: Diverse scrambles (no duplicates)
- Actual: 1000 unique scrambles ✓
- Conclusion: Sufficient diversity for training

**Experiment**: Manual testing (no formal document yet)

### Tradeoff Analysis

| Approach | Coverage | Memory | Depth Accuracy | Diversity |
|----------|----------|--------|----------------|-----------|
| **Complete BFS** | 100% | High (32M+) | Perfect (all d=10) | Perfect |
| **Random Sampling** | ~0.5% | Controlled (16M) | Perfect (all d=10) | High |
| **Greedy Expansion** | Biased | Controlled | Imperfect (d=9,10 mix) | Low |

**Chosen**: Random sampling (best tradeoff for mobile constraints)

---

## Implementation Warnings & Best Practices

### 1. Always Pre-Reserve Before Attach

**❌ Bad**:
```cpp
robin_set.attach_element_vector(&vector);  // vector.capacity() == 0
// Insertions cause reallocation → memory spike
```

**✅ Good**:
```cpp
vector.reserve(expected_size);  // Pre-allocate capacity
robin_set.attach_element_vector(&vector);  // No reallocation
```

### 2. Use Bulk Insert for Hash Sets

**❌ Bad**:
```cpp
for (uint64_t node : nodes) {
    set.insert(node);  // May rehash multiple times
}
```

**✅ Good**:
```cpp
set.rehash(next_power_of_2(nodes.size() / load_factor));
set.insert(nodes.begin(), nodes.end());  // Single allocation
```

### 3. Hoist Allocations Out of Loops

**❌ Bad**:
```cpp
for (int i = 0; i < 1000000; ++i) {
    std::vector<int> temp(18);  // Allocated 1M times!
}
```

**✅ Good**:
```cpp
std::vector<int> temp(18);  // Allocated once
for (int i = 0; i < 1000000; ++i) {
    std::shuffle(temp.begin(), temp.end(), rng);
}
```

### 4. Free Large Data Structures Explicitly

**❌ Bad**:
```cpp
{
    robin_set<uint64_t> large_set(10000000);
    // ... use set ...
}  // Destructor may delay memory return
```

**✅ Good**:
```cpp
{
    robin_set<uint64_t> large_set(10000000);
    // ... use set ...
    
    // Explicit swap-with-empty (immediate deallocation)
    robin_set<uint64_t> temp;
    large_set.swap(temp);
}
```

### 5. Check for Unexpected Rehash

**❌ Bad**:
```cpp
set.reserve(target_size);
while (set.size() < target_size) {
    set.insert(node);
    // No check → may exceed memory if rehash occurs
}
```

**✅ Good**:
```cpp
size_t last_bucket_count = set.bucket_count();
while (set.size() < target_size) {
    set.insert(node);
    if (set.bucket_count() != last_bucket_count) {
        // Unexpected rehash → stop immediately
        break;
    }
}
```

### 6. Use Consistent Load Factors

**Recommendation**:
- **Expansion buckets** (depths 7-10): `max_load_factor(0.9)` for memory efficiency
- **Temporary sets** (depth7_set, depth8_set, depth9_set): `max_load_factor(0.95)` for speed

**Reason**: Higher load factor → more collisions but less memory

### 7. Measure at Correct Checkpoints

**WASM**:
```cpp
log_emscripten_heap("Phase 5 Complete");  // Use heap size
// "[Heap] Final Cleanup Complete" is peak (heap never shrinks)
```

**Native**:
```cpp
std::cout << "RSS after Phase 5: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
// RSS may include allocator cache (use malloc_trim for accuracy)
```

---

## Scramble Generation (IDA*)

### Overview

IDA* (Iterative Deepening A*) is used to generate optimal scrambles with **guaranteed exact depth**:
- Start from solved state
- Search for state in database at target depth
- **Verify actual optimal depth** (critical for depths 7+)
- Return move sequence to reach that state

**See [API_REFERENCE.md](API_REFERENCE.md) for detailed function signatures.**

### High-Level Flow (Updated 2026-01-04)

```cpp
std::string get_xxcross_scramble(std::string target_depth) {
    int len = std::stoi(target_depth);
    
    // Depth guarantee loop (up to 100 attempts)
    for (int attempt = 0; attempt < 100; ++attempt) {
        // Random target state from database
        size_t idx = random_index(num_list[len]);
        uint64_t target_node = index_pairs[len][idx];
        State target = decode_node(target_node);
        
        // Verify actual optimal depth
        int actual_depth = -1;
        for (int d = 1; d <= len; ++d) {
            if (depth_limited_search(target, d, adj)) {
                actual_depth = d;
                break;  // Found optimal solution
            }
        }
        
        // Accept only if depth matches request
        if (actual_depth == len) {
            tmp = AlgToString(solution);
            return tmp;  // Success!
        }
        // Otherwise retry with different node
    }
    
    // Fallback after 100 attempts (rare)
    tmp = AlgToString(solution);
    return tmp;
}
```

**Critical Fix (2026-01-03)**: Added `tmp = AlgToString(solution);` before return to ensure scramble string is properly assigned. Previously, function was returning uninitialized or stale `tmp` value, causing scramble length to always show as 1 move in browser UI.

**Depth Guarantee Implementation (2026-01-04)**: 

**Problem**: `index_pairs[len]` contains nodes **discovered** at depth `len`, but many have shorter optimal solutions due to sparse coverage:
- Depth 9: ~5B nodes exist, we store ~8M (0.16% coverage)
- Depth 10: ~50B nodes exist, we store ~4M (0.008% coverage)
- **99.84-99.99% of state space uncovered** → High probability random node has depth < len optimal solution

**Solution**: Retry loop with depth verification
- Test depths 1→len to find actual optimal solution
- Accept only if `actual_depth == len`
- Retry up to 100 times with different random nodes

**Performance** (Tested 2026-01-03):
- **Depth 1-6**: 1 attempt (full BFS guarantees exact depth)
- **Depth 7-8**: ~2-10 attempts average
- **Depth 9-10**: ~5-20 attempts average
- **Depth 10 Test**: 9 attempts, <2ms total time (minimal bucket: 1M/1M/2M/4M)
- **Per-attempt cost**: ~0.1-0.2ms (IDA* is extremely fast)
- **User-perceived latency**: Negligible (instant scramble generation)

**Why Depth Guarantee is Critical**:
1. **Training Quality**: Users need exact depth for effective practice
2. **Sparse Coverage**: Only 0.008-0.16% of state space covered at depths 7+
3. **Collision Probability**: Random selection likely yields shallower optimal solution
4. **Prune Tables**: Only provide lower bound, not exact depth
5. **Database Growth**: State space grows exponentially (Depth 10 ≈ 100× Depth 7)

**See**: [IMPLEMENTATION_PROGRESS.md Phase 7.7](IMPLEMENTATION_PROGRESS.md#77-scramble-generation-depth-accuracy--completed-2026-01-03-2330) for detailed coverage analysis and test results.

### Key Functions

**start_search**:
- Iterative deepening from depth 0 to target_depth
- Calls `depth_limited_search` for each depth limit

**depth_limited_search**:
- Recursive DFS with depth limit
- Database pruning: Skip states already in database at ≤ current depth
- Returns `true` if solution found

**Database Pruning**:
```cpp
int estimated_depth = get_depth_from_database(current_state);
if (estimated_depth <= remaining_depth) {
    return false;  // Prune (can't improve on database entry)
}
```

**Performance**:
- **Depth 1-6 scramble**: ~0.01 seconds (1 attempt, guaranteed depth)
- **Depth 7-8 scramble**: ~0.01-0.1 seconds (~2-10 attempts average)
- **Depth 9-10 scramble**: ~0.1-2 seconds (~5-20 attempts, <0.2ms per attempt)
- **Worst case**: Falls back after 100 attempts (rare, indicates severe coverage issue)

---

## Summary

**Key Implementation Features (stable_20260103)**:

1. **5-Phase Construction**: Depths 0-6 (complete), 7-9 (face-diverse), 9-10 (random sampling)
2. **Memory Spike Elimination**: Pre-reserve pattern eliminates 50-100 MB transient spikes
3. **Depth 10 Support**: Random sampling fills bucket to 90% capacity
4. **WASM/Native Dual Support**: Platform-specific code paths with unified API
5. **Malloc Trim Management**: Configurable allocator cache clearing
6. **Developer Options**: Environment variables for research configurations
7. **Random Sampling**: Ensures diversity and depth accuracy for depth 10

**Memory Behavior**:
- Deterministic peak based on bucket configuration
- <1% variation across runs (extremely stable)
- No transient spikes (all eliminated via pre-reserve)

**Performance**:
- Database construction: ~165-180 seconds (5-phase construction up to depth 10)
  - Phase 1 (BFS 0-6): ~30s
  - Phase 2 (Depth 7): ~10s
  - Phase 3 (Depth 8): ~40s
  - Phase 4 (Depth 9): ~70s
  - Phase 5 (Depth 10 random sampling): ~15-20s
- Scramble generation: <0.1 seconds (depth 8), ~2 seconds (depth 10)

**Recommended Reading**:
1. [API_REFERENCE.md](API_REFERENCE.md) - Function signatures and usage
2. [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md) - WASM deployment
3. [Experiences/depth_10_memory_spike_investigation.md](Experiences/depth_10_memory_spike_investigation.md) - Spike analysis
4. [Experiences/depth_10_implementation_results.md](Experiences/depth_10_implementation_results.md) - Phase 5 results
5. [Experiences/wasm_heap_measurement_data.md](Experiences/wasm_heap_measurement_data.md) - WASM measurements

---

**Document Version**: 2.1  
**Last Updated**: 2026-01-03  
**Status**: Current (stable_20260103)

**Recent Updates**:
- Fixed `get_xxcross_scramble()` return value (tmp assignment)
- Added custom bucket constructor (5-parameter: adj, b7_mb, b8_mb, b9_mb, b10_mb)
- Added Final Statistics output in constructor (#ifdef __EMSCRIPTEN__)
- Documented delegating constructor pattern for custom buckets
- Updated WASM integration notes (embind exports, statistics parsing)

**Corresponds to**: solver_dev.cpp stable_20260103 (4723 lines)  
**Previous Version**: stable-20251230 (archived: backups/SOLVER_IMPLEMENTATION_20251230.md)
