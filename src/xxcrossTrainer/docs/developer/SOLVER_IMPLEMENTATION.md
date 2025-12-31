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

**Purpose**: Expand depths 7, 8, 9 with **memory constraints** using dynamically calculated bucket sizes.

#### Bucket Allocation Strategy

The solver uses **calculation-based bucket allocation** that determines sizes at runtime:

```cpp
LocalExpansionConfig determine_bucket_sizes(
    int max_depth,
    size_t remaining_memory_bytes,
    size_t bucket_n_current,
    size_t cur_set_memory_bytes,
    bool verbose)
{
    // Calculate usable memory (remaining - safety margin)
    size_t wasm_safety_margin = remaining_memory * (20-25)/100;
    size_t usable_memory = remaining_memory - wasm_safety_margin;
    
    // Try candidate bucket ratios (1:2 or 1:1 for n+1:n+2)
    // Prefer n+1 <= n+2 for better coverage
    std::vector<BucketRatio> candidates = {
        {32M, 64M, "1:2"},  // Best for large memory
        {16M, 32M, "1:2"},
        {8M, 16M, "1:2"},
        {4M, 8M, "1:2"},
        {2M, 4M, "1:2"},
        {32M, 32M, "1:1"},  // Fallback ratios
        {16M, 16M, "1:1"},
        // ... etc
    };
    
    // Select best configuration that fits in memory
    for (auto candidate : candidates) {
        auto params_n1 = g_expansion_params.get(max_depth, candidate.bucket_n1);
        auto params_n2 = g_expansion_params.get(max_depth, candidate.bucket_n2);
        
        // Calculate expected nodes using empirical load factors
        size_t nodes_n1 = bucket_n1 * params_n1.effective_load_factor;
        size_t nodes_n2 = bucket_n2 * params_n2.backtrace_load_factor;
        
        // Calculate memory using empirical memory factor
        size_t predicted_rss = total_memory * params_n2.measured_memory_factor;
        
        if (predicted_rss <= usable_memory * 0.98) {  // 98% safety
            return candidate;  // Found suitable configuration
        }
    }
}
```

**Key Points**:
- **Dynamic Calculation**: Bucket sizes determined by runtime logic, not lookup tables
- **Empirical Parameters**: Uses measured load factors and memory factors from `g_expansion_params`
- **Safety Margins**: 20-25% buffer for WASM compatibility and rehash protection  
- **Ratio Selection**: Prefers 1:2 ratio (n+1:n+2) for optimal coverage
- **Stability**: While calculation-based, empirically-tuned constants are now fixed

**Typical Configurations** (determined by calculation logic):

| Memory Available | Calculated Config | Expected Nodes | Usage |
|------------------|-------------------|----------------|--------|
| ~400 MB | 2M/4M | ~1.5M/2.5M | Mobile |
| ~700 MB | 4M/8M or 8M/8M | ~3M/5M or ~5M/5M | Standard |
| ~1200 MB | 8M/16M | ~5M/10M | High-memory |
| ~1700 MB | 16M/32M | ~10M/20M | Performance |

For detailed theoretical analysis of this calculation approach, see [Experiences/MEMORY_THEORY_ANALYSIS.md](Experiences/MEMORY_THEORY_ANALYSIS.md).

---

#### Node Expansion Strategy (Local BFS)

**Critical Design**: Random parent selection for diversity and load factor optimization.

##### Step 2: Random Expansion for Depth n+1

**Purpose**: Generate depth n+1 nodes by randomly selecting parents from depth n.

**Why Random Selection?**

1. **Parent Diversity (Hash Level)**: 
   - Random selection ensures hash table entries are spread across buckets
   - Prevents clustering in hash space
   - Maintains high load factor (~0.90) without rehashing

2. **Parent Diversity (Real Node Level)**:
   - Different parents produce different children (move combinations)
   - Diverse parents → diverse coverage of state space
   - Critical for optimal solving performance

3. **Child Node Randomness**:
   - Random parent × 18 moves → random child distribution
   - Child randomness maintains parent diversity for next depth
   - Prevents hash collisions and improves load factor

**Implementation**:

```cpp
void local_expand_step2_random_n1(
    const LocalExpansionConfig &config,
    const tsl::robin_set<uint64_t> &depth_n_nodes,
    tsl::robin_set<uint64_t> &depth_n1_nodes,
    ...)
{
    // Convert depth=n nodes to vector for random access
    std::vector<uint64_t> depth_n_vec(depth_n_nodes.begin(), depth_n_nodes.end());
    std::uniform_int_distribution<size_t> dist(0, depth_n_vec.size() - 1);
    
    // Prevent rehash by pre-allocating bucket
    depth_n1_nodes.rehash(config.bucket_n1);
    depth_n1_nodes.max_load_factor(0.9f);
    
    while (processed_parents < max_parent_nodes) {
        // Random parent selection
        size_t random_idx = dist(gen);
        uint64_t parent_node = depth_n_vec[random_idx];
        
        // Expand all 18 moves from this parent
        for (int move = 0; move < 18; ++move) {
            uint64_t child_node = apply_move(parent_node, move, move_tables);
            
            // Skip if already in depth n or n+1
            if (depth_n_nodes.find(child_node) != depth_n_nodes.end() ||
                depth_n1_nodes.find(child_node) != depth_n1_nodes.end()) {
                continue;
            }
            
            // Check for imminent rehash
            if (depth_n1_nodes.will_rehash_on_next_insert()) {
                goto expansion_complete;  // Stop before rehash
            }
            
            // Insert child (automatically recorded in index_pairs via element_vector)
            depth_n1_nodes.insert(child_node);
        }
        
        processed_parents++;
    }
    
expansion_complete:
    depth_n1_nodes.detach_element_vector();  // Finalize recording
}
```

**Key Optimizations**:

1. **Rehash Protection**: 
   - `will_rehash_on_next_insert()` checks before insertion
   - Stops expansion before rehash (prevents memory spike)
   - Achieves ~88-90% load factor safely

2. **Element Vector Attachment**:
   - `depth_n1_nodes.attach_element_vector(&index_pairs[n+1])`
   - Inserts automatically record to `index_pairs` (zero-copy)
   - Avoids post-processing step

3. **Memory Efficiency**:
   - Reuses `depth_n_vec` (no additional allocation)
   - Single-pass expansion (no intermediate storage)
   - Directly writes to final storage

---

##### Step 5: Backtrace Expansion for Depth n+2

**Purpose**: Generate depth n+2 nodes with verification to avoid false positives.

**Strategy**: Expand from depth n+1, but filter using backtrace to depth n.

**Why Backtrace?**

When expanding locally, we risk adding nodes that are actually reachable from depth < n+2:
- A node at "depth n+2" in local expansion might be reachable from depth n in 1 move
- Backtrace verification ensures true depth assignment
- Maintains database integrity for optimal solving

**Implementation**:

```cpp
BacktraceExpansionResult expand_with_backtrace_filter(
    tsl::robin_set<uint64_t> &depth_n_nodes,   // For backtrace
    tsl::robin_set<uint64_t> &depth_n1_nodes,  // Parents
    tsl::robin_set<uint64_t> &depth_n2_nodes,  // Children
    ...)
{
    // Sample 1/12 of depth n+1 nodes as parents
    std::vector<uint64_t> sampled_parents;
    for (auto node : depth_n1_nodes) {
        if (dist_12(gen) == 0) {  // 1/12 probability
            sampled_parents.push_back(node);
        }
    }
    
    for (auto parent : sampled_parents) {
        for (int move = 0; move < 18; ++move) {
            uint64_t child = apply_move(parent, move, move_tables);
            
            // Skip if already seen
            if (depth_n_nodes.find(child) != depth_n_nodes.end() ||
                depth_n1_nodes.find(child) != depth_n1_nodes.end() ||
                depth_n2_nodes.find(child) != depth_n2_nodes.end()) {
                continue;
            }
            
            // Backtrace verification
            bool valid_n2 = true;
            for (int backtrace_move = 0; backtrace_move < 18; ++backtrace_move) {
                uint64_t backtrace_node = apply_move(child, backtrace_move, move_tables);
                if (depth_n_nodes.find(backtrace_node) != depth_n_nodes.end()) {
                    // Child is reachable from depth n in 1 move → reject
                    valid_n2 = false;
                    rejected_n1_nodes++;
                    break;
                }
            }
            
            if (valid_n2) {
                // Check rehash before insert
                if (depth_n2_nodes.will_rehash_on_next_insert()) {
                    goto backtrace_expansion_end;
                }
                depth_n2_nodes.insert(child);
            }
        }
    }
    
backtrace_expansion_end:
    return {depth_n2_nodes.size(), sampled_parents.size(), rejected_n1_nodes, ...};
}
```

**Sampling Strategy**:
- **1 in 12 sampling**: Reduces computational cost while maintaining coverage
- **Random selection**: Ensures diverse parent set
- **Sufficient coverage**: Empirically validated to fill buckets effectively

**Backtrace Filter Benefits**:
- Ensures depth assignment accuracy
- Prevents solving errors (incorrect depth → wrong solutions)
- Small cost (~10% overhead) for correctness guarantee

---

#### Memory Optimization Techniques

**1. In-Place Expansion**:
```cpp
// Directly write to index_pairs during insertion
depth_n1_nodes.attach_element_vector(&index_pairs[n+1]);
depth_n1_nodes.insert(node);  // Automatically records to vector
```

**2. Rehash Prevention**:
```cpp
// Check before every insertion
if (robin_set.will_rehash_on_next_insert()) {
    stop_expansion();  // Prevents memory spike
}
```

**3. Set Rotation** (Phase 1 BFS):
```cpp
// Reuse memory across depths
visited.prev = std::move(visited.cur);
visited.cur = std::move(visited.next);
visited.next.clear();  // Ready for next depth
```

**4. Element Vector Optimization**:
- Zero-copy recording (no separate storage step)
- Automatic during insertion (no post-processing)
- Detach after expansion to free robin_set overhead

**5. Load Factor Control**:
```cpp
robin_set.max_load_factor(0.9f);  // High load for memory efficiency
```

**Memory Behavior**:
- **Predictable peaks**: Determined by bucket size × load factor
- **No spikes**: Rehash protection ensures stable memory
- **Efficient reuse**: Move semantics avoid copying large sets

---

## Part 2: Common Systems

### Move Tables

Move tables provide **O(1) state transitions** for cube pieces. This is a **standard system** used across all solvers in this repository and is considered foundational knowledge.

**Concept**: Pre-compute all piece transitions for 18 moves:

```cpp
std::vector<int> edge_move_table(24 * 18);  // 24 states × 18 moves
std::vector<int> corner_move_table(24 * 18);

// Usage: O(1) lookup
int new_edge_state = edge_move_table[current_state * 18 + move];
```

For detailed implementation, see move table creation functions in lines 220-340 of solver_dev.cpp.

---

### IDA* Search

IDA* (Iterative Deepening A*) is used for **scramble generation** from the database. This is another **standard system** used repository-wide.

**High-level Flow**:
1. Select random state from database at depth d
2. Use IDA* to find path from solved state to target
3. Return path as scramble sequence

This functionality is well-established and documented in other solvers within this repository.

---

## Part 3: Sparse Database Construction System

### Overview

The core innovation of this solver is the **sparse database construction system** - a collection of classes and functions that enable memory-adaptive BFS with local expansion.

**Key Components** (Lines 350-2500):

1. **SlidingDepthSets** - BFS engine with rehash protection
2. **LocalExpansionConfig** - Configuration for local expansion
3. **ExpansionParams** - Empirical parameters (load factors, memory factors)
4. **determine_bucket_sizes()** - Dynamic bucket allocation logic
5. **local_expand_step2_random_n1()** - Random parent expansion
6. **expand_with_backtrace_filter()** - Backtrace verification expansion
7. **local_bfs_n_to_n2()** - Complete local expansion coordinator

### SlidingDepthSets Class (Lines 350-550)

**Purpose**: Sliding window BFS engine with three robin_sets (prev, cur, next).

**Key Features**:
```cpp
struct SlidingDepthSets {
    tsl::robin_set<uint64_t> prev, cur, next;
    size_t max_total_nodes;
    bool expansion_stopped;
    
    // Rehash protection
    bool will_rehash_on_next_insert() {
        return next.size() >= next.load_threshold();
    }
    
    // Zero-copy recording
    void attach_element_vector(std::vector<uint64_t>* vec) {
        next.attach_element_vector(vec);
    }
    
    // Memory-efficient rotation
    void advance_depth() {
        prev = std::move(cur);
        cur = std::move(next);
        next.clear();
    }
};
```

**Innovations**:
- **Rehash prediction**: Stops before memory spike
- **Element vector**: Direct recording to index_pairs (zero-copy)
- **Move semantics**: Efficient set rotation without copying

### LocalExpansionConfig (Lines 800-890)

**Purpose**: Container for bucket configuration and expansion parameters.

```cpp
struct LocalExpansionConfig {
    bool enable_n1_expansion;    // Can we expand to depth n+1?
    bool enable_n2_expansion;    // Can we expand to depth n+2?
    
    size_t bucket_n;             // Current depth bucket size
    size_t bucket_n1;            // Depth n+1 bucket size
    size_t bucket_n2;            // Depth n+2 bucket size
    
    size_t nodes_n1;             // Expected nodes at n+1
    size_t nodes_n2;             // Expected nodes at n+2
};
```

Returned by `determine_bucket_sizes()` based on available memory.

### ExpansionParams Class (Lines 550-800)

**Purpose**: Store empirical parameters for each (depth, bucket_size) combination.

```cpp
struct ExpansionParams {
    double effective_load_factor;      // For random expansion (Step 2)
    double backtrace_load_factor;      // For backtrace expansion (Step 5)
    double measured_memory_factor;     // Actual RSS / theoretical memory
};

// Global parameter database
ExpansionParamsDatabase g_expansion_params;

// Usage
auto params = g_expansion_params.get(max_depth, bucket_size);
double expected_load = params.effective_load_factor;
```

**Empirical Tuning**:
- Measured from 660,000+ RSS samples
- Stable across configurations (<0.03% variation)
- Used for accurate bucket selection

### Bucket Allocation Logic (Lines 890-1080)

**determine_bucket_sizes()** - The heart of memory-adaptive allocation:

**Algorithm**:
1. Calculate usable memory (remaining - safety margin)
2. Try candidate bucket ratios (1:2, 1:1 for n+1:n+2)
3. For each candidate:
   - Get empirical load factors from g_expansion_params
   - Calculate expected nodes: `bucket * load_factor`
   - Calculate predicted RSS: `total_memory * memory_factor`
   - Check if RSS <= usable_memory * 0.98 (safety)
4. Select best configuration (maximum total nodes)

**Fallback Strategy**:
- If n+2 not possible → try n+1 only
- If n+1 not possible → skip local expansion

### Random Parent Expansion (Lines 1083-1280)

**local_expand_step2_random_n1()** - Generate depth n+1 nodes:

**Key Steps**:
1. Pre-allocate bucket: `depth_n1_nodes.rehash(bucket_n1)`
2. Convert depth n nodes to vector for random access
3. Random selection loop:
   - Select random parent
   - Expand all 18 moves
   - Check rehash before each insert
   - Stop when `will_rehash_on_next_insert()` returns true
4. Detach element_vector to finalize

**Why This Works**:
- Random parents → diverse hash distribution
- Diverse hash → high load factor without rehashing
- Random children → parent diversity for next depth

### Backtrace Expansion (Lines 1564-1900)

**expand_with_backtrace_filter()** - Generate depth n+2 with verification:

**Key Steps**:
1. Sample 1/12 of depth n+1 nodes as parents
2. For each parent:
   - Expand all 18 moves to get children
   - For each child:
     - Backtrace: try all 18 moves backward
     - If backtrace reaches depth n → reject (false n+2)
     - If valid → insert (check rehash first)
3. Return statistics (nodes added, rejected, etc.)

**Backtrace Verification**:
- Ensures depth n+2 nodes are NOT reachable from depth n in 1 move
- Prevents database corruption
- Small overhead (~10%) for correctness

### Local BFS Coordinator (Lines 2900-3500)

**local_bfs_n_to_n2()** - Orchestrates complete local expansion:

**Flow**:
```
Step 0: determine_bucket_sizes()  → config
Step 1: Calculate capacity N from config
Step 2: local_expand_step2_random_n1()  → depth n+1 nodes
Step 3: Data reorganization (prev/cur/next sets)
Step 4: Build sampled_nodes (1/12 of n+1)
Step 5: expand_with_backtrace_filter()  → depth n+2 nodes
Step 6: Statistics and cleanup
```

**Memory Management**:
- Careful set rotation to minimize peak
- Element vector attachment for zero-copy
- Explicit cleanup of intermediate data

---

## Part 4: Class Structure (xxcross_search)

### Overview

The solver is organized as a single class `xxcross_search` that encapsulates all solver functionality. All major components (move tables, database, search functions) are organized within this class.

```cpp
struct xxcross_search {
    // Move tables (pre-computation)
    std::vector<int> edge_move_table;
    std::vector<int> corner_move_table;
    std::vector<int> multi_move_table_cross_edges;
    std::vector<int> multi_move_table_F2L_slots_edges;
    std::vector<int> multi_move_table_F2L_slots_corners;
    
    // Pruning tables
    std::vector<unsigned char> prune_table1;
    std::vector<unsigned char> prune_table23_couple;
    
    // Database (BFS results)
    std::vector<std::vector<uint64_t>> index_pairs;
    
    // Search state
    std::vector<int> sol;
    std::string scramble;
    int max_length;
    // ... other search variables
    
    // Constructor: performs ALL pre-computation
    xxcross_search(bool adj = true, 
                   int BFS_DEPTH = 6, 
                   int MEMORY_LIMIT_MB = 1600, 
                   bool verbose = true);
    
    // Public API
    std::string start_search(std::string arg_scramble = "");
    std::string get_xxcross_scramble(std::string arg_length = "7");
    std::string func(std::string arg_scramble = "", std::string arg_length = "7");
};
```

### Constructor (Lines 3325-3400)

All pre-computation happens in the constructor:

```cpp
xxcross_search::xxcross_search(bool adj, int BFS_DEPTH, int MEMORY_LIMIT_MB, bool verbose) {
    // Step 1: Create move tables
    edge_move_table = create_edge_move_table();
    corner_move_table = create_corner_move_table();
    create_multi_move_table(4, 2, 12, 24*22*20*18, edge_move_table, multi_move_table_cross_edges);
    create_multi_move_table(2, 2, 12, 24*22, edge_move_table, multi_move_table_F2L_slots_edges);
    create_multi_move_table(2, 3, 8, 24*21, corner_move_table, multi_move_table_F2L_slots_corners);
    
    // Step 2: Create pruning tables
    prune_table1 = std::vector<unsigned char>(24*22*20*18, 255);
    prune_table23_couple = std::vector<unsigned char>(24*22*24*21, 255);
    create_prune_table(index1, size1, 8, multi_move_table_cross_edges, prune_table1);
    create_prune_table2(index2, index3, size2, size3, 9, 
                        multi_move_table_F2L_slots_edges, 
                        multi_move_table_F2L_slots_corners, 
                        prune_table23_couple);
    
    // Step 3: Build complete search database (BFS + Local Expansion)
    build_complete_search_database(
        index1, index2, index3,
        size1, size2, size3,
        multi_move_table_cross_edges,
        multi_move_table_F2L_slots_edges,
        multi_move_table_F2L_slots_corners,
        index_pairs,  // Output: populated with BFS results
        BFS_DEPTH,
        MEMORY_LIMIT_MB,
        verbose
    );
}
```

**Design Rationale**:
- **All-in-one initialization**: After construction, the solver is immediately ready for search
- **No separate initialization methods**: Reduces user error (forgetting to call init)
- **RAII pattern**: Resource acquisition is initialization
- **Member storage**: Move tables and database persist for repeated searches

**Memory Lifecycle**:
1. Construction: All tables and database built (~1-2 GB)
2. Search calls: Use pre-built data (no additional allocation)
3. Destruction: All resources automatically freed

### Search Functions (Lines 3460-3550)

After the constructor completes, these functions can be called repeatedly:

**start_search()** (Lines 3505-3543):

```cpp
std::string start_search(std::string arg_scramble = "") {
    // Purpose: Solve a given scramble to find XXCross solution
    // Uses: Pre-built database (index_pairs) + IDA* search
    
    // Parse scramble and apply to goal state
    std::vector<int> alg = StringToAlg(scramble);
    for (int move : alg) {
        index1 = multi_move_table_cross_edges[index1 * 18 + move];
        index2 = multi_move_table_F2L_slots_edges[index2 * 18 + move];
        index3 = multi_move_table_F2L_slots_corners[index3 * 18 + move];
    }
    
    // Get pruning table lower bounds
    prune1_tmp = prune_table1[index1];
    prune23_tmp = prune_table23_couple[index2 * size3 + index3];
    
    // Iterative deepening search
    for (int d = std::max(prune1_tmp, prune23_tmp); d <= max_length; ++d) {
        if (depth_limited_search(index1, index2, index3, d, 324)) {
            break;  // Solution found
        }
    }
    
    return AlgToString(sol);
}
```

**Key Points**:
- Uses pre-built `multi_move_table_*` for O(1) state transitions
- Pruning tables provide lower bound for IDA* depth
- No database construction during search (all pre-built)

**get_xxcross_scramble()** (Lines 3460-3500):

```cpp
st::string get_xxcross_scramble(std::string arg_length = "7") {
    // Purpose: Generate a random scramble of specified length
    // Uses: Pre-built database to select random state at target depth
    
    int len = std::stoi(arg_length);
    
    // Select random state from database at depth=len
    std::uniform_int_distribution<int> dist(0, index_pairs[len].size() - 1);
    int random_index = dist(generator);
    uint64_t selected_state = index_pairs[len][random_index];
    
    // Decode state and solve back to scrambled position
    // (IDA* search from selected_state to goal)
    // ...
    
    return tmp;  // Scramble string
}
```

**Key Points**:
- Database (`index_pairs[depth]`) stores all states at each depth
- Random sampling ensures variety
- No real-time BFS required (uses pre-built database)

**func()** - Convenience wrapper:
```cpp
std::string func(std::string arg_scramble = "", std::string arg_length = "7") {
    // Solves arg_scramble, then generates new scramble of length arg_length
    return arg_scramble + start_search(arg_scramble) + "," + get_xxcross_scramble(arg_length);
}
```

**Note**: `depth_limited_search()` is omitted from documentation as it is a standard IDA* implementation (common repository knowledge).

---

## Part 5: Memory Management

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

### robin_hash Capacity Monitoring and Memory Release

The solver uses several advanced features from `tsl::robin_hash` (documented in [src/tsl/ELEMENT_VECTOR_FEATURE.md](../../../tsl/ELEMENT_VECTOR_FEATURE.md)):

**Capacity Monitoring API**:
- `load_threshold()` - Get precomputed rehash threshold (bucket_count × max_load_factor)
- `available_capacity()` - Calculate remaining capacity before rehash
- `will_rehash_on_next_insert()` - Predict if next insert will trigger rehash

**Usage in solver_dev.cpp**:
```cpp
// Prevent rehash during expansion
while (!depth_n1_nodes.will_rehash_on_next_insert()) {
    depth_n1_nodes.insert(child_node);
}
```

**Explicit Memory Release via swap()**:

While `tsl::robin_hash` provides `clear()`, it does NOT release the allocated bucket memory. To force memory deallocation, use the **swap idiom**:

```cpp
// Standard clear() - does NOT free memory
depth_n1_nodes.clear();  
// Bucket array still allocated (size=0, capacity unchanged)

// Explicit memory release via swap
tsl::robin_set<uint64_t>().swap(depth_n1_nodes);
// Creates temporary empty set, swaps with depth_n1_nodes, temporary destructs
// Result: depth_n1_nodes now has 0 capacity, all memory freed
```

**Real Usage in solver_dev.cpp** (Lines 2850-2900):
```cpp
// After depth 7 expansion complete
index_pairs[7] = std::move(depth_7_vector);  // Transfer ownership

// Free intermediate depth 6 data
tsl::robin_set<uint64_t>().swap(depth_6_nodes);  // Explicit memory release
index_pairs[6].clear();  // Clear vector
index_pairs[6].shrink_to_fit();  // Release vector capacity

std::cout << "RSS after depth 6 cleanup: " << (get_rss_kb() / 1024.0) << " MB" << std::endl;
```

**Why swap() instead of clear()**:
- `clear()` only removes elements, bucket array remains allocated
- `swap()` with temporary empty set forces complete deallocation
- Critical in memory-constrained environments (WASM, embedded systems)
- Reduces peak RSS by ~200-400 MB in typical configurations

**Pattern Summary**:
```cpp
container.clear();           // Elements removed, capacity unchanged
container.shrink_to_fit();   // Hint to reduce capacity (not guaranteed)
Container().swap(container); // Guaranteed full memory release (C++11 idiom)
```

See [src/tsl/ELEMENT_VECTOR_FEATURE.md](../../../tsl/ELEMENT_VECTOR_FEATURE.md) for complete API reference.

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

## Part 6: WebAssembly Specifics

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
├─ Lines 1-350: Standard Headers and Foundation
│  ├─ Includes (#include, pragmas)
│  ├─ State struct (cube representation)
│  ├─ Move definitions (U, R, F, etc.)
│  ├─ Utility functions (AlgToString, StringToAlg, etc.)
│  ├─ Index encoding/decoding (array_to_index, index_to_array)
│  └─ Move table creation (create_edge_move_table, create_corner_move_table, create_multi_move_table)
│  
│  **Note**: These are foundational components common across all solvers in this repository.
│  Can be summarized as "standard cube solver infrastructure".
│
├─ Lines 350-2900: **Sparse Database Construction System** (NEW IMPLEMENTATION)
│  ├─ Lines 350-550: SlidingDepthSets class
│  │  ├─ Rehash protection (will_rehash_on_next_insert)
│  │  ├─ Element vector attachment (zero-copy recording)
│  │  ├─ Memory-efficient set rotation (prev/cur/next)
│  │  └─ Capacity tracking and expansion control
│  │
│  ├─ Lines 550-800: ExpansionParams and ExpansionParamsDatabase
│  │  ├─ ExpansionParams struct (load factors, memory factors)
│  │  ├─ Global parameter database (g_expansion_params)
│  │  └─ Empirical parameter lookup by (depth, bucket_size)
│  │
│  ├─ Lines 800-890: LocalExpansionConfig struct
│  │  ├─ Bucket sizes (n, n+1, n+2)
│  │  ├─ Expected node counts
│  │  └─ Expansion flags (enable_n1, enable_n2)
│  │
│  ├─ Lines 890-1080: determine_bucket_sizes()
│  │  ├─ Memory budget calculation
│  │  ├─ Candidate bucket ratio evaluation (1:2, 1:1)
│  │  ├─ RSS prediction using empirical factors
│  │  └─ Best configuration selection
│  │
│  ├─ Lines 1080-1280: local_expand_step2_random_n1()
│  │  ├─ Random parent selection for diversity
│  │  ├─ Rehash-protected expansion
│  │  ├─ Element vector auto-recording
│  │  └─ High load factor achievement (~90%)
│  │
│  ├─ Lines 1280-1550: Utility functions
│  │  ├─ get_n1_capacity() - Calculate expansion capacity
│  │  ├─ Helper functions for sampling and statistics
│  │  └─ Data structure manipulation
│  │
│  ├─ Lines 1550-1900: expand_with_backtrace_filter()
│  │  ├─ 1/12 parent sampling
│  │  ├─ Child node generation
│  │  ├─ Backtrace verification (depth validation)
│  │  ├─ Rejection tracking (false n+2 nodes)
│  │  └─ Result statistics
│  │
│  └─ Lines 1900-2900: local_bfs_n_to_n2()
│     ├─ Step 0: Bucket size determination
│     ├─ Step 1: Capacity calculation
│     ├─ Step 2: Random n+1 expansion
│     ├─ Step 3: Data reorganization
│     ├─ Step 4: Parent sampling for n+2
│     ├─ Step 5: Backtrace expansion for n+2
│     └─ Step 6: Cleanup and statistics
│
├─ Lines 2900-3200: Main BFS Loop and Database Build
│  ├─ Phase 1: Full BFS (depth 0-6)
│  ├─ Phase 2: Local expansion coordinator
│  ├─ Depth 7: Initial local expansion (full)
│  ├─ Depth 8: local_bfs_n_to_n2() call
│  └─ Depth 9: local_bfs_n_to_n2() call (if possible)
│
├─ Lines 3200-3500: Scramble Generation (IDA* based)
│  ├─ get_xxcross_scramble() - Public API
│  ├─ Random state selection from database
│  └─ IDA* search to generate path
│
└─ Lines 3500-3631: Main Function and CLI
   ├─ Memory limit parsing (argv[1])
   ├─ Database construction call
   ├─ Interactive solving loop (optional)
   └─ WASM bindings (#ifdef __EMSCRIPTEN__)
```

### Key Functions by Purpose

**Database Construction**:
- `determine_bucket_sizes()` - Dynamic bucket allocation logic
- `local_expand_step2_random_n1()` - Random parent expansion with rehash protection
- `expand_with_backtrace_filter()` - Backtrace-verified expansion for depth n+2
- `local_bfs_n_to_n2()` - Complete local expansion coordinator

**Standard Infrastructure** (used across all solvers):
- `create_edge_move_table()` / `create_corner_move_table()` - Move pre-computation
- `create_multi_move_table()` - Multi-piece move table generation
- `array_to_index()` / `index_to_array()` - State encoding/decoding

**Memory Monitoring**:
- `get_rss_kb()` - Read RSS from /proc/self/status (Linux only)

**Public API**:
- `get_xxcross_scramble()` - Generate random scramble
- `start_search()` - Solve from given scramble (if implemented)

---

### Key Classes and Structs

**SlidingDepthSets** (Lines 350-550):
- **Purpose**: BFS engine with sliding window and rehash protection
- **Key Methods**:
  - `encounter_and_mark_next()` - Insert with deduplication
  - `will_rehash_on_next_insert()` - Predict rehash
  - `attach_element_vector()` - Zero-copy recording
  - `advance_depth()` - Memory-efficient set rotation

**LocalExpansionConfig** (Lines 800-890):
- **Purpose**: Configuration container for local expansion
- **Fields**: bucket sizes, expected nodes, expansion flags
- **Usage**: Returned by `determine_bucket_sizes()`

**ExpansionParams** (Lines 550-650):
- **Purpose**: Store empirical parameters for (depth, bucket_size)
- **Fields**:
  - `effective_load_factor` - Random expansion load
  - `backtrace_load_factor` - Backtrace expansion load
  - `measured_memory_factor` - RSS / theoretical memory ratio
- **Usage**: Retrieved from `g_expansion_params` global database

**ExpansionParamsDatabase** (Lines 650-800):
- **Purpose**: Global parameter lookup table
- **Usage**: `auto params = g_expansion_params.get(depth, bucket_size);`
- **Contents**: Empirically measured parameters from 47-point campaign

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
