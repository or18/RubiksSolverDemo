# XXCross Trainer Solver - Developer Documentation

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: [User Guide](USER_GUIDE.md) | [Memory Configuration](MEMORY_CONFIGURATION_GUIDE.md) | Developer Docs (You are here)

---

## Overview

This is a high-performance Rubik's Cube XXCross (double cross) solver designed for both native C++ and WebAssembly environments. The solver uses a **Two-Phase BFS approach** with **memory-adaptive bucket allocation** to efficiently solve F2L (First Two Layers) configurations.

### Key Features

- **Two-Phase Optimal Solving**: Depth 0-6 full BFS + Depth 7-9 local BFS
- **Memory-Adaptive Buckets**: Automatically adjusts hash table sizes based on available memory (300 MB - 2000+ MB)
- **WebAssembly Support**: Compiles to WASM for browser-based solving
- **Production-Ready**: Extensively tested with empirical memory measurements (47-point campaign)
- **Stable Performance**: Peak memory variation <0.03% within same configuration

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
   cd RubiksSolverDemo/src/xxcrossTrainer
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
- **[USER_GUIDE.md](USER_GUIDE.md)** - End-user guide with deployment examples
- **[MEMORY_CONFIGURATION_GUIDE.md](MEMORY_CONFIGURATION_GUIDE.md)** - Production memory configuration reference

### Developer Documentation (`developer/`)

#### Core Technical Docs
- **[SOLVER_IMPLEMENTATION.md](developer/SOLVER_IMPLEMENTATION.md)** - Detailed code logic (database construction, IDA*, etc.)
- **[WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md)** - WebAssembly compilation and testing

#### Memory Analysis & Experiments
- **[MEMORY_MONITORING.md](developer/MEMORY_MONITORING.md)** - `/proc`-based memory monitoring system
- **[EXPERIMENT_SCRIPTS.md](developer/EXPERIMENT_SCRIPTS.md)** - Shell script samples for memory testing
- **[WASM_EXPERIMENT_SCRIPTS.md](developer/WASM_EXPERIMENT_SCRIPTS.md)** - WASM testing script examples

#### Experimental Findings (`developer/Experiences/`)
- **[MEASUREMENT_RESULTS_20251230.md](developer/Experiences/MEASUREMENT_RESULTS_20251230.md)** - Comprehensive 47-point measurement analysis
- **[MEMORY_THEORY_ANALYSIS.md](developer/Experiences/MEMORY_THEORY_ANALYSIS.md)** - Theoretical understanding of memory behavior
- **[THEORETICAL_MODEL_REVISION.md](developer/Experiences/THEORETICAL_MODEL_REVISION.md)** - Model evolution and limitations
- **[MEASUREMENT_SUMMARY.md](developer/Experiences/MEASUREMENT_SUMMARY.md)** - Executive summary of findings

---

## Memory Configuration (Quick Reference)

The solver **automatically selects** bucket configuration based on memory limit:

| Memory Limit | Configuration | Peak Usage | Safety Margin |
|--------------|---------------|------------|---------------|
| 300-568 MB | 2M/2M/2M | 293 MB | +23 MB |
| 569-612 MB | 2M/4M/2M | 404 MB | +165 MB |
| 613-920 MB | 4M/4M/4M | 434 MB | +179 MB |
| 921-999 MB | 4M/8M/4M | 655 MB | +266 MB |
| **1000-1536 MB** | **8M/8M/8M** | **748 MB** | **+252 MB** ✅ |
| 1537-1701 MB | 8M/16M/8M | 1189 MB | +348 MB |
| 1702+ MB | 16M/16M/16M | 1375 MB | +327 MB |

**Recommended**: `1005 MB` limit (8M/8M/8M with 10% safety margin)

For detailed configuration strategies, see [MEMORY_CONFIGURATION_GUIDE.md](MEMORY_CONFIGURATION_GUIDE.md).

---

## Key Implementation Details

### Bucket Allocation Logic

The solver uses **calculation-based bucket allocation** that determines bucket sizes dynamically:

```cpp
// From solver_dev.cpp (determine_bucket_sizes)
// Key calculation components:
// 1. Remaining memory after Phase 1 BFS (depth 0-6)
// 2. Safety margin (20-25% for WASM compatibility)
// 3. Memory per slot (33 bytes empirically measured)
// 4. Target allocation ratios (e.g., 2:2:2 or 1:2 for n+1:n+2)

// Example: For 1000 MB limit
usable_memory = remaining_memory * 0.80;  // 20% safety margin
target_d9 = usable_memory / (6 * 33);     // 6*d9 for 2:2:2 ratio
bucket_7 = round_to_power_of_2(target_d9 * 2, 2M, 32M);
bucket_8 = round_to_power_of_2(target_d9 * 2, 2M, 32M);
bucket_9 = round_to_power_of_2(target_d9 * 2, 2M, 32M);
// Result: typically 8M/8M/8M for 1000 MB

// Then flexible upgrade if budget allows
if (remaining_budget >= bucket_8 * 33) {
    bucket_8 *= 2;  // Upgrade to 16M if possible
}
```

### Memory Behavior

Key characteristics:
- **Dynamic Calculation**: Bucket sizes determined by runtime logic, not lookup tables
- **Safety Margins**: 20-25% buffer for WASM compatibility and rehash protection
- **Baseline Memory**: `bucket_slots × 33 bytes/slot` (empirically measured overhead)
- **Load Factor**: Varies 0.65-0.90 (large buckets maintain higher load for efficiency)
- **Stable Parameters**: While calculation-based, empirically-tuned constants are now fixed and stable

For theoretical analysis, see [developer/Experiences/MEMORY_THEORY_ANALYSIS.md](developer/Experiences/MEMORY_THEORY_ANALYSIS.md).

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

## Project History

### Recent Major Updates

**2025-12-30** (stable-20251230):
- ✅ Completed 47-point memory measurement campaign (300-2000 MB)
- ✅ Verified all 6 bucket threshold transitions (±1 MB precision)
- ✅ Documented theoretical model limitations and empirical findings
- ✅ Created production-ready configuration guide with safety margins
- ✅ Declared stable version with comprehensive documentation

**2025-12-29**:
- Fixed memory threshold calculation bugs
- Improved bucket allocation flexibility
- Added RSS monitoring system

For detailed history, see `developer/Experiences/` documentation.

---

## Performance Characteristics

### Solving Speed

- **Average solving time**: ~150 seconds (for typical scrambles)
- **Phases**:
  - Phase 1 (BFS 0-6): ~30s
  - Phase 2 (Partial expansion): ~10s
  - Phase 3 (Local BFS 7→8): ~40s
  - Phase 4 (Local BFS 8→9): ~70s

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
- **Experimental findings**: Archive in `docs/developer/Experiences/`
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
- **Memory behavior**: See [MEMORY_CONFIGURATION_GUIDE.md](MEMORY_CONFIGURATION_GUIDE.md)

---

**Next Steps for New Developers**:

1. Read [USER_GUIDE.md](USER_GUIDE.md) to understand basic usage
2. Review [developer/SOLVER_IMPLEMENTATION.md](developer/SOLVER_IMPLEMENTATION.md) for code structure
3. Explore [developer/Experiences/](developer/Experiences/) for empirical insights
4. Try building and running the solver locally
5. Experiment with different memory configurations

**Happy Coding!** 🎲
