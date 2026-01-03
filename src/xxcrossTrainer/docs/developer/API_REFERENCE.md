# XXCross Solver API Reference

**Version**: stable_20260103  
**Last Updated**: 2026-01-03  
**Source**: solver_dev.cpp (4723 lines)

> **Navigation**: [← Back to Implementation](SOLVER_IMPLEMENTATION.md) | [Development Summary](DEVELOPMENT_SUMMARY.md)

---

## Table of Contents

1. [Core Data Structures](#core-data-structures)
2. [State Operations](#state-operations)
3. [Move String Conversion](#move-string-conversion)
4. [Encoding Functions](#encoding-functions)
5. [Move Table Generation](#move-table-generation)
6. [Database Construction](#database-construction)
7. [Local Expansion Functions](#local-expansion-functions)
8. [Prune Table Generation](#prune-table-generation)
9. [Search Class (xxcross_search)](#search-class-xxcross_search)
10. [WASM Integration](#wasm-integration)

---

## Core Data Structures

### State

```cpp
struct State {
    std::vector<int> cp;  // Corner permutation (8 elements)
    std::vector<int> co;  // Corner orientation (8 elements, 0-2)
    std::vector<int> ep;  // Edge permutation (12 elements)
    std::vector<int> eo;  // Edge orientation (12 elements, 0-1)
    
    // Constructor
    State(std::vector<int> arg_cp = {0,1,2,3,4,5,6,7}, 
          std::vector<int> arg_co = {0,0,0,0,0,0,0,0},
          std::vector<int> arg_ep = {0,1,2,3,4,5,6,7,8,9,10,11},
          std::vector<int> arg_eo = {0,0,0,0,0,0,0,0,0,0,0,0});
    
    // Methods
    State apply_move(State move);
    State apply_move_edge(State move, int e);
    State apply_move_corner(State move, int c);
};
```

**Purpose**: Represents Rubik's cube state (solved state by default)

**Usage**:
```cpp
State solved;  // Default constructor
State scrambled = solved.apply_move(moves["R"]);
```

---

### SlidingDepthSets

```cpp
struct SlidingDepthSets {
    tsl::robin_set<uint64_t> prev, cur, next;
    size_t max_total_nodes;
    int current_depth;
    bool expansion_stopped;
    bool verbose;
    
    static const std::vector<size_t> expected_nodes_per_depth;
    static constexpr size_t BYTES_PER_ELEMENT = 32;
    
    SlidingDepthSets(size_t max_nodes = SIZE_MAX, bool enable_verbose = true);
    bool encounter_and_mark_next(uint64_t idx, bool &capacity_reached);
    void set_initial(uint64_t idx);
    void advance_depth(std::vector<std::vector<uint64_t>> &index_pairs);
    void clear_all();
    size_t total_size() const;
    size_t mem_bytes() const;
    static size_t next_power_of_2(size_t n);
};
```

**Purpose**: Efficient BFS sliding window for depth-by-depth expansion

**Key Features**:
- Automatic memory management with `attach_element_vector`
- Rehash prediction to prevent memory spikes
- Memory budget enforcement

---

### LocalExpansionConfig

```cpp
struct LocalExpansionConfig {
    bool enable_n1_expansion = false;
    bool enable_n2_expansion = false;
    size_t bucket_n1 = 0;
    size_t bucket_n2 = 0;
    size_t nodes_n1 = 0;
    size_t nodes_n2 = 0;
    size_t bucket_n = 0;
};
```

**Purpose**: Configuration for local expansion (depths 7-10)

---

### BacktraceExpansionResult

```cpp
struct BacktraceExpansionResult {
    size_t total_nodes;
    size_t unique_nodes;
    size_t rejected_duplicates;
    double rejection_rate;
};
```

**Purpose**: Statistics from random sampling expansion

---

## State Operations

### Global Move Definitions

```cpp
extern std::unordered_map<std::string, State> moves;
extern std::vector<std::string> move_names;
```

**move_names**: `{"U", "U2", "U'", "D", "D2", "D'", "L", "L2", "L'", "R", "R2", "R'", "F", "F2", "F'", "B", "B2", "B'"}`  
**moves**: Map of move names to State transformations

---

## Move String Conversion

### AlgToString

```cpp
std::string AlgToString(std::vector<int> &alg);
```

**Purpose**: Convert move sequence to string  
**Parameters**:
- `alg`: Vector of move indices (0-17)

**Returns**: String like `"R U R' U' "`

**Example**:
```cpp
std::vector<int> moves = {6, 0, 8, 2};  // R U R' U'
std::string str = AlgToString(moves);   // "R U R' U' "
```

**Move Encoding**:
- 0-2: U, U2, U'
- 3-5: D, D2, D'
- 6-8: L, L2, L'
- 9-11: R, R2, R'
- 12-14: F, F2, F'
- 15-17: B, B2, B'

---

### StringToAlg

```cpp
std::vector<int> StringToAlg(std::string str);
```

**Purpose**: Parse algorithm string to move indices  
**Parameters**:
- `str`: Algorithm string (e.g., `"R U R' U'"`)

**Returns**: Vector of move indices

**Example**:
```cpp
std::string alg = "R U R' U'";
std::vector<int> moves = StringToAlg(alg);  // {6, 0, 8, 2}
```

---

### AlgConvertRotation

```cpp
std::vector<int> AlgConvertRotation(std::vector<int> alg, std::string rotation);
```

**Purpose**: Apply cube rotation to algorithm  
**Parameters**:
- `alg`: Move sequence
- `rotation`: Rotation string (`"x"`, `"y"`, `"z"`, `"x2"`, `"y2"`, `"z2"`, `"x'"`, `"y'"`, `"z'"`)

**Returns**: Rotated move sequence

---

### AlgRotation

```cpp
std::vector<int> AlgRotation(std::vector<int> alg, std::string rotation_algString);
```

**Purpose**: Apply multiple rotations to algorithm  
**Parameters**:
- `alg`: Move sequence
- `rotation_algString`: Space-separated rotations (e.g., `"x y2"`)

**Returns**: Rotated move sequence

---

## Encoding Functions

### array_to_index

```cpp
inline int array_to_index(std::vector<int> &a, int n, int c, int pn);
```

**Purpose**: Encode permutation + orientation to integer index  
**Parameters**:
- `a`: Combined array (permutation * orientation_modulus + orientation)
- `n`: Number of pieces
- `c`: Orientation modulus (2 for edges, 3 for corners)
- `pn`: Total pieces in the cube (8 for corners, 12 for edges)

**Returns**: Unique integer index

**Algorithm**: Lehmer code for permutation + base-c for orientation

**⚠️ IMPORTANT**: This function **modifies** the input array `a`

**Example**:
```cpp
// Encode 4 cross edges (positions 8,9,10,11 with orientations 0,1,0,1)
std::vector<int> a = {16, 18, 20, 22};  // = {8*2+0, 9*2+0, 10*2+0, 11*2+0}
int index = array_to_index(a, 4, 2, 12);  // a is now modified!
```

---

### index_to_array

```cpp
inline void index_to_array(std::vector<int> &p, int index, int n, int c, int pn);
```

**Purpose**: Decode integer index to permutation + orientation  
**Parameters**:
- `p`: Output array (size `n`, pre-allocated)
- `index`: Integer index
- `n`: Number of pieces
- `c`: Orientation modulus
- `pn`: Total pieces in the cube

**Example**:
```cpp
std::vector<int> p(4);
index_to_array(p, 0, 4, 2, 12);  // Decode index 0
// p now contains {16, 18, 20, 22} (solved cross edges)
```

---

## Move Table Generation

### create_edge_move_table

```cpp
std::vector<int> create_edge_move_table();
```

**Purpose**: Generate single-edge move table (24 × 18)  
**Returns**: Move table for all edge positions/orientations

**Size**: 432 integers (24 edge states × 18 moves)

---

### create_corner_move_table

```cpp
std::vector<int> create_corner_move_table();
```

**Purpose**: Generate single-corner move table (24 × 18)  
**Returns**: Move table for all corner positions/orientations

**Size**: 432 integers (24 corner states × 18 moves)

---

### create_multi_move_table

```cpp
void create_multi_move_table(
    int n, int c, int pn, int size,
    const std::vector<int> &table,
    std::vector<int> &move_table);
```

**Purpose**: Generate multi-piece move table from single-piece table  
**Parameters**:
- `n`: Number of pieces (e.g., 4 for cross edges)
- `c`: Orientation modulus (2 or 3)
- `pn`: Total pieces (8 or 12)
- `size`: Output table size (factorial(pn, n) × c^n)
- `table`: Single-piece move table (from `create_edge_move_table` or `create_corner_move_table`)
- `move_table`: Output move table (size × 18)

**Example**:
```cpp
std::vector<int> edge_table = create_edge_move_table();
std::vector<int> cross_moves;
create_multi_move_table(4, 2, 12, 24*22*20*18, edge_table, cross_moves);
// cross_moves now contains move table for 4 cross edges
```

---

## Database Construction

### create_prune_table_sparse

```cpp
int create_prune_table_sparse(
    int index1, int index2, int index3,
    int size1, int size2, int size3,
    int max_depth, int max_memory_kb,
    const std::vector<int> &table1,
    const std::vector<int> &table2,
    const std::vector<int> &table3,
    std::vector<std::vector<uint64_t>> &index_pairs,
    std::vector<int> &num_list,
    bool verbose = true,
    const ResearchConfig& research_config = ResearchConfig());
```

**Purpose**: Main BFS database construction (depths 0-6 full expansion)  
**Parameters**:
- `index1`, `index2`, `index3`: Goal state indices
- `size1`, `size2`, `size3`: Coordinate space sizes
- `max_depth`: Maximum depth (typically 10)
- `max_memory_kb`: Memory budget in KB
- `table1`, `table2`, `table3`: Move tables for 3 coordinates
- `index_pairs`: Output - nodes per depth (vector of vectors)
- `num_list`: Output - node counts per depth
- `verbose`: Enable logging
- `research_config`: Research options (memory limits, verbose, etc.)

**Returns**: Reached depth (may be less than `max_depth` if memory limited)

**Key Features**:
- Uses `SlidingDepthSets` for memory-efficient BFS
- Automatic `attach_element_vector` to record nodes
- Memory budget enforcement
- Rehash prediction to prevent spikes

---

### run_bfs_phase

```cpp
int run_bfs_phase(
    int index1, int index2, int index3,
    int size1, int size2, int size3,
    int bfs_depth, int max_memory_kb,
    const std::vector<int> &table1,
    const std::vector<int> &table2,
    const std::vector<int> &table3,
    std::vector<std::vector<uint64_t>> &index_pairs,
    std::vector<int> &num_list,
    bool verbose,
    const ResearchConfig& research_config);
```

**Purpose**: Wrapper for `create_prune_table_sparse` (Phase 1)  
**Returns**: Reached depth

---

### build_complete_search_database

```cpp
void build_complete_search_database(
    int index1, int index2, int index3,
    int size1, int size2, int size3,
    int bfs_depth, int memory_limit_mb,
    const std::vector<int> &table1,
    const std::vector<int> &table2,
    const std::vector<int> &table3,
    std::vector<std::vector<uint64_t>> &index_pairs,
    std::vector<int> &num_list,
    bool verbose,
    const BucketConfig& bucket_config,
    const ResearchConfig& research_config);
```

**Purpose**: Complete database construction (Phases 1-5)  
**Workflow**:
1. **Phase 1**: Full BFS (depths 0-6) via `run_bfs_phase`
2. **Phase 2**: Local expansion (depth 6→7) if enabled
3. **Phase 3**: Local expansion (depth 7→8) if enabled
4. **Phase 4**: Local expansion (depth 8→9) if enabled
5. **Malloc Trim**: Clear allocator cache (native only, configurable)
6. **Phase 5**: Random sampling (depth 9→10) if enabled

**Key Features**:
- Unified interface for all construction phases
- Automatic bucket model selection or custom configuration
- Platform-specific optimization (WASM vs Native)

---

## Local Expansion Functions

### determine_bucket_sizes

```cpp
LocalExpansionConfig determine_bucket_sizes(
    int max_depth,
    size_t remaining_memory_bytes,
    size_t bucket_n_current,
    size_t cur_set_memory_bytes,
    bool verbose = true);
```

**Purpose**: Calculate optimal bucket sizes for local expansion (depths 7-9)  
**Returns**: `LocalExpansionConfig` with bucket allocations

**Strategy**: 1:1 or 1:2 balance, prefer n+1 ≤ n+2

---

### run_local_expansion_steps_1_to_4

```cpp
LocalExpansionResults run_local_expansion_steps_1_to_4(
    int depth_n,
    const std::vector<uint64_t>& nodes_n,
    const std::vector<int>& table1,
    const std::vector<int>& table2,
    const std::vector<int>& table3,
    size_t size2, size_t size3,
    const LocalExpansionConfig& config,
    std::vector<std::vector<uint64_t>>& index_pairs,
    std::vector<int>& num_list,
    bool verbose);
```

**Purpose**: Execute local expansion steps 1-4 (depth n → n+1 → n+2)  
**Returns**: `LocalExpansionResults` with statistics

**Steps**:
1. Random sampling (depth n → n+1)
2. Reorganization (depth n+1 nodes)
3. Calculate capacity (depth n+1 → n+2)
4. Backtrace expansion (depth n+1 → n+2)

---

### expand_with_backtrace_filter

```cpp
BacktraceExpansionResult expand_with_backtrace_filter(
    const std::vector<uint64_t>& parent_nodes,
    size_t target_capacity,
    const std::vector<int>& table1,
    const std::vector<int>& table2,
    const std::vector<int>& table3,
    size_t size2, size_t size3,
    std::vector<uint64_t>& output_nodes,
    bool verbose);
```

**Purpose**: Random sampling with duplicate filtering  
**Returns**: `BacktraceExpansionResult` with rejection statistics

**Algorithm**:
- Select random parent from `parent_nodes`
- Apply random move (0-17)
- Check if child exists in parent set (backtrace filter)
- Insert if unique

---

## Prune Table Generation

### create_prune_table

```cpp
void create_prune_table(
    int index, int size, int depth,
    const std::vector<int> table,
    std::vector<unsigned char> &prune_table);
```

**Purpose**: Generate single-coordinate prune table (BFS to depth d)  
**Parameters**:
- `index`: Goal state index
- `size`: Coordinate space size
- `depth`: Maximum depth
- `table`: Move table
- `prune_table`: Output (size × 1, values 0-255)

**Usage**: Used for cross edge pruning (coordinate 1)

---

### create_prune_table2

```cpp
void create_prune_table2(
    int index1, int index2,
    int size1, int size2,
    int depth,
    const std::vector<int> table1,
    const std::vector<int> table2,
    std::vector<unsigned char> &prune_table);
```

**Purpose**: Generate two-coordinate prune table (BFS to depth d)  
**Parameters**:
- `index1`, `index2`: Goal state indices
- `size1`, `size2`: Coordinate space sizes
- `depth`: Maximum depth
- `table1`, `table2`: Move tables
- `prune_table`: Output (size1 × size2, values 0-255)

**Usage**: Used for F2L slot pruning (coordinates 2+3)

---

## Search Class (xxcross_search)

### Constructor

```cpp
xxcross_search(
    bool adj = true,
    int BFS_DEPTH = 6,
    int MEMORY_LIMIT_MB = 1600,
    bool verbose = true,
    const BucketConfig& bucket_config = BucketConfig(),
    const ResearchConfig& research_config = ResearchConfig());
```

**Purpose**: Construct solver and build database  
**Parameters**:
- `adj`: Adjacent (true) or opposite (false) F2L slots
- `BFS_DEPTH`: Full BFS depth (typically 6)
- `MEMORY_LIMIT_MB`: Memory budget
- `verbose`: Enable logging
- `bucket_config`: Bucket model configuration
- `research_config`: Research options

**Example**:
```cpp
BucketConfig config;
config.model = BucketModel::BALANCED;

xxcross_search solver(true, 6, 1600, true, config);
```

---

### get_xxcross_scramble

```cpp
std::string get_xxcross_scramble(std::string arg_length = "7");
```

**Purpose**: Generate random scramble at specific depth with **guaranteed exact depth**  
**Parameters**:
- `arg_length`: Target depth (1-10 as string)

**Returns**: Scramble algorithm string (guaranteed to have optimal solution at requested depth)

**Example**:
```cpp
std::string scramble = solver.get_xxcross_scramble("8");
// Returns scramble like "R U R' U' F D F' D' " (guaranteed 8-move optimal solution)
```

**Algorithm**: 
1. Random selection from `index_pairs[depth]`
2. Verify actual optimal depth using IDA* search (depths 1→target)
3. Retry with different node if actual depth < target depth
4. Maximum 100 attempts before fallback

**Depth Guarantee (2026-01-04)**:

**Critical Feature**: `index_pairs[len]` contains nodes **discovered** at depth `len`, but may have shorter optimal solutions due to sparse coverage (especially depths 7+).

**Solution**: Retry loop with depth verification:
```cpp
for (int attempt = 0; attempt < 100; ++attempt) {
    uint64_t node = index_pairs[len][random_index()];
    
    // Verify actual optimal depth
    for (int d = 1; d <= len; ++d) {
        if (depth_limited_search(node, d)) {
            actual_depth = d;
            break;
        }
    }
    
    if (actual_depth == len) {
        return solution;  // Success!
    }
    // Retry with different node
}
```

**Why Necessary**:
- **Sparse Coverage**: Depth 9 ≈ 5B nodes, we store ~8M (0.16% coverage)
- **Depth 10**: ~50B nodes, we store ~4M (0.008% coverage)
- **High Collision Probability**: Random node likely has depth < target optimal solution
- **Training Quality**: Users need exact depth for effective practice

**Performance** (Tested 2026-01-03):
- Depth 1-6: 1 attempt (full BFS guarantees depth)
- Depth 7-8: ~2-10 attempts average
- Depth 9-10: ~5-20 attempts average
- **Depth 10 Test**: 9 attempts, <2ms total (0.008% coverage, minimal bucket 1M/1M/2M/4M)

**See**: [IMPLEMENTATION_PROGRESS.md Phase 7.7](IMPLEMENTATION_PROGRESS.md#77-scramble-generation-depth-accuracy--completed-2026-01-03-2330) for detailed analysis

---

### get_scramble_length

```cpp
int get_scramble_length(std::string scramble_str);
```

**Purpose**: Calculate number of moves in scramble algorithm  
**Parameters**:
- `scramble_str`: Scramble algorithm string (space-separated moves)

**Returns**: Number of moves (integer)

**Example**:
```cpp
int moves = solver.get_scramble_length("R U R' F2 D L' B2");
// Returns: 7
```

**Note**: 
- Uses same parser as solver (`StringToAlg()`)
- Avoids JavaScript/C++ vector conversion issues in WASM
- Recommended for WASM environments over JavaScript string splitting

**Use Case (WASM)**:
```javascript
// Worker JavaScript
const scramble = solver.func("", "8").split(',')[1];
const moveCount = solver.get_scramble_length(scramble);  // Reliable
// vs. scramble.split(/\s+/).length  // May have parsing issues
```

---

### start_search

```cpp
std::string start_search(std::string arg_scramble = "");
```

**Purpose**: Solve scramble using IDA* search  
**Parameters**:
- `arg_scramble`: Scramble algorithm string

**Returns**: Solution algorithm string

**Example**:
```cpp
std::string solution = solver.start_search("R U R' U'");
// Returns solution like "U R U' R' "
```

**Algorithm**: IDA* with prune table pruning

---

### depth_limited_search

```cpp
bool depth_limited_search(
    int arg_index1, int arg_index2, int arg_index3,
    int depth, int prev);
```

**Purpose**: Recursive depth-limited search (IDA* helper)  
**Parameters**:
- `arg_index1`, `arg_index2`, `arg_index3`: Current coordinate indices (× 18)
- `depth`: Remaining search depth
- `prev`: Previous move (for pruning)

**Returns**: `true` if solution found

**Internal use**: Called by `start_search` and `get_xxcross_scramble`

---

### func

```cpp
std::string func(
    std::string arg_scramble = "",
    std::string arg_length = "7");
```

**Purpose**: Combined solve + scramble generation (legacy WASM interface)  
**Parameters**:
- `arg_scramble`: Scramble to solve
- `arg_length`: Depth for new scramble

**Returns**: `"<scramble>,<solution>,<new_scramble>"`

**Deprecated**: Use `start_search` and `get_xxcross_scramble` separately

---

## WASM Integration

### SolverStatistics

```cpp
struct SolverStatistics {
    std::vector<int> node_counts;                 // Per-depth (0-10)
    std::vector<double> load_factors;             // Depths 7-10
    double avg_children_per_parent;
    int max_children_per_parent;
    size_t final_heap_mb;
    size_t peak_heap_mb;
    std::vector<std::string> sample_scrambles;    // Per-depth (1-10)
    std::vector<int> scramble_lengths;            // Per-depth (1-10)
    bool success;
    std::string error_message;
};
```

**Purpose**: Statistics from WASM database construction

**WASM Access**:
```javascript
const stats = Module.get_solver_statistics();
console.log(`Depth 10 nodes: ${stats.node_counts[10]}`);
console.log(`Peak heap: ${stats.peak_heap_mb} MB`);
console.log(`Sample scramble: ${stats.sample_scrambles[8]}`);
```

---

### solve_with_custom_buckets

```cpp
SolverStatistics solve_with_custom_buckets(
    int bucket_7_mb, int bucket_8_mb,
    int bucket_9_mb, int bucket_10_mb,
    bool verbose = true);
```

**Purpose**: WASM entry point for custom bucket database construction  
**Parameters**:
- `bucket_7_mb`, `bucket_8_mb`, `bucket_9_mb`, `bucket_10_mb`: Bucket sizes in MB
- `verbose`: Enable logging

**Returns**: `SolverStatistics` structure

**Example (JavaScript)**:
```javascript
const stats = Module.solve_with_custom_buckets(8, 8, 8, 8, true);
if (stats.success) {
    console.log(`Built database with ${stats.node_counts.reduce((a,b) => a+b, 0)} nodes`);
    console.log(`Peak heap: ${stats.peak_heap_mb} MB`);
} else {
    console.error(`Error: ${stats.error_message}`);
}
```

**Key Features**:
- Automatic heap measurement (`emscripten_get_heap_size`)
- Exception handling with error reporting
- Sample scramble generation per depth
- Load factor tracking

---

### get_solver_statistics

```cpp
SolverStatistics get_solver_statistics();
```

**Purpose**: Retrieve statistics from last `solve_with_custom_buckets` call  
**Returns**: `SolverStatistics` structure

**Example (JavaScript)**:
```javascript
const stats = Module.get_solver_statistics();
console.log(`Depth 9 load factor: ${stats.load_factors[2]}`);
```

---

### Emscripten Bindings

```cpp
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::class_<xxcross_search>("xxcross_search")
        .constructor<>()
        .constructor<bool>()
        .constructor<bool, int>()
        .constructor<bool, int, int>()
        .constructor<bool, int, int, bool>()
        .function("func", &xxcross_search::func);
    
    emscripten::class_<SolverStatistics>("SolverStatistics")
        .constructor<>()
        .property("node_counts", &SolverStatistics::node_counts)
        .property("load_factors", &SolverStatistics::load_factors)
        .property("avg_children_per_parent", &SolverStatistics::avg_children_per_parent)
        .property("max_children_per_parent", &SolverStatistics::max_children_per_parent)
        .property("final_heap_mb", &SolverStatistics::final_heap_mb)
        .property("peak_heap_mb", &SolverStatistics::peak_heap_mb)
        .property("sample_scrambles", &SolverStatistics::sample_scrambles)
        .property("scramble_lengths", &SolverStatistics::scramble_lengths)
        .property("success", &SolverStatistics::success)
        .property("error_message", &SolverStatistics::error_message);
    
    emscripten::function("solve_with_custom_buckets", &solve_with_custom_buckets);
    emscripten::function("get_solver_statistics", &get_solver_statistics);
    
    emscripten::register_vector<int>("VectorInt");
    emscripten::register_vector<double>("VectorDouble");
    emscripten::register_vector<std::string>("VectorString");
}
```

**Purpose**: JavaScript bindings for WASM module

---

## Utility Functions

### get_rss_kb

```cpp
size_t get_rss_kb();
```

**Purpose**: Get current RSS memory usage (Linux only, native)  
**Returns**: RSS in KB (0 on WASM)

**Usage**:
```cpp
size_t rss_before = get_rss_kb();
// ... database construction ...
size_t rss_after = get_rss_kb();
std::cout << "RSS delta: " << ((rss_after - rss_before) / 1024.0) << " MB" << std::endl;
```

---

### log_emscripten_heap

```cpp
inline void log_emscripten_heap(const char* phase_name);
```

**Purpose**: Log WASM heap size (WASM only, no-op on native)  
**Parameters**:
- `phase_name`: Label for log message

**Usage**:
```cpp
log_emscripten_heap("After Phase 1");
// [Heap] After Phase 1: Total=512 MB
```

---

### calculate_memory

```cpp
size_t calculate_memory(size_t bucket_size, size_t node_count);
```

**Purpose**: Calculate total memory from bucket size and node count  
**Parameters**:
- `bucket_size`: Number of hash table buckets
- `node_count`: Number of nodes

**Returns**: Memory in bytes (buckets × 4 + nodes × 32)

---

### calculate_effective_nodes

```cpp
size_t calculate_effective_nodes(int max_depth, size_t bucket_size);
```

**Purpose**: Calculate effective node capacity from bucket size  
**Parameters**:
- `max_depth`: Target depth
- `bucket_size`: Number of buckets

**Returns**: Node capacity (bucket_size × load_factor)

**Note**: Load factor varies by depth (0.88-0.95), see `expansion_parameters.h`

---

## Performance Notes

**Encoding Functions**:
- `array_to_index`: O(n²) - use pre-built move tables for performance
- `index_to_array`: O(n log n) - sorting overhead

**Move Operations**:
- `StringToAlg` / `AlgToString`: O(n) - simple iteration
- `AlgConvertRotation`: O(n) - in-place transformation

**BFS Construction**:
- **Phase 1** (depths 0-6): ~220 MB peak, 2-5 seconds
- **Phase 2-4** (depths 7-9): Variable (depends on bucket sizes)
- **Phase 5** (depth 10): Random sampling, ~2-5 seconds

**IDA* Search**:
- **Depth 8**: <0.1 seconds (average)
- **Depth 10**: 1-5 seconds (depends on database quality)

---

## See Also

- [SOLVER_IMPLEMENTATION.md](SOLVER_IMPLEMENTATION.md) - Implementation details
- [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md) - WASM usage guide
- [bucket_config.h](../../bucket_config.h) - Bucket model definitions

---

**Document Version**: 2.0  
**Corresponds to**: solver_dev.cpp stable_20260103 (4723 lines)  
**All functions verified**: 2026-01-03
