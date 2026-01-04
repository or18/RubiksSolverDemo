# Production Directory

Production-ready WASM modules and Worker files.

## Overview

This directory contains the production WASM implementation with:
- **Dual solver support**: Adjacent and Opposite F2L pairs
- **Bucket model selection**: 6 presets from MOBILE_LOW to DESKTOP_ULTRA
- **Depth guarantee**: 100% accurate depth scramble generation
- **Optimized build**: -O3 -msimd128 -flto (95KB JS, 302KB WASM)

**Performance**: All models achieve <20ms mean depth 10 generation with initialization times ranging from 10-96s depending on bucket size.

**Status**: ✅ Production ready - All 12 configurations tested (6,000 trials)

## Production Files (Git Tracked)

### Build Artifacts
- **solver_prod.js** (95KB) - JavaScript glue code
- **solver_prod.wasm** (302KB) - Compiled WebAssembly binary
- **worker.js** (4.0KB) - Web Worker implementation

### Source Code
- **solver_prod_stable_20260103.cpp** (171KB) - Main C++ implementation
- **bucket_config.h** (13KB) - Bucket configuration presets
- **expansion_parameters.h** (7.8KB) - Expansion table parameters
- **memory_calculator.h** (17KB) - Memory usage calculator

### Support Libraries
- **tsl/** - Robin hood hash map headers

### Build & Test
- **build_production.sh** (2.2KB) - Production build script
- **test_worker.html** (16KB) - Browser test interface

## Tools (Git Tracked)

Test and measurement scripts for development:

```
tools/
├── test_worker_measurement.js          # 100-trial performance testing
├── test_desktop_ultra_opposite_full.js # DESKTOP_ULTRA standalone test
├── extract_11_results.js               # Log parser (11 configs)
├── merge_all_results.js                # Result merger
└── test_desktop_ultra_only.js          # Quick verification test
```

## Archive (Git Ignored)

Experimental results and detailed reports:

```
_archive/
├── logs/                  # Test execution logs (~44,000 lines)
│   ├── test_output.log
│   └── desktop_ultra_test.log
├── docs/                  # Detailed experiment reports
│   ├── Production_Implementation.md
│   └── performance_results_detailed.md
└── results/               # Intermediate result JSON
    ├── performance_results_complete.json
    ├── results_11_configs.json
    └── desktop_ultra_opposite_results.json
```

## Build Instructions	

```bash
# Release build (Production)
./build_production.sh

# Debug build (Development)
./build_production.sh debug
```

**Output**: `solver_prod.js`, `solver_prod.wasm`

## Usage Examples

### Worker Initialization
```javascript
// Using worker.js
const worker = new Worker('production/worker.js');

worker.postMessage({
  type: 'initSolver',
  adj: true,  // Adjacent pairs
  bucketModel: 'DESKTOP_STD'
});

worker.onmessage = (e) => {
  if (e.data.type === 'solverInitialized') {
    console.log('Ready!');
  }
};
```

### Scramble Generation

```javascript
worker.postMessage({
  type: 'generateScramble',
  adj: true,
  depth: 10
});

worker.onmessage = (e) => {
  if (e.data.type === 'scrambleGenerated') {
    console.log('Scramble:', e.data.scramble);
    console.log('Time:', e.data.timeMs, 'ms');
  }
};
```

## Performance Data

| Model | Init Time | Depth 10 Avg |
|-------|-----------|--------------|
| MOBILE_LOW | 10-12s | 12-14ms |
| DESKTOP_STD | 34-37s | 12-13ms |
| DESKTOP_ULTRA | 82-96s | 13-18ms |

**Details**: [../../docs/developer/performance_results.md](../../docs/developer/performance_results.md)

## Documentation

- **Implementation Plan**: [../../docs/developer/Production_Implementation.md](../../docs/developer/Production_Implementation.md)
- **Performance Results**: [../../docs/developer/performance_results.md](../../docs/developer/performance_results.md)
- **Detailed Experiment Reports**: [_archive/docs/performance_results_detailed.md](_archive/docs/performance_results_detailed.md)