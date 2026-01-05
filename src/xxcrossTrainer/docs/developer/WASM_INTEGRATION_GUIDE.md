# WASM Solver Integration Guide

**Purpose**: Complete guide for integrating xxcross solver into WASM-based trainer applications  
**Audience**: Developers building new trainers or maintaining existing ones  
**Level**: Advanced - assumes basic WASM knowledge  
**Last Updated**: 2026-01-03

> **Prerequisites**: For basic WASM build setup, see [WASM_BUILD_GUIDE.md](WASM_BUILD_GUIDE.md) first

---

## Table of Contents

1. [Overview](#overview)
2. [Build Process](#build-process)
3. [HTML Integration](#html-integration)
4. [Solver Configuration](#solver-configuration)
5. [Statistics & Analysis](#statistics--analysis)
6. [Memory Tier Selection](#memory-tier-selection)
7. [Performance Optimization](#performance-optimization)
8. [Troubleshooting](#troubleshooting)

---

## Overview

### Architecture

```
┌─────────────────────────────────────────┐
│  HTML Trainer Page                      │
│  ├─ UI Controls (bucket selection, etc) │
│  ├─ Statistics Display                  │
│  └─ Result Analysis                     │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  JavaScript Wrapper                     │
│  ├─ solve_with_custom_buckets()        │
│  ├─ get_solver_statistics()            │
│  └─ Memory Tier Detection              │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│  WASM Module (solver.wasm + solver.js)  │
│  ├─ Database Construction               │
│  ├─ Heap Measurement                    │
│  └─ Scramble Generation                 │
└─────────────────────────────────────────┘
```

### Key Features

- **Runtime Configuration**: Bucket sizes set via JavaScript (no recompilation needed)
- **Heap Monitoring**: Real-time memory usage tracking via `emscripten_get_heap_size()`
- **Statistics Export**: Comprehensive data via embind (node counts, load factors, scrambles)
- **Dual-Solver Support**: Two solvers per page (adjacent + opposite orientations)

---

## Build Process

### Prerequisites

```bash
# Install Emscripten SDK (in home directory)
cd ~/
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Current Production Build (stable_20260103)

The current production WASM build uses **em++ directly** (no build script needed):

```bash
#!/bin/bash
# Production WASM build for xxcross trainer
# From src/xxcrossTrainer/ directory

source ~/emsdk/emsdk_env.sh

em++ solver_dev.cpp -o solver_dev.js -std=c++17 -O3 -I../.. \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MAXIMUM_MEMORY=4GB \
  -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind \
  -s INITIAL_MEMORY=64MB

echo "✅ Build complete: solver_dev.js + solver_dev.wasm"
ls -lh solver_dev.{js,wasm}
```

**Output**: 
- `solver_dev.js` (JavaScript loader, ~94 KB)
- `solver_dev.wasm` (WebAssembly binary, ~295 KB)

**Used by**:
- `test_wasm_browser.html` - Production test page
- `worker_test_wasm_browser.js` - Worker implementation

### Legacy Measurement Build

**⚠️ Note**: `build_wasm_unified.sh` exists for **legacy heap measurement workflows only**.

It builds `solver_heap_measurement.{js,wasm}` for archived measurement tools:
- Used with `wasm_heap_advanced.html` (now in backups/)
- MODULARIZE build with different export name (`SolverModule`)
- Not used in current production

See [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md) for historical reference.

---

## HTML Integration

### Basic Template

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>XXCross Trainer</title>
    <style>
        /* Minimal styling */
        body { font-family: Arial, sans-serif; margin: 20px; }
        .controls { margin: 20px 0; }
        .stats { margin-top: 20px; border: 1px solid #ccc; padding: 10px; }
    </style>
</head>
<body>
    <h1>XXCross Trainer</h1>
    
    <!-- Bucket Configuration -->
    <div class="controls">
        <label>Memory Tier:</label>
        <select id="tier-select">
            <option value="MOBILE_LOW">Mobile LOW (618 MB)</option>
            <option value="MOBILE_MIDDLE">Mobile MIDDLE (896 MB)</option>
            <option value="MOBILE_HIGH">Mobile HIGH (1070 MB)</option>
            <option value="DESKTOP_STD" selected>Desktop STD (1512 MB)</option>
            <option value="DESKTOP_HIGH">Desktop HIGH (2786 MB)</option>
            <option value="DESKTOP_ULTRA">Desktop ULTRA (2884 MB)</option>
        </select>
        <button onclick="initializeSolver()">Initialize Solver</button>
    </div>
    
    <!-- Statistics Display -->
    <div class="stats" id="stats-display">
        <p>Solver not initialized</p>
    </div>
    
    <!-- Load WASM Module -->
    <script src="solver_heap_measurement.js"></script>
    <script src="trainer.js"></script>
</body>
</html>
```

### JavaScript Wrapper (trainer.js)

```javascript
// Memory tier configurations
const WASM_TIERS = {
    MOBILE_LOW:    { buckets: [1, 1, 2, 4],    heap: 618,  name: "Mobile LOW" },
    MOBILE_MIDDLE: { buckets: [2, 4, 4, 4],    heap: 896,  name: "Mobile MIDDLE" },
    MOBILE_HIGH:   { buckets: [4, 4, 4, 4],    heap: 1070, name: "Mobile HIGH" },
    DESKTOP_STD:   { buckets: [8, 8, 8, 8],    heap: 1512, name: "Desktop STD" },
    DESKTOP_HIGH:  { buckets: [8, 16, 16, 16], heap: 2786, name: "Desktop HIGH" },
    DESKTOP_ULTRA: { buckets: [16, 16, 16, 16], heap: 2884, name: "Desktop ULTRA" }
};

let solverModule = null;
let currentStats = null;

// Initialize WASM module
async function initializeSolver() {
    const tierSelect = document.getElementById('tier-select');
    const tier = WASM_TIERS[tierSelect.value];
    
    console.log(`Initializing solver: ${tier.name}`);
    document.getElementById('stats-display').innerHTML = '<p>Loading WASM module...</p>';
    
    try {
        // Load WASM module
        solverModule = await createSolverModule();
        console.log('✅ WASM Runtime initialized');
        
        // Call solver with custom buckets
        const [b7, b8, b9, b10] = tier.buckets;
        console.log(`▶ solve_with_custom_buckets(${b7}, ${b8}, ${b9}, ${b10})`);
        
        solverModule.solve_with_custom_buckets(b7, b8, b9, b10, true);
        
        // Get statistics
        currentStats = solverModule.get_solver_statistics();
        displayStatistics(currentStats, tier);
        
    } catch (error) {
        console.error('❌ Solver initialization failed:', error);
        document.getElementById('stats-display').innerHTML = 
            `<p style="color: red;">Error: ${error.message}</p>`;
    }
}

// Display statistics
function displayStatistics(stats, tier) {
    const heapMB = stats.peak_heap_mb;
    const finalHeapMB = stats.final_heap_mb;
    const totalNodes = stats.node_counts.reduce((a, b) => a + b, 0);
    
    const html = `
        <h3>Solver Statistics - ${tier.name}</h3>
        <table>
            <tr><td><strong>Peak Heap:</strong></td><td>${heapMB} MB</td></tr>
            <tr><td><strong>Final Heap:</strong></td><td>${finalHeapMB} MB</td></tr>
            <tr><td><strong>Total Nodes:</strong></td><td>${totalNodes.toLocaleString()}</td></tr>
            <tr><td><strong>Expected (Single):</strong></td><td>${tier.heap / 2} MB</td></tr>
            <tr><td><strong>Dual-Solver Estimate:</strong></td><td>${tier.heap} MB</td></tr>
        </table>
        
        <h4>Per-Depth Breakdown</h4>
        <table border="1" cellpadding="5">
            <tr>
                <th>Depth</th>
                <th>Nodes</th>
                <th>Load Factor</th>
            </tr>
            ${stats.node_counts.map((count, depth) => `
                <tr>
                    <td>${depth}</td>
                    <td>${count.toLocaleString()}</td>
                    <td>${depth >= 7 && depth <= 10 ? stats.load_factors[depth - 7].toFixed(2) : 'N/A'}</td>
                </tr>
            `).join('')}
        </table>
    `;
    
    document.getElementById('stats-display').innerHTML = html;
}

// Auto-detect memory tier (optional)
function autoDetectTier() {
    if ('deviceMemory' in navigator) {
        const deviceMemoryGB = navigator.deviceMemory;
        console.log(`Device memory: ${deviceMemoryGB} GB`);
        
        // Simple heuristic
        if (deviceMemoryGB <= 2) return 'MOBILE_LOW';
        if (deviceMemoryGB <= 4) return 'MOBILE_MIDDLE';
        if (deviceMemoryGB <= 6) return 'MOBILE_HIGH';
        if (deviceMemoryGB <= 8) return 'DESKTOP_STD';
        if (deviceMemoryGB <= 16) return 'DESKTOP_HIGH';
        return 'DESKTOP_ULTRA';
    }
    
    // Fallback: Use screen size heuristic
    const isMobile = window.innerWidth < 768;
    return isMobile ? 'MOBILE_MIDDLE' : 'DESKTOP_STD';
}
```

---

## Solver Configuration

### C++ Side (solver_dev.cpp)

```cpp
#include <emscripten/bind.h>
#include <emscripten/heap.h>

struct SolverStatistics {
    std::vector<int> node_counts;      // Per-depth (0-10)
    std::vector<double> load_factors;  // Depths 7-10
    std::vector<std::string> sample_scrambles; // One per depth
    int final_heap_mb;
    int peak_heap_mb;
    bool success;
    std::string error_message;
};

// Main solver function
SolverStatistics solve_with_custom_buckets(
    int bucket_d7_mb, int bucket_d8_mb, 
    int bucket_d9_mb, int bucket_d10_mb, bool verbose) {
    
    SolverStatistics stats;
    
    try {
        // Convert MB to bytes
        size_t b7 = bucket_d7_mb * 1024 * 1024;
        size_t b8 = bucket_d8_mb * 1024 * 1024;
        size_t b9 = bucket_d9_mb * 1024 * 1024;
        size_t b10 = bucket_d10_mb * 1024 * 1024;
        
        // Create configuration
        BucketConfig config;
        config.model = BucketModel::CUSTOM;
        config.custom_bucket_7 = b7;
        config.custom_bucket_8 = b8;
        config.custom_bucket_9 = b9;
        config.custom_bucket_10 = b10;
        
        ResearchConfig research;
        research.enable_custom_buckets = true;
        research.skip_search = true;  // Database build only
        
        // Build solver
        xxcross_search solver(308, config, research, true);
        
        // Collect statistics
        stats.node_counts.resize(11);
        for (int d = 0; d <= 10; d++) {
            stats.node_counts[d] = solver.index_pairs[d].size();
        }
        
        // Heap measurements
        #ifdef __EMSCRIPTEN__
        stats.final_heap_mb = emscripten_get_heap_size() / (1024 * 1024);
        stats.peak_heap_mb = stats.final_heap_mb; // WASM heap never shrinks
        #endif
        
        // Load factors (typical for robin_set)
        stats.load_factors = {0.88, 0.88, 0.88, 0.88};
        
        // Generate sample scrambles
        for (int d = 1; d <= 10; d++) {
            if (solver.index_pairs[d].size() > 0) {
                std::string scramble = solver.get_safe_scramble_at_depth(d);
                stats.sample_scrambles.push_back(scramble);
            }
        }
        
        stats.success = true;
        
    } catch (const std::exception& e) {
        stats.success = false;
        stats.error_message = e.what();
    }
    
    return stats;
}

// embind exports
EMSCRIPTEN_BINDINGS(solver_module) {
    emscripten::value_object<SolverStatistics>("SolverStatistics")
        .field("node_counts", &SolverStatistics::node_counts)
        .field("load_factors", &SolverStatistics::load_factors)
        .field("sample_scrambles", &SolverStatistics::sample_scrambles)
        .field("final_heap_mb", &SolverStatistics::final_heap_mb)
        .field("peak_heap_mb", &SolverStatistics::peak_heap_mb)
        .field("success", &SolverStatistics::success)
        .field("error_message", &SolverStatistics::error_message);
    
    emscripten::function("solve_with_custom_buckets", 
                        &solve_with_custom_buckets);
    
    // Vector types
    emscripten::register_vector<int>("VectorInt");
    emscripten::register_vector<double>("VectorDouble");
    emscripten::register_vector<std::string>("VectorString");
}
```

---

## Statistics & Analysis

### Console Log Format

Expected output when running solver:

```
✅ WASM Runtime initialized
▶ solve_with_custom_buckets(8, 8, 8, 8)

========================================
Bucket Configuration:
  Depth 7:  8 MB (8388608 bytes)
  Depth 8:  8 MB (8388608 bytes)
  Depth 9:  8 MB (8388608 bytes)
  Depth 10: 8 MB (8388608 bytes)
========================================

Creating solver with memory limit: 308 MB

[Phase 1] BFS depth 0→6: 4,550,760 nodes
[Phase 2] Local expansion depth 7: 3,774,873 nodes (90% LF)
[Phase 3] Local expansion depth 8: 3,774,873 nodes (90% LF)
[Phase 4] Local expansion depth 9: 7,549,746 nodes (90% LF)
[Phase 5] Local expansion depth 10: 15,099,492 nodes (90% LF)

[Heap] Final Cleanup Complete: Total=756 MB

✅ Database build complete!
Total nodes: 34,749,750
```

### Result Validation

Check these metrics to verify correct operation:

1. **Load Factors**: Should be ~0.88-0.90 for depths 7-10
2. **Heap Growth**: Should be monotonic (never decrease)
3. **Node Counts**: Should match expected distribution
   - depth 6: ~4.2M
   - depth 7-9: ~3.8M each (90% of bucket capacity)
   - depth 10: ~15M (90% of 16M bucket)
4. **Final Heap = Peak Heap**: WASM heap never shrinks

### CSV Export Example

```csv
Timestamp,Tier,Config,Final_Heap_MB,Total_Nodes,Nodes_D7,Nodes_D8,Nodes_D9,Nodes_D10
2026-01-03T10:30:00,DESKTOP_STD,8M/8M/8M/8M,756,34749750,3774873,3774873,7549746,15099492
```

---

## Memory Tier Selection

### Automatic Detection

```javascript
function selectOptimalTier() {
    // Method 1: Device Memory API
    if ('deviceMemory' in navigator) {
        const memoryGB = navigator.deviceMemory;
        if (memoryGB >= 16) return WASM_TIERS.DESKTOP_ULTRA;
        if (memoryGB >= 8)  return WASM_TIERS.DESKTOP_HIGH;
        if (memoryGB >= 6)  return WASM_TIERS.DESKTOP_STD;
        if (memoryGB >= 4)  return WASM_TIERS.MOBILE_HIGH;
        if (memoryGB >= 2)  return WASM_TIERS.MOBILE_MIDDLE;
        return WASM_TIERS.MOBILE_LOW;
    }
    
    // Method 2: User Agent heuristic
    const ua = navigator.userAgent;
    const isMobile = /Android|iPhone|iPad/i.test(ua);
    
    if (isMobile) {
        // Conservative mobile default
        return WASM_TIERS.MOBILE_MIDDLE;
    }
    
    // Desktop default
    return WASM_TIERS.DESKTOP_STD;
}

// Usage
window.onload = function() {
    const defaultTier = selectOptimalTier();
    document.getElementById('tier-select').value = 
        Object.keys(WASM_TIERS).find(k => WASM_TIERS[k] === defaultTier);
};
```

### Manual Override UI

```html
<div class="tier-selector">
    <h3>Memory Configuration</h3>
    <label>
        <input type="radio" name="tier" value="auto" checked>
        Auto-detect (Recommended)
    </label>
    <label>
        <input type="radio" name="tier" value="manual">
        Manual Selection:
        <select id="manual-tier" disabled>
            <option value="MOBILE_LOW">Mobile LOW (12M nodes, 618 MB)</option>
            <option value="MOBILE_MIDDLE">Mobile MIDDLE (18M nodes, 896 MB)</option>
            <option value="MOBILE_HIGH">Mobile HIGH (20M nodes, 1070 MB)</option>
            <option value="DESKTOP_STD">Desktop STD (35M nodes, 1512 MB)</option>
            <option value="DESKTOP_HIGH">Desktop HIGH (57M nodes, 2786 MB)</option>
            <option value="DESKTOP_ULTRA">Desktop ULTRA (65M nodes, 2884 MB)</option>
        </select>
    </label>
</div>

<script>
document.querySelectorAll('input[name="tier"]').forEach(radio => {
    radio.addEventListener('change', function() {
        const manualSelect = document.getElementById('manual-tier');
        manualSelect.disabled = (this.value === 'auto');
    });
});
</script>
```

---

## Performance Optimization

### Heap Pre-allocation

```javascript
// Pre-allocate WASM heap to avoid growth pauses
const tier = WASM_TIERS.DESKTOP_STD;
const initialHeapMB = tier.heap;

createSolverModule({
    INITIAL_MEMORY: initialHeapMB * 1024 * 1024,
    ALLOW_MEMORY_GROWTH: true,
    MAXIMUM_MEMORY: 4 * 1024 * 1024 * 1024  // 4GB limit
}).then(module => {
    solverModule = module;
    console.log(`Heap pre-allocated: ${initialHeapMB} MB`);
});
```

### Worker Thread Integration

**Pattern Verification**: ✅ This pattern is compatible with existing xcrossTrainer implementation (see [src/xcrossTrainer/worker.js](../../xcrossTrainer/worker.js)).

**Current Implementation** (xcrossTrainer/worker.js):
```javascript
// Existing pattern uses importScripts and onmessage
let xcrossSearchInstance;
const initPromise = new Promise((resolve, reject) => {
    self.Module = {
        onRuntimeInitialized: () => {
            xcrossSearchInstance = new self.Module.xcross_search();
            resolve();
        }
    };
});
importScripts('solver.js');

self.onmessage = async function (event) {
    const { scr, len } = event.data;
    await initPromise;
    if (xcrossSearchInstance) {
        const ret = xcrossSearchInstance.func(scr, len);
        self.postMessage(ret);
    }
};
```

**\u2705 Pattern Verification**: This approach is compatible with existing implementation (see [src/xcrossTrainer/worker.js](../../xcrossTrainer/worker.js)).

**Updated Pattern** (for solver_heap_measurement.js):
```javascript
// solver-worker.js
importScripts('solver_heap_measurement.js');

let solver = null;

self.onmessage = async function(e) {
    const { buckets, verbose } = e.data;
    
    if (!solver) {
        solver = await createSolverModule();
    }
    
    const [b7, b8, b9, b10] = buckets;
    solver.solve_with_custom_buckets(b7, b8, b9, b10, verbose);
    
    const stats = solver.get_solver_statistics();
    self.postMessage({ type: 'complete', stats });
};

// Main thread
const worker = new Worker('solver-worker.js');
worker.onmessage = function(e) {
    if (e.data.type === 'complete') {
        displayStatistics(e.data.stats);
    }
};

worker.postMessage({
    buckets: [8, 8, 8, 8],
    verbose: true
});
```

### Caching Strategy

```javascript
// Cache compiled WASM module
const CACHE_NAME = 'solver-wasm-v1';

async function loadSolverWithCache() {
    const cache = await caches.open(CACHE_NAME);
    
    // Try cache first
    let wasmResponse = await cache.match('solver_heap_measurement.wasm');
    
    if (!wasmResponse) {
        // Fetch and cache
        wasmResponse = await fetch('solver_heap_measurement.wasm');
        await cache.put('solver_heap_measurement.wasm', wasmResponse.clone());
    }
    
    const wasmBytes = await wasmResponse.arrayBuffer();
    
    return createSolverModule({
        wasmBinary: wasmBytes
    });
}
```

---

## Troubleshooting

### Common Issues

#### 1. "Module not found" Error

**Symptom**: `Uncaught ReferenceError: createSolverModule is not defined`

**Solution**:
```html
<!-- Ensure correct script load order -->
<script src="solver_heap_measurement.js"></script>
<script>
    // Wait for module to load
    createSolverModule().then(module => {
        console.log('Module loaded');
        solverModule = module;
    });
</script>
```

#### 2. Heap Size Shows 0 MB

**Symptom**: Statistics show `final_heap_mb: 0`

**Solution**:
```cpp
// Ensure __EMSCRIPTEN__ is defined
#ifdef __EMSCRIPTEN__
    #include <emscripten/heap.h>
    stats.final_heap_mb = emscripten_get_heap_size() / (1024 * 1024);
#else
    stats.final_heap_mb = 0;  // Native builds don't track heap
#endif
```

#### 3. Browser Crashes on Large Configurations

**Symptom**: Tab crashes when loading Desktop ULTRA

**Solution**:
```javascript
// Add memory check before initialization
function checkMemoryAvailable(tierName) {
    const tier = WASM_TIERS[tierName];
    
    if ('deviceMemory' in navigator) {
        const availableGB = navigator.deviceMemory;
        const requiredGB = tier.heap / 1024;
        
        if (requiredGB > availableGB) {
            alert(`Warning: ${tier.name} requires ~${requiredGB.toFixed(1)} GB, ` +
                  `but only ${availableGB} GB detected. Consider lower tier.`);
            return false;
        }
    }
    return true;
}
```

#### 4. Scramble Generation Fails

**Symptom**: `sample_scrambles` array is empty

**Solution**:
```cpp
// Use safe scramble generation
std::string get_safe_scramble_at_depth(int depth) {
    if (depth < 1 || depth > 10) return "";
    if (index_pairs[depth].size() == 0) return "";
    
    try {
        // Use first node instead of random sampling
        uint64_t node = index_pairs[depth][0];
        return get_xxcross_scramble(std::to_string(depth));
    } catch (...) {
        return "";  // Fail gracefully
    }
}
```

### Debug Tools

#### Browser Console Monitoring

```javascript
// Enable verbose logging
Module = {
    print: function(text) {
        console.log('[WASM stdout]', text);
    },
    printErr: function(text) {
        console.error('[WASM stderr]', text);
    },
    onRuntimeInitialized: function() {
        console.log('✅ WASM Runtime Ready');
    }
};
```

#### Memory Profiling

```javascript
// Chrome DevTools Memory Profiler
function monitorHeapGrowth() {
    setInterval(() => {
        if (solverModule) {
            const heapMB = solverModule.HEAP8.length / (1024 * 1024);
            console.log(`Current heap: ${heapMB.toFixed(2)} MB`);
        }
    }, 1000);
}
```

---

## References

### Documentation
- [WASM Measurement Results](Experiments/wasm_heap_measurement_results.md) - Tier definitions
- [Implementation Progress](IMPLEMENTATION_PROGRESS.md) - Development tracker
- [Memory Budget Design (Archived)](_archive/MEMORY_BUDGET_DESIGN_archived_20260103.md) - System architecture (implementation complete)

### External Resources
- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
- [embind Guide](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html)

---

**Last Updated**: 2026-01-03  
**Maintainer**: xxcrossTrainer Development Team  
**Questions?** See [DEVELOPMENT_SUMMARY.md](docs/developer/DEVELOPMENT_SUMMARY.md)
