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

The solver uses **flexible bucket allocation** based on available memory:

```cpp
// From solver_dev.cpp (calculate_buckets_flexible)
const int EMPIRICAL_OVERHEAD = 33;  // bytes per slot

if (memory_limit_mb >= 1702) {
    bucket_7_mb = 16; bucket_8_mb = 16; bucket_9_mb = 16;
} else if (memory_limit_mb >= 1537) {
    bucket_7_mb = 8; bucket_8_mb = 16; bucket_9_mb = 8;
} else if (memory_limit_mb >= 1000) {
    bucket_7_mb = 8; bucket_8_mb = 8; bucket_9_mb = 8;
}
// ... (see SOLVER_IMPLEMENTATION.md for complete logic)
```

### Memory Behavior

Key empirical findings:
- **Fixed Overhead**: 62 MB (C++ runtime, allocator metadata, etc.)
- **Baseline Memory**: `bucket_slots × bytes_per_slot` (27-43 bytes/slot depending on configuration)
- **No Spike Reserve**: Large buckets (8M+) show minimal transient memory spikes
- **Load Factor**: Varies 0.6-1.0 (large buckets maintain ~65% load for performance)

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

## License

(Include license information here)

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
