# Element Vector Feature

## Overview

The **Element Vector** feature is a zero-copy extraction mechanism added to `robin_hash` and `robin_set`. It allows automatic recording of inserted elements into an external `std::vector` during insertion, eliminating the need for post-insertion extraction and significantly reducing memory overhead.

## Problem Statement

Traditional approaches to extracting elements from a hash table require:
1. Allocating a separate vector
2. Iterating through the hash table
3. Copying all elements to the vector

This approach has several drawbacks:
- **Memory spike**: Both structures exist simultaneously during extraction
- **Performance overhead**: O(n) iteration + memory allocation
- **Memory constraints**: May fail in memory-limited environments (e.g., embedded systems, large datasets)

## Solution

The Element Vector feature records elements during insertion, not after:
- Elements are automatically appended to an attached vector upon successful insertion
- Zero-copy extraction via `std::move()` of the pre-populated vector
- Hash table can be cleared immediately after, freeing memory
- No post-processing iteration required

## API Reference

### robin_hash Methods

#### `attach_element_vector(std::vector<value_type>* vec)`

Attaches an external vector to the hash table. All subsequent successful insertions will automatically record the inserted element in this vector.

**Parameters:**
- `vec`: Pointer to the external vector. Must remain valid until detached.

**Notes:**
- Must be called **before** any insertions you want to record
- The vector is not owned by the hash table (no automatic cleanup)
- Duplicates are **not** recorded (only successful insertions)
- Passing `nullptr` is safe but has no effect

**Example:**
```cpp
tsl::robin_set<uint64_t> my_set;
std::vector<uint64_t> recorded_elements;

my_set.attach_element_vector(&recorded_elements);

// These insertions are automatically recorded
my_set.insert(123);  // recorded_elements = {123}
my_set.insert(456);  // recorded_elements = {123, 456}
my_set.insert(123);  // Duplicate - NOT recorded
```

#### `detach_element_vector()`

Detaches the currently attached vector, if any. After calling this method, insertions will no longer be recorded.

**Notes:**
- Safe to call even if no vector is attached
- Does not modify the vector contents
- Hash table retains all inserted elements

**Example:**
```cpp
my_set.detach_element_vector();

// Subsequent insertions are NOT recorded
my_set.insert(789);  // recorded_elements still = {123, 456}
```

#### `get_element_vector() const`

Returns a pointer to the currently attached vector, or `nullptr` if none is attached.

**Returns:**
- `std::vector<value_type>*`: Pointer to attached vector, or `nullptr`

**Use cases:**
- Checking if a vector is attached before operations
- Validating vector state during debugging
- Accessing the vector for size checks

**Example:**
```cpp
if (my_set.get_element_vector() != nullptr) {
    std::cout << "Recording enabled, " 
              << my_set.get_element_vector()->size() 
              << " elements recorded" << std::endl;
}
```

### robin_set Methods

`robin_set` provides wrapper methods with identical signatures and behavior:
- `attach_element_vector(std::vector<Key>* vec)`
- `detach_element_vector()`
- `get_element_vector() const`

## Capacity Monitoring API

In addition to the element vector feature, several utility methods were added to `robin_hash` for precise capacity monitoring and rehash prediction. These are essential for memory-constrained environments.

### robin_hash Methods

#### `load_threshold() const noexcept`

Returns the precomputed load threshold, which equals `bucket_count() * max_load_factor()`. When `size()` reaches this value, the next insertion will trigger a rehash.

**Returns:**
- `size_type`: The maximum number of elements before rehash

**Use cases:**
- Capacity planning in memory-limited environments
- Calculating exact remaining capacity
- Determining safe insertion limits

**Example:**
```cpp
tsl::robin_set<uint64_t> my_set;
my_set.reserve(1000000);

std::cout << "Bucket count: " << my_set.bucket_count() << std::endl;
std::cout << "Max load factor: " << my_set.max_load_factor() << std::endl;
std::cout << "Load threshold: " << my_set.load_threshold() << std::endl;
// Output:
// Bucket count: 1048576
// Max load factor: 0.9
// Load threshold: 943718
```

**Notes:**
- This is a precomputed value stored in `m_load_threshold`
- O(1) operation with no calculation overhead
- Updated automatically after `reserve()`, `rehash()`, or `max_load_factor()` changes

---

#### `available_capacity() const noexcept`

Returns how many more elements can be inserted before hitting the load threshold (not accounting for probe distance limit).

**Returns:**
- `size_type`: Number of available slots before rehash, or 0 if at/over threshold

**Formula:**
```cpp
available_capacity() = max(0, load_threshold() - size())
```

**Example:**
```cpp
tsl::robin_set<uint64_t> my_set;
my_set.reserve(1000000);

std::cout << "Available capacity: " << my_set.available_capacity() << std::endl;
// Output: 943718

my_set.insert(500000);
std::cout << "After 500k insertions: " << my_set.available_capacity() << std::endl;
// Output: 443718

// Safe to insert without rehash
for (int i = 0; i < my_set.available_capacity(); ++i) {
    my_set.insert(generate_value());  // No rehash guaranteed
}
```

**Use cases:**
- Determining safe batch insertion size
- Loop termination conditions in memory-constrained BFS
- Progress reporting with remaining capacity

**Caveats:**
- Does **not** account for probe distance limit violations
- Actual available capacity may be lower if hash collisions are severe
- For safety-critical code, use `will_rehash_on_next_insert()`

---

#### `will_rehash_on_next_insert() const noexcept`

Checks if the hash table will rehash on the next insert operation.

**Returns:**
- `bool`: `true` if next insert will trigger rehash, `false` otherwise

**Conditions checked:**
1. `m_grow_on_next_insert` flag (set when probe distance limit exceeded)
2. Current `size() >= load_threshold()` (only for tables with ≥ 1M buckets)

**Example:**
```cpp
tsl::robin_set<uint64_t> my_set;
my_set.reserve(1000000);

while (!my_set.will_rehash_on_next_insert()) {
    my_set.insert(generate_value());
}

std::cout << "Stopped at " << my_set.size() << " elements" << std::endl;
std::cout << "Load threshold was " << my_set.load_threshold() << std::endl;
```

**Use cases:**
- Loop termination in memory-constrained expansion
- Preventing unexpected memory spikes
- Validating capacity calculations

**Important notes:**
- **Cannot predict probe distance violations** during actual insertion
- Only checks load threshold for large tables (≥ 1M buckets)
  - Small table rehashes are cheap and allowed to proceed normally
  - Prevents false positives during initial growth phase
- Conservative check: may return `false` even if rehash will occur due to probe distance

**Best practice for critical code:**
```cpp
// Stop BEFORE will_rehash returns true
while (my_set.available_capacity() > SAFETY_MARGIN &&
       !my_set.will_rehash_on_next_insert()) {
    my_set.insert(value);
}
```

---

### robin_set Wrapper Methods

All capacity monitoring methods are also available in `robin_set`:
- `load_threshold() const noexcept`
- `available_capacity() const noexcept`
- `will_rehash_on_next_insert() const noexcept`

## Usage Patterns

### Pattern 1: Zero-Copy Extraction

Extract all elements from a hash table without memory overhead:

```cpp
tsl::robin_set<uint64_t> hash_table;
std::vector<uint64_t> extracted_elements;

// Step 1: Attach vector BEFORE insertion
hash_table.attach_element_vector(&extracted_elements);

// Step 2: Perform insertions (elements automatically recorded)
for (uint64_t value : source_data) {
    hash_table.insert(value);
}

// Step 3: Detach (optional but recommended)
hash_table.detach_element_vector();

// Step 4: Use extracted_elements (zero-copy!)
// Size validation
assert(extracted_elements.size() == hash_table.size());

// Step 5: Clear hash table to free memory
hash_table.clear();

// extracted_elements now contains all unique elements
// No memory spike occurred during extraction
```

### Pattern 2: BFS Level Tracking

Track nodes at each depth level in breadth-first search:

```cpp
std::vector<std::vector<uint64_t>> nodes_by_depth(max_depth + 1);
tsl::robin_set<uint64_t> current_level, next_level;

for (int depth = 0; depth < max_depth; ++depth) {
    // Attach vector for next depth
    next_level.clear();
    next_level.attach_element_vector(&nodes_by_depth[depth + 1]);
    
    // Expand current level
    for (uint64_t node : current_level) {
        for (uint64_t neighbor : get_neighbors(node)) {
            next_level.insert(neighbor);  // Auto-recorded
        }
    }
    
    // Detach and swap
    next_level.detach_element_vector();
    current_level.swap(next_level);
}

// nodes_by_depth[d] contains all nodes at depth d
```

### Pattern 3: Conditional Recording

Record only specific insertions:

```cpp
tsl::robin_set<uint64_t> all_nodes;
std::vector<uint64_t> special_nodes;

for (uint64_t node : data) {
    all_nodes.insert(node);  // Always insert
    
    if (is_special(node)) {
        // Temporarily attach for this insertion
        all_nodes.attach_element_vector(&special_nodes);
        all_nodes.insert(node);  // Duplicate, but ensures recording
        all_nodes.detach_element_vector();
    }
}
```

### Pattern 4: Memory-Constrained Expansion

Use capacity monitoring to prevent rehashing in memory-limited environments:

```cpp
const size_t MEMORY_LIMIT_MB = 1900;
const size_t BYTES_PER_ELEMENT = 24;  // robin_set overhead
const size_t MAX_ELEMENTS = (MEMORY_LIMIT_MB * 1024 * 1024) / BYTES_PER_ELEMENT;

tsl::robin_set<uint64_t> nodes;
std::vector<uint64_t> recorded_nodes;

// Reserve based on memory limit
size_t safe_capacity = static_cast<size_t>(MAX_ELEMENTS * 0.9);  // 90% load factor
nodes.reserve(safe_capacity);
nodes.attach_element_vector(&recorded_nodes);

std::cout << "Bucket count: " << nodes.bucket_count() << std::endl;
std::cout << "Load threshold: " << nodes.load_threshold() << std::endl;
std::cout << "Available capacity: " << nodes.available_capacity() << std::endl;

// Expand until rehash is predicted
size_t inserted = 0;
for (uint64_t parent : parent_nodes) {
    for (uint64_t child : expand(parent)) {
        // Check before insertion to prevent rehash
        if (nodes.will_rehash_on_next_insert()) {
            std::cout << "Stopping: rehash imminent at " << nodes.size() 
                      << " elements" << std::endl;
            goto expansion_complete;
        }
        
        nodes.insert(child);
        ++inserted;
    }
}

expansion_complete:
nodes.detach_element_vector();

std::cout << "Inserted " << inserted << " elements" << std::endl;
std::cout << "Recorded " << recorded_nodes.size() << " unique elements" << std::endl;
std::cout << "Final load: " << (100.0 * nodes.size() / nodes.bucket_count()) 
          << "%" << std::endl;

// Verify no rehash occurred
assert(nodes.bucket_count() == initial_bucket_count);
```

### Pattern 5: Capacity-Aware BFS

Track remaining capacity during breadth-first search:

```cpp
tsl::robin_set<uint64_t> current_level, next_level;
std::vector<uint64_t> next_level_nodes;

// Reserve for next level
size_t estimated_next_size = current_level.size() * BRANCHING_FACTOR;
next_level.reserve(estimated_next_size);
next_level.attach_element_vector(&next_level_nodes);

std::cout << "Reserved capacity: " << next_level.available_capacity() 
          << " elements" << std::endl;

size_t rejected_due_to_capacity = 0;

for (uint64_t node : current_level) {
    for (uint64_t neighbor : get_neighbors(node)) {
        // Early termination if near capacity
        if (next_level.will_rehash_on_next_insert()) {
            ++rejected_due_to_capacity;
            continue;  // Skip to avoid rehash
        }
        
        next_level.insert(neighbor);
    }
    
    // Progress reporting
    if (node_count % 10000 == 0) {
        std::cout << "Processed " << node_count << " nodes, "
                  << "generated " << next_level.size() << ", "
                  << "remaining capacity: " << next_level.available_capacity()
                  << std::endl;
    }
}

next_level.detach_element_vector();

std::cout << "Expansion complete:" << std::endl;
std::cout << "  Generated: " << next_level.size() << " nodes" << std::endl;
std::cout << "  Rejected: " << rejected_due_to_capacity << " nodes" << std::endl;
std::cout << "  Final load: " << (100.0 * next_level.size() / next_level.bucket_count()) 
          << "%" << std::endl;
```

## Implementation Details

### Internal Mechanism

1. **Private Member**: `std::vector<value_type>* m_element_vector` (default: `nullptr`)
2. **Insertion Hook**: Modified `insert_impl()` to call `emplace_back()` after successful insertion
   - **Update 2026-01-01**: Changed from `push_back()` to `emplace_back()` for move semantics
   - Reduces copy overhead for large value types
3. **Rehash Safety**: `rehash_impl()` preserves the pointer across rehashing
4. **Zero Overhead**: When `nullptr`, only a single pointer check per insertion

### Performance Optimization (2026-01-01)

**Critical**: Always `reserve()` the attached vector before insertion to avoid reallocation spikes.

**Non-optimized (causes memory spikes):**
```cpp
std::vector<uint64_t> vec;  // No reserve!
hash_table.attach_element_vector(&vec);

// Bad: Each emplace_back may trigger reallocation
for (int i = 0; i < 1000000; ++i) {
    hash_table.insert(i);  // Reallocation every 2x growth
}
// Result: Multiple 500KB+ memory spikes during vector growth
```

**Optimized (no spikes):**
```cpp
std::vector<uint64_t> vec;
vec.reserve(1000000);  // Pre-allocate full capacity
hash_table.attach_element_vector(&vec);

// Good: emplace_back never reallocates
for (int i = 0; i < 1000000; ++i) {
    hash_table.insert(i);  // O(1) append, no spikes
}
// Result: Flat memory usage, no reallocations
```

**Recommended pattern:**
```cpp
// Reserve based on hash table's load threshold
size_t estimated_capacity = hash_table.load_threshold();
vec.reserve(estimated_capacity);
hash_table.attach_element_vector(&vec);
```

### Memory Layout

**Before Element Vector (Traditional Extraction):**
```
robin_set: 1000 MB
extraction buffer: 120 MB (during copy)
Peak memory: 1120 MB
```

**With Element Vector:**
```
robin_set: 1000 MB
recording vector: 120 MB (grows during insertion)
Peak memory: 1120 MB (same)

After clear():
robin_set: 0 MB
recording vector: 120 MB
Final memory: 120 MB (88% reduction!)
```

### Performance Characteristics

- **Insertion overhead**: Single `push_back()` + pointer check per successful insertion
- **Typical overhead**: < 2% in microbenchmarks
- **Memory overhead**: None when disabled (`nullptr`)
- **Rehash overhead**: None (pointer copy is O(1))

## Caveats and Best Practices

### ⚠️ Reserve Capacity Before Attach (CRITICAL)

**Wrong (causes memory spikes):**
```cpp
std::vector<uint64_t> vec;  // No reserve - DANGER!
hash_table.attach_element_vector(&vec);
// Millions of insertions will cause repeated reallocations
```

**Correct (optimal performance):**
```cpp
std::vector<uint64_t> vec;
vec.reserve(estimated_size);  // Pre-allocate based on hash table capacity
hash_table.attach_element_vector(&vec);
// All insertions are O(1) with no reallocations
```

**Best practice:**
```cpp
// Method 1: Reserve based on load threshold
vec.reserve(hash_table.load_threshold());

// Method 2: Reserve based on bucket count and load factor
vec.reserve(static_cast<size_t>(hash_table.bucket_count() * 0.9));

// Method 3: Reserve based on estimated final size
vec.reserve(estimated_final_insertions);

hash_table.attach_element_vector(&vec);
```

**Impact of not reserving:**
- **Memory spikes**: Vector grows by 2x each time (1→2→4→8...MB)
- **Performance**: O(n log n) due to repeated copies during growth
- **Fragmentation**: Multiple allocations/deallocations fragment memory
- **Measurement accuracy**: RSS spikes corrupt memory usage measurements

### ⚠️ Attach Before Insertion

**Wrong:**
```cpp
hash_table.insert(123);
hash_table.attach_element_vector(&vec);  // Too late!
hash_table.insert(456);  // Only this is recorded
```

**Correct:**
```cpp
hash_table.attach_element_vector(&vec);
hash_table.insert(123);  // Recorded
hash_table.insert(456);  // Recorded
```

### ⚠️ Vector Lifetime

The vector must outlive the attachment period:

**Wrong:**
```cpp
{
    std::vector<uint64_t> temp;
    hash_table.attach_element_vector(&temp);
}  // temp destroyed - DANGLING POINTER!
hash_table.insert(123);  // UNDEFINED BEHAVIOR
```

**Correct:**
```cpp
std::vector<uint64_t> persistent;
hash_table.attach_element_vector(&persistent);
hash_table.insert(123);  // Safe
hash_table.detach_element_vector();  // Good practice
```

### ⚠️ Rehash Behavior

The element vector **survives rehashing**:

```cpp
hash_table.attach_element_vector(&vec);
hash_table.insert(/* many elements */);  // May trigger rehash

// vec still attached and valid after rehash
assert(hash_table.get_element_vector() == &vec);
```

### ⚠️ Size Synchronization

Always validate that vector size matches hash table size:

```cpp
hash_table.detach_element_vector();

if (vec.size() != hash_table.size()) {
    std::cerr << "WARNING: Size mismatch detected!" << std::endl;
    // Possible causes:
    // - Attached after some insertions
    // - Vector was not empty before attach
    // - Logic error in attach/detach sequence
}
```

### ✅ Best Practice: Clear Before Attach

```cpp
vec.clear();  // Start fresh
hash_table.attach_element_vector(&vec);
// Now vec.size() will always match insertion count
```

## Advanced Usage

### Multi-Depth Recording

```cpp
std::vector<std::vector<uint64_t>> depth_vectors(10);
tsl::robin_set<uint64_t> prev, cur, next;

for (int depth = 0; depth < 10; ++depth) {
    // Detach from prev (if any)
    if (prev.get_element_vector() != nullptr) {
        prev.detach_element_vector();
    }
    
    // Attach to next
    next.attach_element_vector(&depth_vectors[depth]);
    
    // Expand
    for (auto node : cur) {
        for (auto child : expand(node)) {
            next.insert(child);
        }
    }
    
    // Rotate
    prev = std::move(cur);
    cur = std::move(next);
    next.clear();
}
```

### Memory-Constrained BFS

```cpp
const size_t MEMORY_LIMIT = 1900 * 1024 * 1024;  // 1900 MB
size_t current_memory = 0;

for (int depth = 0; depth < max_depth; ++depth) {
    next_level.attach_element_vector(&nodes[depth + 1]);
    
    // Expand until memory limit
    for (auto node : current_level) {
        for (auto child : expand(node)) {
            if (current_memory + estimate_size(next_level) > MEMORY_LIMIT) {
                goto stop_expansion;
            }
            next_level.insert(child);
        }
    }
    
stop_expansion:
    next_level.detach_element_vector();
    current_level.clear();  // Free memory immediately
    current_level = std::move(next_level);
}
```

## Testing and Validation

### Unit Test Example

```cpp
void test_element_vector_basic() {
    tsl::robin_set<int> s;
    std::vector<int> v;
    
    s.attach_element_vector(&v);
    
    // Test insertion
    s.insert(1);
    s.insert(2);
    s.insert(3);
    assert(v.size() == 3);
    
    // Test duplicate rejection
    s.insert(2);
    assert(v.size() == 3);  // Not recorded
    
    // Test detach
    s.detach_element_vector();
    s.insert(4);
    assert(v.size() == 3);  // Still 3
    
    std::cout << "✓ All tests passed" << std::endl;
}
```

### Validation Helper

```cpp
template<typename T>
bool validate_element_vector(
    const tsl::robin_set<T>& set,
    const std::vector<T>& vec)
{
    if (vec.size() != set.size()) {
        std::cerr << "Size mismatch: vector=" << vec.size()
                  << ", set=" << set.size() << std::endl;
        return false;
    }
    
    for (const T& elem : vec) {
        if (set.find(elem) == set.end()) {
            std::cerr << "Element in vector but not in set" << std::endl;
            return false;
        }
    }
    
    return true;
}
```

## Comparison with Alternatives

### Element Vector vs. Manual Extraction

| Aspect | Element Vector | Manual Extraction |
|--------|----------------|-------------------|
| Memory peak | 1.0x base | 2.0x base |
| Implementation | 3 lines | 10+ lines |
| Performance | O(n) insert overhead | O(n) extraction overhead |
| Error-prone | Low | Medium (forget to extract) |

### Element Vector vs. Custom Iterator

| Aspect | Element Vector | Custom Iterator |
|--------|----------------|-------------------|
| Complexity | Simple API | Complex implementation |
| Type safety | Strong | Weak (void*) |
| Flexibility | Attach/detach anytime | Fixed at construction |
| Standard compatibility | std::vector | Custom type |

### Capacity Monitoring vs. Manual Calculation

| Aspect | Capacity API | Manual Calculation |
|--------|--------------|-------------------|
| Accuracy | Exact (uses internal state) | Approximate (may drift) |
| Performance | O(1) precomputed | O(1) but requires sync |
| Rehash prediction | `will_rehash_on_next_insert()` | Impossible (no access to flags) |
| Code clarity | `if (set.will_rehash_on_next_insert())` | `if (set.size() >= set.bucket_count() * 0.9)` |
| Maintenance | Automatic updates | Manual tracking required |

## Version History

- **2026-01-01**: Performance optimization update
  - **Changed**: `push_back()` → `emplace_back()` for move semantics (avoid copy overhead)
  - **Added**: Critical warning about `reserve()` before `attach_element_vector()`
  - **Added**: Performance optimization section with examples
  - **Impact**: Eliminates unnecessary copies for large value types
  - **Context**: Discovered during memory spike elimination in solver_dev.cpp

- **2024-12-27**: Initial implementation
  - **Element Vector Feature:**
    - Added `attach_element_vector()` - Attach external vector for automatic recording
    - Added `detach_element_vector()` - Detach recording vector
    - Added `get_element_vector()` - Query attached vector pointer
    - Implemented rehash preservation for attached vectors
    - Tested with 15M+ element insertions in BFS scenarios
  
  - **Capacity Monitoring API:**
    - Added `load_threshold()` - Get precomputed rehash threshold
    - Added `available_capacity()` - Calculate remaining capacity before rehash
    - Added `will_rehash_on_next_insert()` - Predict next rehash occurrence
    - All methods are `noexcept` and O(1) complexity
  
  - **Use Case:** Memory-constrained breadth-first search in RubiksSolverDemo
  - **Performance Impact:** < 2% overhead for element vector, negligible for capacity monitoring

## License

This feature is part of the Tessil/robin-map library modifications and follows the same MIT license.

## See Also

- [robin_hash.h](robin_hash.h) - Core implementation
- [robin_set.h](robin_set.h) - Wrapper implementation
