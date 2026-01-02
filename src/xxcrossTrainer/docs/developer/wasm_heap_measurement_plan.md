# WASM Heap Memory Measurement Plan

**Date**: 2026-01-02 (Updated: 2026-01-03)  
**Objective**: Measure actual WASM heap usage at peak for bucket model selection  
**Critical**: Measure **peak heap size**, not final RSS (WASM heap doesn't shrink)  
**Approach**: Unified build - single WASM binary, runtime bucket configuration via UI

---

## Background

**Why WASM heap measurement is needed**:
- Emscripten doesn't use contiguous heap regions
- Theoretical calculations are unreliable
- **WASM heap grows but never shrinks** → peak = final heap size
- Native RSS ≠ WASM heap (overhead varies by model)

**Current Status**:
- Native peak validated: 414 MB (Phase 4, for 4M/4M/4M/2M + depth 10)
- Existing models use theoretical estimates (not measured)
- Need actual WASM heap measurements for each target configuration

**Design Revision (2026-01-03)**:
- **Previous approach**: Build 6 separate WASM binaries (one per bucket config)
- **Current approach**: Single WASM binary, bucket config set at runtime via UI
- **Benefits**: 
  - Smaller deployment (~1-2 MB vs ~6-12 MB)
  - Easier testing (no binary switching)
  - Flexible experimentation (arbitrary bucket sizes)
  - Single build step (~5-10 min vs ~30-60 min)

---

## Target Configurations

### Mobile Targets (4 tiers)

| Tier | WASM Heap Range | Target Heap | Bucket Model Candidate | Notes |
|------|----------------|-------------|------------------------|-------|
| **LOW** | 300-500 MB | ~400 MB | 2M/2M/2M (no depth 10) | Minimal coverage |
| **MIDDLE** | 600-800 MB | ~700 MB | 4M/4M/4M (no depth 10) | Good balance |
| **HIGH** | 900-1100 MB | ~1000 MB | 4M/4M/4M/2M (depth 10) | High coverage |
| **ULTRA** | 1200-1500 MB | ~1350 MB | 8M/8M/8M/4M (depth 10) | Maximum mobile |

### Desktop Targets (2 tiers)

| Tier | WASM Heap Range | Target Heap | Bucket Model Candidate | Notes |
|------|----------------|-------------|------------------------|-------|
| **STANDARD** | 1500-1700 MB | ~1600 MB | 16M/16M/16M/8M | Standard desktop |
| **HIGH** | 1700-2000 MB | ~1850 MB | 32M/32M/32M/16M | High-end desktop |

---

## Measurement Methodology

### Tools
- **Emscripten API**: `emscripten_get_heap_size()` at checkpoints
- **Browser DevTools**: Memory profiler for validation
- **Monitoring points**: All 8 checkpoints (Phase 1-5 + cleanup)

### Process
1. Build WASM with specific bucket configuration
2. Run in browser (Chrome DevTools open)
3. Capture `log_emscripten_heap()` output at all checkpoints
4. Record **final heap size** (= peak, since heap doesn't shrink)
5. Compare with theoretical estimates

### Success Criteria
- Measure actual peak heap for 6 configurations
- Validate overhead factor varies by model (not constant 2.2x)
- Select bucket sizes that fit within target ranges

---

## Measurement Candidates (Priority Order)

### Phase 1: Mobile Critical Paths (Priority: HIGH)

#### Test 1: LOW Mobile (300-500 MB target)
**Config**: `2M/2M/2M/0` (no depth 10)
```bash
BUCKET_MODEL=CUSTOM CUSTOM_BUCKET_7=2M CUSTOM_BUCKET_8=2M CUSTOM_BUCKET_9=2M CUSTOM_BUCKET_10=0
```
**Expected**:
- Native theoretical: ~120 MB (depth 0-9)
- Estimated WASM: 120 × 2.5 = 300 MB
- **Target**: 300-400 MB WASM heap

#### Test 2: MIDDLE Mobile (600-800 MB target)
**Config**: `4M/4M/4M/0` (no depth 10)
```bash
BUCKET_MODEL=CUSTOM CUSTOM_BUCKET_7=4M CUSTOM_BUCKET_8=4M CUSTOM_BUCKET_9=4M CUSTOM_BUCKET_10=0
```
**Expected**:
- Native theoretical: ~250 MB (depth 0-9, Phase 4 peak)
- Estimated WASM: 250 × 2.8 = 700 MB
- **Target**: 650-750 MB WASM heap

#### Test 3: HIGH Mobile (900-1100 MB target)
**Config**: `4M/4M/4M/2M` **(Current 414 MB native)**
```bash
BUCKET_MODEL=4M/4M/4M/2M (existing measurements)
```
**Expected**:
- Native measured: 414 MB (Phase 4 peak)
- Estimated WASM: 414 × 2.4 = 994 MB
- **Target**: 950-1050 MB WASM heap

#### Test 4: ULTRA Mobile (1200-1500 MB target)
**Config**: `8M/8M/8M/4M`
```bash
BUCKET_MODEL=CUSTOM CUSTOM_BUCKET_7=8M CUSTOM_BUCKET_8=8M CUSTOM_BUCKET_9=8M CUSTOM_BUCKET_10=4M
```
**Expected**:
- Native theoretical: ~550 MB (Phase 4 peak estimate)
- Estimated WASM: 550 × 2.4 = 1320 MB
- **Target**: 1250-1400 MB WASM heap

### Phase 2: Desktop Configurations (Priority: MEDIUM)

#### Test 5: STANDARD Desktop (1500-1700 MB target)
**Config**: `16M/16M/16M/8M`
```bash
BUCKET_MODEL=CUSTOM CUSTOM_BUCKET_7=16M CUSTOM_BUCKET_8=16M CUSTOM_BUCKET_9=16M CUSTOM_BUCKET_10=8M
```
**Expected**:
- Native theoretical: ~700 MB
- Estimated WASM: 700 × 2.3 = 1610 MB
- **Target**: 1550-1650 MB WASM heap

#### Test 6: HIGH Desktop (1700-2000 MB target)
**Config**: `32M/32M/32M/16M`
```bash
BUCKET_MODEL=CUSTOM CUSTOM_BUCKET_7=32M CUSTOM_BUCKET_8=32M CUSTOM_BUCKET_9=32M CUSTOM_BUCKET_10=16M
```
**Expected**:
- Native theoretical: ~850 MB
- Estimated WASM: 850 × 2.2 = 1870 MB
- **Target**: 1800-1950 MB WASM heap

---

## Implementation Steps

### Step 1: Create WASM Build Script ✅
**File**: `build_wasm_heap_measurement.sh`
```bash
#!/bin/bash
# Build WASM with specific bucket configuration and heap monitoring

CONFIG=$1  # e.g., "2M/2M/2M/0"
OUTPUT="solver_heap_${CONFIG//\//_}"

# Parse config
IFS='/' read -ra BUCKETS <<< "$CONFIG"
BUCKET_7=${BUCKETS[0]}
BUCKET_8=${BUCKETS[1]}
BUCKET_9=${BUCKETS[2]}
BUCKET_10=${BUCKETS[3]:-0}

emcc -std=c++17 -O3 -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=64MB \
  -s MAXIMUM_MEMORY=2GB \
  -s EXPORTED_FUNCTIONS='["_main", "_malloc", "_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
  -DVERBOSE_MODE \
  -I.. \
  -o "${OUTPUT}.js" \
  solver_dev.cpp \
  -D CUSTOM_BUCKET_7=$(parse_size $BUCKET_7) \
  -D CUSTOM_BUCKET_8=$(parse_size $BUCKET_8) \
  -D CUSTOM_BUCKET_9=$(parse_size $BUCKET_9) \
  -D CUSTOM_BUCKET_10=$(parse_size $BUCKET_10)
```

### Step 2: Create Browser Test Harness
**File**: `wasm_heap_test.html`
- Load WASM module
- Capture console output (including `log_emscripten_heap()`)
- Display heap usage in real-time
- Save results to localStorage/file

### Step 3: Run Measurements
For each of 6 configurations:
1. Build WASM: `./build_wasm_heap_measurement.sh "4M/4M/4M/2M"`
2. Open `wasm_heap_test.html` in Chrome
3. Run solver with test scramble
4. Record final heap size from console
5. Validate with DevTools Memory Profiler

### Step 4: Analyze Results
- Create table: Config → Native RSS → WASM Heap → Overhead Factor
- Identify patterns (does overhead vary by size?)
- Adjust bucket models to fit target ranges
- Update `bucket_config.h` with measured values

### Step 5: Update Documentation
- Record all measurements in `wasm_heap_measurements.md`
- Update `IMPLEMENTATION_PROGRESS.md` with results
- Update `bucket_config.h` with final model selection logic

---

## Implementation Details (2026-01-03)

### Bucket Size Constraints
- **UI Change**: Switched from number inputs to dropdowns
- **Allowed values**: 1, 2, 4, 8, 16, 32 MB (powers of 2 only)
- **Rationale**: Hash table sizes are powers of 2, alignment matches memory allocator

### Solver Integration (`solve_with_custom_buckets`)
```cpp
// Function signature
SolverStatistics solve_with_custom_buckets(
    int bucket_7_mb, int bucket_8_mb, 
    int bucket_9_mb, int bucket_10_mb,
    bool verbose = true
);

// Integration approach
1. Create BucketConfig with model=CUSTOM
2. Set bucket sizes (MB → bytes conversion)
3. Create ResearchConfig:
   - skip_search = true (database build only)
   - enable_custom_buckets = true
   - high_memory_wasm_mode = true
4. Instantiate xxcross_search(adj=true, BFS_DEPTH=6, total_mb, verbose, bucket_config, research_config)
5. Collect statistics from solver.index_pairs
6. Measure heap before/after with emscripten_get_heap_size()
7. Generate scrambles using solver.get_xxcross_scramble(std::to_string(depth))
```

### Statistics Collection
- **Node counts**: Real values from `solver.index_pairs[depth].size()`
- **Heap usage**: `emscripten_get_heap_size()` before/after database construction
- **Scrambles**: Generated per depth (1-10) using solver instance's `get_xxcross_scramble()`
- **Scramble lengths**: Calculated via `StringToAlg(scramble).size()` for depth verification
  - Validates that depth N scrambles have N moves
  - Detects potential generation errors
- **Load factors**: 0.88 (typical for tsl::robin_set after rehash)
  - robin_set achieves ~0.88-0.93 load factor in practice
  - Actual bucket_count() not accessible after solver construction
  - Hash table instances (prev/cur/next) are local to BFS expansion
- **Children per parent**: Placeholder (TODO: add instrumentation during expansion)

### Advanced HTML UI Features
- Per-depth bucket configuration (4 dropdowns: depth 7, 8, 9, 10)
- 6 preset buttons (Mobile: LOW/MIDDLE/HIGH/ULTRA, Desktop: STD/HIGH)
- Statistics display tables:
  - Node distribution (depth, count, cumulative)
  - Load factors (bucket, LF, efficiency %)
  - Memory usage (final heap MB, peak heap MB)
  - Sample scrambles (one per depth)
- Console output capture with heap log highlighting
- CSV download with full results

---

## Expected Challenges

### Challenge 1: Overhead Factor Variance
**Problem**: Overhead may not be constant 2.2x across all sizes
**Solution**: Measure actual overhead for each config, adjust models accordingly

### Challenge 2: Browser Memory Limit
**Problem**: Some browsers may limit heap growth
**Solution**: Test in Chrome (most permissive), document browser compatibility

### Challenge 3: Measurement Timing
**Problem**: `emscripten_get_heap_size()` called at checkpoints may miss transient peaks
**Solution**: WASM heap doesn't shrink → final size = peak size (no issue)

---

## Deliverables

1. ✅ **Build Script**: `build_wasm_heap_measurement.sh`
2. ✅ **Test Harness**: `wasm_heap_test.html`
3. ✅ **Measurement Results**: Table of 6 configs with actual WASM heap usage
4. ✅ **Updated bucket_config.h**: Replace theoretical values with measured WASM heap
5. ✅ **Documentation**: `wasm_heap_measurements.md` with full results

---

## Timeline

- **Preparation** (1-2 hours): Create build script + test harness
- **Measurement** (3-4 hours): Build and test 6 configurations
- **Analysis** (1 hour): Create results table, identify patterns
- **Integration** (1 hour): Update `bucket_config.h`, documentation

**Total**: 6-8 hours

---

## Success Metrics

- ✅ Measured WASM heap for all 6 target configurations
- ✅ Identified actual overhead factors (not assumed 2.2x)
- ✅ Selected bucket models that fit within target ranges
- ✅ Validated mobile configs fit within 512 MB / 1500 MB limits
- ✅ Desktop configs fit within 2000 MB limit
