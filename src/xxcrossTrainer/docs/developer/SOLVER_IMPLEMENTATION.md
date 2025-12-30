# Solver Implementation Details

**Version**: stable-20251230  
**Last Updated**: 2025-12-30  
**Source**: solver_dev.cpp (3631 lines)

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md) | [Memory Config](../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [WASM Build](WASM_BUILD_GUIDE.md) | [Memory Monitoring](MEMORY_MONITORING.md) | [Experiments](Experiences/README.md)

---

## Purpose

This document explains the **core implementation logic** of the XXCross solver, focusing on:
1. **Database Construction** (Two-Phase BFS)
2. **Memory-Adaptive Bucket Allocation**
3. **IDA* Search** (for scramble generation)

For usage instructions, see [../USER_GUIDE.md](../USER_GUIDE.md). For memory configuration, see [../MEMORY_CONFIGURATION_GUIDE.md](../MEMORY_CONFIGURATION_GUIDE.md).

---

## Architecture Overview

### High-Level Flow

```
main()
  ├─ Read memory limit from argv[1]
  ├─ Calculate bucket sizes (flexible allocation)
  │   ├─ Determine bucket_7, bucket_8, bucket_9
  │   └─ Select from 7 configurations based on limit
  │
  ├─ Phase 1: Full BFS (Depth 0-6)
  │   ├─ Complete expansion of all states
  │   ├─ Store in index_pairs[0..6]
  │   └─ Free depth_6_nodes after phase
  │
  ├─ Phase 2: Local Expansion (Depth 7-9)
  │   ├─ Partial expansion using bucket constraints
  │   ├─ Depth 7: Complete expansion
  │   ├─ Depth 8: Local BFS from depth 7
  │   └─ Depth 9: Local BFS from depth 8
  │
  └─ Ready for search (IDA* or direct lookup)
```

### Key Data Structures

```cpp
// State representation
struct State {
    std::vector<int> cp;  // Corner permutation (8 elements)
    std::vector<int> co;  // Corner orientation (8 elements)
    std::vector<int> ep;  // Edge permutation (12 elements)
    std::vector<int> eo;  // Edge orientation (12 elements)
};

// Database storage
std::vector<std::vector<uint64_t>> index_pairs;  // [depth][state_index]

// Sliding window for BFS
struct SlidingDepthSets {
    tsl::robin_set<uint64_t> prev;  // Depth n-1
    tsl::robin_set<uint64_t> cur;   // Depth n
    tsl::robin_set<uint64_t> next;  // Depth n+1
    size_t max_total_nodes;         // Memory constraint
};
```

---

## Part 1: Database Construction

### Phase 1: Full BFS (Depth 0-6)

**Purpose**: Build complete database of all reachable states up to depth 6.

**Implementation**:

```cpp
int expand_one_depth(
    SlidingDepthSets& visited,
    int cur_depth,
    const std::vector<int>& move_table,
    std::vector<std::vector<uint64_t>>& index_pairs,
    bool verbose = true)
{
    int next_depth = cur_depth + 1;
    int move_idx;
    uint64_t next_idx;
    bool capacity_reached = false;
    bool stop = false;
    
    visited.next.attach_element_vector(&index_pairs[next_depth]);
    
    // Expand all states at current depth
    for (uint64_t idx : visited.cur) {
        for (int move = 0; move < 18; ++move) {  // All 18 moves
            move_idx = move_table[idx % move_table_size + 18 * (idx / move_table_size) + move];
            next_idx = (idx / move_table_size) * move_table_size + move_idx;
            
            // Mark if new (not in prev, cur, or next)
            if (visited.encounter_and_mark_next(next_idx, capacity_reached)) {
                if (capacity_reached) {
                    stop = true;
                    break;
                }
            }
        }
        if (stop) break;
    }
    
    // Rotate sets for next iteration
    visited.prev = std::move(visited.cur);
    visited.cur = std::move(visited.next);
    visited.next.clear();
    
    return stop ? (next_depth - 1) : next_depth;
}
```

**Key Features**:
- **Element Vector Attachment**: `next` set directly writes to `index_pairs[next_depth]` to avoid copying
- **Deduplication**: States already in `prev`, `cur`, or `next` are skipped
- **Move Table**: Pre-computed move transitions for fast state generation

**Memory Behavior**:
- Depths 0-6: ~20 MB total (compact storage)
- Depth 6: ~217 MB peak during expansion
- After Phase 1: `depth_6_nodes` freed (~100 MB reclaimed)

---

### Phase 2: Local Expansion (Depth 7-9)

**Purpose**: Expand depths 7, 8, 9 with **memory constraints** using bucket limits.

#### Bucket Allocation Strategy

The solver uses **fixed bucket configurations** based on total memory:

```cpp
void calculate_buckets_flexible(int memory_limit_mb,
                                 int& bucket_7_mb,
                                 int& bucket_8_mb,
                                 int& bucket_9_mb)
{
    const int EMPIRICAL_OVERHEAD = 33;  // bytes per slot
    
    // Configuration thresholds (empirically measured)
    if (memory_limit_mb >= 1702) {
        bucket_7_mb = 16; bucket_8_mb = 16; bucket_9_mb = 16;
    } else if (memory_limit_mb >= 1537) {
        bucket_7_mb = 8; bucket_8_mb = 16; bucket_9_mb = 8;
    } else if (memory_limit_mb >= 1000) {
        bucket_7_mb = 8; bucket_8_mb = 8; bucket_9_mb = 8;
    } else if (memory_limit_mb >= 921) {
        bucket_7_mb = 4; bucket_8_mb = 8; bucket_9_mb = 4;
    } else if (memory_limit_mb >= 613) {
        bucket_7_mb = 4; bucket_8_mb = 4; bucket_9_mb = 4;
    } else if (memory_limit_mb >= 569) {
        bucket_7_mb = 2; bucket_8_mb = 4; bucket_9_mb = 2;
    } else {
        bucket_7_mb = 2; bucket_8_mb = 2; bucket_9_mb = 2;
    }
}
```

**7 Configurations**:

| Limit Range | Config | Peak Memory | Use Case |
|-------------|--------|-------------|----------|
| 300-568 MB | 2M/2M/2M | 293 MB | Embedded |
| 569-612 MB | 2M/4M/2M | 404 MB | Mobile |
| 613-920 MB | 4M/4M/4M | 434 MB | Standard |
| 921-999 MB | 4M/8M/4M | 655 MB | - |
| 1000-1536 MB | 8M/8M/8M | 748 MB | **Production** |
| 1537-1701 MB | 8M/16M/8M | 1189 MB | High-memory |
| 1702+ MB | 16M/16M/16M | 1375 MB | Performance |

**Why These Specific Thresholds?**

Each threshold is calculated to ensure the **next configuration** fits within the limit:

```
Formula (simplified):
threshold = 62 (fixed overhead) + total_buckets × bytes_per_slot

For 8M/8M/8M:
threshold = 62 + (8+8+8) × 33 = 62 + 792 = 854 MB
With safety margin: 1000 MB
```

For full theoretical analysis, see [Experiences/MEMORY_THEORY_ANALYSIS.md](Experiences/MEMORY_THEORY_ANALYSIS.md).

---

#### Depth 7 Expansion

**Implementation**:

```cpp
// Allocate bucket for depth 7
SlidingDepthSets visited_7(bucket_7_mb * 1024 * 1024, verbose);

// Reserve bucket capacity
visited_7.cur.reserve(bucket_7_slots);

// Expand from depth 6
int final_depth = expand_one_depth(
    visited_7,
    6,  // cur_depth
    move_table,
    index_pairs,
    verbose
);
```

**Characteristics**:
- **Complete expansion**: All depth 7 states within bucket limit
- **Element vector**: `index_pairs[7]` populated directly
- **Memory peak**: Reached during this phase for most configurations

---

#### Depth 8 and 9 Expansion

**Local Expansion Strategy**:

Instead of expanding all states, the solver uses **parent-child relationships**:

```cpp
// Depth 8: Expand from sampled depth 7 states
for (uint64_t parent_idx : sampled_depth_7_states) {
    for (int move = 0; move < 18; ++move) {
        uint64_t child_idx = apply_move(parent_idx, move);
        if (is_depth_8(child_idx)) {
            visited_8.next.insert(child_idx);
        }
    }
}

// Depth 9: Expand from sampled depth 8 states
for (uint64_t parent_idx : sampled_depth_8_states) {
    for (int move = 0; move < 18; ++move) {
        uint64_t child_idx = apply_move(parent_idx, move);
        if (is_depth_9(child_idx)) {
            visited_9.next.insert(child_idx);
        }
    }
}
```

**Sampling Strategy**:
- **1 in 12 sampling** for depth 7→8 expansion (reduces search space)
- **Backtrace verification** to ensure reachability

**Memory Optimization**:
- Uses existing buckets (no additional allocation)
- Reuses `visited.cur` and `visited.next` sets
- Minimal transient memory

---

## Part 2: Move Tables

### Purpose

Move tables enable **O(1) state transitions** without recalculating cube states.

### Edge Move Table

```cpp
std::vector<int> create_edge_move_table() {
    std::vector<int> move_table(24 * 18, -1);  // 24 edge states × 18 moves
    
    for (int edge_state = 0; edge_state < 24; ++edge_state) {
        // edge_state = 2*position + orientation
        int edge_piece = edge_state / 2;
        int orientation = edge_state % 2;
        
        for (int move = 0; move < 18; ++move) {
            State result = apply_move_to_edge(edge_piece, orientation, move);
            int new_position = find_edge_position(result, edge_piece);
            int new_orientation = get_edge_orientation(result, new_position);
            
            move_table[18 * edge_state + move] = 2 * new_position + new_orientation;
        }
    }
    
    return move_table;
}
```

### Corner Move Table

Similar structure for corner pieces:

```cpp
std::vector<int> create_corner_move_table() {
    std::vector<int> move_table(24 * 18, -1);  // 24 corner states × 18 moves
    
    for (int corner_state = 0; corner_state < 24; ++corner_state) {
        // corner_state = 3*position + orientation
        int corner_piece = corner_state / 3;
        int orientation = corner_state % 3;
        
        for (int move = 0; move < 18; ++move) {
            State result = apply_move_to_corner(corner_piece, orientation, move);
            int new_position = find_corner_position(result, corner_piece);
            int new_orientation = get_corner_orientation(result, new_position);
            
            move_table[18 * corner_state + move] = 3 * new_position + new_orientation;
        }
    }
    
    return move_table;
}
```

### Multi-Piece Move Table

For combinations of pieces (e.g., 5 edges for XXCross):

```cpp
void create_multi_move_table(
    int n,                          // Number of pieces
    int c,                          // Orientation types (2 for edges, 3 for corners)
    int pn,                         // Total pieces in class (12 edges, 8 corners)
    int size,                       // Table size
    const std::vector<int>& table,  // Single-piece table
    std::vector<int>& move_table)   // Output
{
    move_table = std::vector<int>(size * 18, -1);
    
    std::vector<int> state(n);
    std::vector<int> new_state(n);
    
    for (int index = 0; index < size; ++index) {
        index_to_array(state, index, n, c, pn);  // Decode state
        
        for (int move = 0; move < 18; ++move) {
            // Apply move to each piece
            for (int i = 0; i < n; ++i) {
                new_state[i] = table[state[i] + move];
            }
            
            int new_index = array_to_index(new_state, n, c, pn);  // Encode
            move_table[index * 18 + move] = new_index;
        }
    }
}
```

**Performance**: Pre-computing moves reduces expansion time by ~100× compared to recalculating states.

---

## Part 3: IDA* Search (Scramble Generation)

### Purpose

Generate valid scrambles from the database using **IDA* (Iterative Deepening A*)**.

### Implementation

```cpp
bool ida_star_search(
    uint64_t target_index,      // Goal state index
    int target_depth,           // Known depth of target
    std::vector<int>& solution, // Output sequence
    const std::vector<int>& move_table,
    const std::vector<std::vector<uint64_t>>& index_pairs)
{
    // Iterative deepening
    for (int max_depth = 0; max_depth <= target_depth; ++max_depth) {
        std::vector<int> path;
        if (dfs(0, max_depth, target_index, target_depth, path, move_table, index_pairs)) {
            solution = path;
            return true;
        }
    }
    
    return false;
}

bool dfs(
    uint64_t current_index,
    int remaining_depth,
    uint64_t target_index,
    int target_depth,
    std::vector<int>& path,
    const std::vector<int>& move_table,
    const std::vector<std::vector<uint64_t>>& index_pairs)
{
    // Base case: found target
    if (current_index == target_index && path.size() == target_depth) {
        return true;
    }
    
    // Pruning: no moves left
    if (remaining_depth == 0) {
        return false;
    }
    
    // Heuristic: minimum moves to target
    int h = estimate_distance(current_index, target_index, index_pairs);
    if (path.size() + h > target_depth) {
        return false;  // Cannot reach target in time
    }
    
    // Try all moves
    for (int move = 0; move < 18; ++move) {
        // Pruning: avoid redundant moves
        if (!path.empty() && is_redundant(path.back(), move)) {
            continue;
        }
        
        uint64_t next_index = apply_move(current_index, move, move_table);
        path.push_back(move);
        
        if (dfs(next_index, remaining_depth - 1, target_index, target_depth, path, move_table, index_pairs)) {
            return true;
        }
        
        path.pop_back();
    }
    
    return false;
}
```

### Heuristic Function

```cpp
int estimate_distance(
    uint64_t current_index,
    uint64_t target_index,
    const std::vector<std::vector<uint64_t>>& index_pairs)
{
    // Find depth of current state in database
    for (int depth = 0; depth < index_pairs.size(); ++depth) {
        if (std::find(index_pairs[depth].begin(), 
                      index_pairs[depth].end(), 
                      current_index) != index_pairs[depth].end()) {
            
            // Find depth of target state
            for (int target_d = 0; target_d < index_pairs.size(); ++target_d) {
                if (std::find(index_pairs[target_d].begin(),
                              index_pairs[target_d].end(),
                              target_index) != index_pairs[target_d].end()) {
                    
                    return abs(target_d - depth);  // Minimum moves
                }
            }
        }
    }
    
    return 0;  // Unknown, assume close
}
```

**Optimization**: For large databases, use **hash lookups** instead of linear search.

---

## Part 4: Memory Management

### RSS Monitoring

The solver can monitor its own memory usage:

```cpp
size_t get_rss_kb() {
#ifdef __EMSCRIPTEN__
    return 0;  // Not available in WebAssembly
#else
    std::ifstream status("/proc/self/status");
    std::string line;
    
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t pos = line.find_last_of(" \t");
            if (pos != std::string::npos) {
                return std::stoull(line.substr(6, pos - 6));
            }
        }
    }
    
    return 0;
#endif
}
```

Usage:

```cpp
size_t rss_before = get_rss_kb();
expand_one_depth(visited, cur_depth, move_table, index_pairs, verbose);
size_t rss_after = get_rss_kb();

std::cout << "RSS increase: " << (rss_after - rss_before) / 1024 << " MB" << std::endl;
```

### Memory Composition

For 8M/8M/8M configuration:

```
Total Peak: 748 MB
├─ Fixed Overhead: 62 MB
│  ├─ C++ runtime: ~15 MB
│  ├─ Phase 1 remnants: ~20 MB
│  ├─ OS allocator: ~10 MB
│  └─ Misc: ~17 MB
│
└─ Bucket Memory: 686 MB
   ├─ Bucket arrays: (8M+8M+8M) × 4 bytes = 96 MB
   ├─ Robin set nodes: ~20M nodes × 24 bytes = ~480 MB
   └─ index_pairs: ~20M indices × 8 bytes = ~160 MB
   └─ Padding/fragmentation: ~(-50) MB (underutilization)
```

**Load Factor Variation**:
- Small buckets (2M-4M): ~90-95% utilization
- Large buckets (8M+): ~60-65% utilization

See [Experiences/MEMORY_THEORY_ANALYSIS.md](Experiences/MEMORY_THEORY_ANALYSIS.md) for detailed analysis.

---

## Part 5: WebAssembly Specifics

### Build Configuration

```bash
emcc -std=c++17 -O3 solver_dev.cpp -o solver.js \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \  # 256 MB
  -s MAXIMUM_MEMORY=2147483648 \  # 2 GB
  --bind \
  -I/path/to/tsl-robin-map/include
```

### Memory Growth Handling

```cpp
#ifdef __EMSCRIPTEN__
EM_ASM({
    console.log('Current memory: ' + (HEAP8.length / 1024 / 1024) + ' MB');
});
#endif
```

### JavaScript Bindings

```cpp
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(solver_module) {
    emscripten::function("solve", &solve_scramble);
    emscripten::function("generate_scramble", &get_xxcross_scramble);
}
#endif
```

For complete WASM build instructions, see [WASM_BUILD_GUIDE.md](WASM_BUILD_GUIDE.md).

---

## Performance Characteristics

### Time Complexity

| Phase | Operation | Time | Memory |
|-------|-----------|------|--------|
| Phase 1 | BFS 0-6 | ~30s | ~217 MB peak |
| Phase 2 (7) | BFS depth 7 | ~10s | Config-dependent |
| Phase 2 (8) | Local BFS 7→8 | ~40s | Same as depth 7 |
| Phase 2 (9) | Local BFS 8→9 | ~70s | Same as depth 7 |
| **Total** | **Database build** | **~150s** | **Config-dependent** |

### Space Complexity

| Configuration | Nodes | Memory | Bytes/Node |
|---------------|-------|--------|------------|
| 2M/2M/2M | ~6M | 293 MB | 48.8 |
| 4M/4M/4M | ~12M | 434 MB | 36.2 |
| 8M/8M/8M | ~24M | 748 MB | 31.2 |
| 16M/16M/16M | ~48M | 1375 MB | 28.6 |

**Pattern**: Larger buckets are more memory-efficient per node.

---

## Code Organization

### File Structure

```
solver_dev.cpp (3631 lines)
├─ Lines 1-100: Headers, State struct, move definitions
├─ Lines 100-300: Move table creation functions
├─ Lines 300-600: Index encoding/decoding utilities
├─ Lines 600-800: SlidingDepthSets class (BFS engine)
├─ Lines 800-1200: Bucket allocation logic
├─ Lines 1200-2000: Local expansion functions
├─ Lines 2000-2800: IDA* search implementation
├─ Lines 2800-3200: Scramble generation
└─ Lines 3200-3631: Main function, CLI interface
```

### Key Functions

- `calculate_buckets_flexible()`: Determine bucket sizes from memory limit
- `expand_one_depth()`: Single BFS iteration
- `create_edge_move_table()` / `create_corner_move_table()`: Pre-compute moves
- `ida_star_search()`: Find scramble sequence
- `get_xxcross_scramble()`: Generate random scramble
- `start_search()`: Solve from given scramble

---

## Future Optimizations

### Potential Improvements

1. **Custom Allocator**: Reduce fixed 62 MB overhead
2. **Compressed State Encoding**: Reduce bytes/node from 32 to ~20
3. **Parallel BFS**: Multi-threaded expansion for depths 7-9
4. **Persistent Database**: Save/load pre-built database to skip construction
5. **Pruning Tables**: More aggressive heuristics for IDA*

### Trade-offs

| Optimization | Speed Gain | Memory Change | Complexity |
|--------------|------------|---------------|------------|
| Custom allocator | - | -20% | High |
| Compressed states | - | -30% | High |
| Parallel BFS | +2× | - | Medium |
| Persistent DB | +100× | Disk I/O | Low |
| Better pruning | +20% | - | Medium |

---

## Summary

**Key Implementation Features**:
- **Two-Phase BFS**: Complete depth 0-6, local 7-9
- **Memory-Adaptive**: 7 configurations from 300 MB to 2 GB
- **Move Tables**: O(1) state transitions
- **IDA* Search**: Optimal scramble generation
- **Production-Ready**: Stable, tested, documented

**Memory Behavior**:
- Deterministic peak based on configuration
- <0.03% variation (extremely stable)
- No transient spikes for large buckets

**Performance**:
- ~150 seconds database build
- Scales with bucket size but **not** speed
- Configuration-independent solving time

For further reading:
- [MEMORY_MONITORING.md](MEMORY_MONITORING.md) - RSS tracking techniques
- [EXPERIMENT_SCRIPTS.md](EXPERIMENT_SCRIPTS.md) - Testing methodology
- [Experiences/MEMORY_THEORY_ANALYSIS.md](Experiences/MEMORY_THEORY_ANALYSIS.md) - Theoretical understanding

---

**Document Version**: 1.0  
**Corresponds to**: solver_dev.cpp stable-20251230
