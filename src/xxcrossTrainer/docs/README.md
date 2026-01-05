# XXCross Trainer Solver - Developer Documentation

**Version**: stable_20260103  
**Last Updated**: 2026-01-03

> **Navigation**: [User Guide](USER_GUIDE.md) | Developer Docs (You are here)

---

## Overview

This is a high-performance Rubik's Cube XXCross (double cross) solver designed for both native C++ and WebAssembly environments. The solver uses a **5-Phase BFS approach** with **configurable bucket models** to efficiently solve F2L (First Two Layers) configurations.

### Key Features

- **5-Phase Construction**: Depth 0-6 full BFS + Depth 7-10 local expansion (depth 10 via random sampling)
- **Memory-Optimized**: Eliminates transient spikes via pre-reserve pattern
- **Multi-Tier WASM Support**: 6 bucket models (Mobile: LOW/MIDDLE/HIGH, Desktop: STD/HIGH/ULTRA)
- **Production-Ready**: Comprehensive heap measurements (13-configuration WASM campaign) + 6,000-trial performance validation (Jan 2026)
- **Dual Platform**: Native C++ and WebAssembly with unified API
- **Guaranteed Depth Scrambles**: Depth guarantee algorithm ensures exact scramble depth even with sparse coverage (2026-01-04)

### Production Status (January 2026)

**Status**: ✅ Production deployed - xxcross_trainer.html operational with browser verification

**Deployment Configuration**:
- **Default Model**: MOBILE_LOW (1M/1M/2M/4M buckets, ~600MB dual-heap)
- **Implementation**: Hardcoded in xxcross_trainer.html (no bucket selection UI)
- **Browser Testing**: Verified on desktop (PC) and mobile devices
- **Performance**: 10-12s initialization, ~12-14ms depth 10 generation

**Comprehensive Testing Results** (6,000 trials):
- ✅ **100% depth guarantee accuracy** - All 6,000 trials generated exact depth scrambles
- ✅ **<20ms depth 10 generation** - All models meet production requirements
- ✅ **10-96s initialization** - Scales predictably with database size
- ✅ **Production ready** - Comprehensive testing validates WASM module stability

**Design Decision**: MOBILE_LOW selected as default configuration for simplicity, stability, and validated performance. Bucket selection UI not implemented - minimal configuration sufficient for all use cases.

For detailed performance data, see [developer/performance_results.md](developer/performance_results.md)

---

## Quick Start

### For Users

See **[USER_GUIDE.md](USER_GUIDE.md)** for:
- Recommended memory configurations
- Usage examples (CLI, Docker, Kubernetes)
- Common troubleshooting

### For Developers

1. **Build the solver**:
   ```bash
   # From workspace root
   cd src/xxcrossTrainer
   g++ -std=c++17 -O3 -march=native solver_dev.cpp -o solver_dev
   ```

2. **Run with memory limit**:
   ```bash
   ./solver_dev 1005  # 1005 MB recommended (8M/8M/8M configuration)
   ```

3. **Build for WebAssembly**:
   See **[developer/WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)**

---

## Documentation Structure

### User Documentation
- **[USER_GUIDE.md](USER_GUIDE.md)** - End-user guide with deployment examples (includes Advanced Configuration section)

### Developer Documentation (`developer/`)

#### Core Technical Docs
- **[SOLVER_IMPLEMENTATION.md](developer/SOLVER_IMPLEMENTATION.md)** - Detailed code logic (database construction, IDA*, etc.)
- **[WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)** - WebAssembly compilation and testing

#### Integration & Deployment
- **[WASM_INTEGRATION_GUIDE.md](developer/WASM_INTEGRATION_GUIDE.md)** - Complete WASM integration for trainers (advanced, practical)
- **[WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)** - Emscripten build process (beginner-friendly)
- **[WASM_EXPERIMENT_SCRIPTS.md](developer/WASM_EXPERIMENT_SCRIPTS.md)** - JavaScript test scripts and examples (Node.js, browser)
- **[WASM_MEASUREMENT_README.md](developer/WASM_MEASUREMENT_README.md)** - Heap measurement methodology (deprecated, see WASM_INTEGRATION_GUIDE)
- **[API_REFERENCE.md](developer/API_REFERENCE.md)** - Complete function reference

#### Memory Analysis
- **[MEMORY_MONITORING.md](developer/MEMORY_MONITORING.md)** - Memory tracking tools
- **[IMPLEMENTATION_PROGRESS.md](developer/IMPLEMENTATION_PROGRESS.md)** - Development tracker

#### Experimental Results (`developer/Experiments/`)
- **[wasm_heap_measurement_results.md](developer/Experiments/wasm_heap_measurement_results.md)** - 6-tier bucket model
- **[wasm_heap_measurement_data.md](developer/Experiments/wasm_heap_measurement_data.md)** - 13-configuration raw data
- **[depth_10_memory_spike_investigation.md](developer/Experiments/depth_10_memory_spike_investigation.md)** - Spike elimination
- **[depth_10_implementation_results.md](developer/Experiments/depth_10_implementation_results.md)** - Phase 5 results

---

## Memory Configuration (Quick Reference)

### WASM 6-Tier Model (Dual-Solver Architecture)

| Tier | Dual Heap | Bucket Config | Total Nodes | Target Devices |
|------|-----------|---------------|-------------|----------------|
| **Mobile LOW** | **618 MB** | 1M/1M/2M/4M | 12.1M | Low-end phones |
| **Mobile MIDDLE** | **896 MB** | 2M/4M/4M/4M | 17.8M | Mid-range phones |
| **Mobile HIGH** | **1070 MB** | 4M/4M/4M/4M | 19.7M | High-end phones |
| **Desktop STD** | **1512 MB** | 8M/8M/8M/8M | 34.7M | ✅ **Recommended** |
| **Desktop HIGH** | **2786 MB** | 8M/16M/16M/16M | 57.4M | High-memory PC |
| **Desktop ULTRA** | **2884 MB** | 16M/16M/16M/16M | 65.0M | 4GB limit |

For detailed tier selection and integration, see:
- [WASM Integration Guide](developer/WASM_INTEGRATION_GUIDE.md)
- [WASM Measurement Results](developer/Experiments/wasm_heap_measurement_results.md)

---

## Key Implementation Details

### Bucket Configuration Model

The solver uses **explicit bucket specification** for depths 7-10:

```cpp
// Current implementation: Explicit bucket sizes (stable_20260103)
// Bucket configuration specified via:
// 1. Pre-defined WASM tiers (bucket_config.h: WASM_MOBILE_LOW/MIDDLE/HIGH, WASM_DESKTOP_STD/HIGH/ULTRA)
// 2. Environment variables (BUCKET_D7, BUCKET_D8, BUCKET_D9, BUCKET_D10)

// Example: Desktop STD tier (8M/8M/8M/8M)
BUCKET_D7=8   // 8M slots for depth 7
BUCKET_D8=8   // 8M slots for depth 8
BUCKET_D9=8   // 8M slots for depth 9
BUCKET_D10=8  // 8M slots for depth 10

// WASM heap usage: 756 MB (single solver), 1512 MB (dual-solver)
```

**Bucket Configuration Sources**:
1. **WASM Pre-defined Tiers** (6 tiers from Mobile LOW to Desktop ULTRA)
2. **Environment Variables** (developer mode, custom configurations)
3. **Legacy** (deprecated): ~~Memory limit-based automatic selection~~

### Memory Behavior

Key characteristics:
- **Explicit Specification**: Bucket sizes are directly specified (not calculated from memory limits)
- **Measured WASM Heaps**: Each tier has empirically measured heap usage (13-configuration campaign)
- **Baseline Memory**: `bucket_slots × ~21-27 bytes/slot` (varies by configuration)
- **Load Factor**: ~0.88-0.93 (tsl::robin_set after rehashing)
- **Dual-Solver Architecture**: Total heap = 2× single solver (adjacent + opposite F2L slots)

For measurement data and tier selection, see [developer/Experiments/wasm_heap_measurement_results.md](developer/Experiments/wasm_heap_measurement_results.md).

---

## Advanced Topics

### Developer Mode (Custom Bucket Sizes)

For advanced users who need direct control over bucket configurations, you can specify exact bucket sizes using environment variables.

**Enable Custom Buckets**:
```bash
export ENABLE_CUSTOM_BUCKETS=1
export BUCKET_D7=8   # Hash table size for depth 7 (millions of slots)
export BUCKET_D8=8   # Hash table size for depth 8
export BUCKET_D9=8   # Hash table size for depth 9
export BUCKET_D10=8  # Hash table size for depth 10 (Phase 5 random sampling)

# Optional: Disable malloc trim (advanced use only)
export DISABLE_MALLOC_TRIM=1

# Run solver
./solver_dev
```

**Example Configurations**:

```bash
# Mobile LOW (618 MB dual-solver WASM)
export ENABLE_CUSTOM_BUCKETS=1
export BUCKET_D7=1 BUCKET_D8=1 BUCKET_D9=2 BUCKET_D10=4

# Desktop STD (1512 MB dual-solver WASM) - Recommended
export BUCKET_D7=8 BUCKET_D8=8 BUCKET_D9=8 BUCKET_D10=8

# Desktop ULTRA (2884 MB dual-solver WASM) - Maximum
export BUCKET_D7=16 BUCKET_D8=16 BUCKET_D9=16 BUCKET_D10=16
```

**Memory Estimation**:
- Each 1M slots ≈ 60-80 MB WASM heap (including overhead)
- Total heap ≈ (D7 + D8 + D9 + D10) × 70 MB (approximate)
- Native C++ RSS ≈ WASM heap / 2.2 (typical overhead factor)
- Always validate with actual measurements (see WASM_INTEGRATION_GUIDE)

**Use Cases**:
- Testing new bucket combinations
- Platform-specific optimization
- Research and experimentation
- Custom trainer requirements

### Creating New Bucket Models

To create and register new WASM memory configurations:

1. **Measure WASM Heap Usage**:
   ```bash
   # From workspace root
   cd src/xxcrossTrainer
   
   # Build current production WASM
   source ~/emsdk/emsdk_env.sh
   em++ solver_dev.cpp -o solver_dev.js -std=c++17 -O3 -I../.. \
     -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=4GB \
     -s EXPORTED_RUNTIME_METHODS='["cwrap"]' -s MODULARIZE=1 \
     -s EXPORT_NAME="createModule" --bind -s INITIAL_MEMORY=64MB
   
   # Test with browser
   python3 -m http.server 8000
   # Open http://localhost:8000/test_wasm_browser.html
   # Test custom bucket combinations
   # Record peak heap sizes
   
   # Note: build_wasm_unified.sh exists for legacy measurement workflows only
   # (builds solver_heap_measurement.{js,wasm} for archived tools)
   ```

2. **Update bucket_config.h**:
   ```cpp
   // Add new enum value
   enum class BucketConfigModel {
       // ... existing models ...
       WASM_CUSTOM_NEW,
   };
   
   // Add configuration entry to BUCKET_CONFIGS map
   {BucketConfigModel::WASM_CUSTOM_NEW, {
       {7, 12000000}, {8, 12000000}, {9, 12000000}, {10, 12000000},
       2100,           // Measured WASM heap (MB)
       42.5,           // Total nodes (millions)
       0.85,           // Average load factor
       "WASM_CUSTOM_NEW"
   }},
   ```

3. **Document Results**:
   - Add measurement data to [developer/Experiments/wasm_heap_measurement_results.md](developer/Experiments/wasm_heap_measurement_results.md)
   - Update tier recommendations if creating new production tier

**Complete Workflow**:
- Measurement methodology: [developer/WASM_MEASUREMENT_README.md](developer/WASM_MEASUREMENT_README.md)
- Integration patterns: [developer/WASM_INTEGRATION_GUIDE.md](developer/WASM_INTEGRATION_GUIDE.md)
- Build instructions: [developer/WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)

---

## Development Workflow

### Testing Memory Behavior

```bash
# Single test
./solver_dev_test 1005
```

See **[developer/MEMORY_MONITORING.md](developer/MEMORY_MONITORING.md)** for detailed monitoring techniques and script implementations.

### Running Comprehensive Tests

All test scripts are documented with complete implementations in:
- **[developer/EXPERIMENT_SCRIPTS.md](developer/EXPERIMENT_SCRIPTS.md)** - Shell script implementations
- **[developer/MEMORY_MONITORING.md](developer/MEMORY_MONITORING.md)** - Python monitoring script

Scripts can be extracted from documentation and executed as needed.

### WASM Development

```bash
# Build WASM
emcc -O3 solver_dev.cpp -o solver.js \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \
  --bind

# Test
node test_wasm.js
```

See **[developer/WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)** for complete build instructions.

---

## Experimental Results Overview

The `developer/Experiments/` directory contains empirical findings from comprehensive measurement campaigns. These documents provide the foundation for production configurations and guide future optimization.

### Current Experiments (2026-01-03)

#### WASM Heap Measurements
- **[wasm_heap_measurement_results.md](developer/Experiments/wasm_heap_measurement_results.md)** - Final 6-tier model recommendation
  - 13 configurations measured (1M/1M/2M/4M through 16M/16M/16M/16M)
  - Dual-solver architecture validated (adjacent + opposite orientations)
  - Mobile/Desktop tier classification
  - Complete with load factors and node counts

- **[wasm_heap_measurement_data.md](developer/Experiments/wasm_heap_measurement_data.md)** - Raw measurement data
  - Per-configuration heap sizes
  - CSV extraction scripts
  - Measurement methodology validation

#### Depth 10 Implementation
- **[depth_10_implementation_results.md](developer/Experiments/depth_10_implementation_results.md)** - Phase 5 expansion proof-of-concept
  - Coverage improvement: 30% → ~85% (+185% relative increase)
  - 1.89M nodes at depth 10 (4M/4M/4M/2M config)
  - Critical bug fixes documented (segfault, rehash, element vector timing)
  
- **[depth_10_expansion_design.md](developer/Experiments/depth_10_expansion_design.md)** - Strategic design rationale
  - Analysis of depth 10 as volume peak (60-70% of all xxcross solutions)
  - 3 implementation options evaluated
  - Random sampling approach selected

- **[depth_10_peak_rss_validation.md](developer/Experiments/depth_10_peak_rss_validation.md)** - 10ms monitoring validation
  - Actual peak RSS: 442 MB (not 378 MB from C++ checkpoints)
  - Gap analysis: +64 MB transient allocations
  - WASM deployment implications

- **[depth_10_memory_spike_investigation.md](developer/Experiments/depth_10_memory_spike_investigation.md)** - Spike elimination
  - Pre-reserve pattern implementation
  - Before/after RSS traces
  - Production readiness validation

#### Memory Optimization
- **[peak_rss_optimization.md](developer/Experiments/peak_rss_optimization.md)** - Optimization strategies
  - depth8_vec elimination: -58 MB peak RSS reduction
  - Malloc_trim integration analysis
  - Face-diverse scramble generation

- **[bucket_model_rss_measurement.md](developer/Experiments/bucket_model_rss_measurement.md)** - Native C++ measurements
  - 7 bucket configurations (2M/2M/2M through 16M/16M/16M)
  - Allocator cache behavior (malloc_trim analysis)
  - Platform consistency validation

### Archived Experiments

Historical measurement campaigns and deprecated analyses are preserved in `developer/_archive/Experiments_old/`:
- MEASUREMENT_RESULTS_20251230.md - 47-point memory campaign (stable-20251230)
- MEASUREMENT_SUMMARY.md - Executive summary
- MEMORY_THEORY_ANALYSIS.md - Theoretical deep-dive
- THEORETICAL_MODEL_REVISION.md - Model evolution

### Using Experimental Data

**For Production Deployment**:
1. Start with [wasm_heap_measurement_results.md](developer/Experiments/wasm_heap_measurement_results.md) for tier selection
2. Reference bucket_model_rss_measurement.md for native C++ configurations
3. Consult depth_10_implementation_results.md for Phase 5 behavior

**For Research/Optimization**:
1. Review measurement methodologies in wasm_heap_measurement_data.md
2. Study spike elimination techniques in depth_10_memory_spike_investigation.md
3. Analyze optimization strategies in peak_rss_optimization.md

**For Understanding Memory Behavior**:
1. Read depth_10_peak_rss_validation.md for 10ms monitoring insights
2. Study bucket_model_rss_measurement.md for allocator behavior
3. Consult archived documents for historical context

---

## Project History

### Recent Major Updates

**2026-01-03** (stable_20260103):
- ✅ Added depth 10 support (random sampling, Phase 5)
- ✅ Eliminated all memory spikes (pre-reserve pattern)
- ✅ Created 6-tier WASM bucket model (Mobile/Desktop)
- ✅ Complete WASM heap measurement (13 configurations)
- ✅ Comprehensive documentation rewrite (API_REFERENCE, SOLVER_IMPLEMENTATION)
- ✅ Malloc trim management (ENV configurable)

**2025-12-30** (stable-20251230):
- ✅ Completed 47-point memory measurement campaign (300-2000 MB)
- ✅ Verified all 6 bucket threshold transitions (±1 MB precision)
- ✅ Documented theoretical model limitations and empirical findings
- ✅ Created production-ready configuration guide with safety margins
- ✅ Declared stable version with comprehensive documentation

For detailed history, see `developer/Experiments/` documentation.

---

## Performance Characteristics

### Solving Speed

- **Average solving time**: ~165-180 seconds (for typical scrambles, including depth 10)
- **Phases**:
  - Phase 1 (BFS 0-6): ~30s
  - Phase 2 (Partial expansion depth 7): ~10s
  - Phase 3 (Local BFS 7→8): ~40s
  - Phase 4 (Local BFS 8→9): ~70s
  - Phase 5 (Depth 10 random sampling): ~15-20s

### Memory Stability

Based on 660,000+ RSS samples:
- **Coefficient of Variation**: <0.03% (extremely stable)
- **Peak variation within config**: <0.5 MB

---

## Contributing

### Before Making Changes

1. **Back up current state**: User maintains backup in `src/xxcrossTrainer_backup_YYYYMMDD_HHMM/`
2. **Review documentation**: Understand current implementation via this README and linked docs
3. **Run tests**: Verify changes don't affect memory behavior

### Documentation Standards

- **User-facing docs**: Keep in `docs/` root (README.md, USER_GUIDE.md)
- **Developer docs**: Place in `docs/developer/`
- **Experimental findings**: Archive in `docs/developer/Experiments/`
- **Code samples**: Embed directly in docs (not as script file references)

---

## Troubleshooting

### Common Issues

**Problem**: Solver crashes with memory error

**Solution**: Check memory limit vs configuration:
```bash
# Too low (will crash)
./solver_dev 500  # Tries 4M/4M/4M but needs 434 MB minimum

# Safe
./solver_dev 1005  # 8M/8M/8M with margin
```

**Problem**: WASM runs out of memory

**Solution**: Increase initial memory in emcc flags:
```bash
-s INITIAL_MEMORY=268435456  # 256 MB minimum
```

For more troubleshooting, see [USER_GUIDE.md](USER_GUIDE.md#troubleshooting).

---

## Contact / Support

For questions about:
- **Usage**: See [USER_GUIDE.md](USER_GUIDE.md)
- **Development**: Review `docs/developer/` documentation
- **Memory configuration**: See [USER_GUIDE.md - Advanced Configuration](USER_GUIDE.md#advanced-configuration)

---

**Next Steps for New Developers**:

1. Read [USER_GUIDE.md](USER_GUIDE.md) to understand basic usage
2. Review [developer/SOLVER_IMPLEMENTATION.md](developer/SOLVER_IMPLEMENTATION.md) for code structure
3. Explore [developer/Experiments/](developer/Experiments/) for empirical insights
4. Try building and running the solver locally
5. Experiment with different memory configurations

**Happy Coding!** 🎲
