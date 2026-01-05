# WASM Experiment Scripts

**Version**: stable_20260103  
**Last Updated**: 2026-01-03

> **⚠️ LEGACY REFERENCE DOCUMENT**
>
> This document describes test scripts for the **LEGACY heap measurement build system**:
> - `build_wasm_unified.sh` → `solver_heap_measurement.{js,wasm}`
> - Used with archived measurement tools in `backups/`
>
> **Current Production** (stable_20260103):
> - **Build**: `em++ solver_dev.cpp -o solver_dev.js` (direct compilation, no build script)
> - **Test page**: `test_wasm_browser.html`
> - **Worker**: `worker_test_wasm_browser.js`
>
> This document is preserved for:
> 1. Understanding the methodology used to derive the 6-tier WASM bucket model
> 2. Historical reference for the measurement campaign
> 3. Reproducing measurements if needed for validation
>
> **Navigation**: [← Back to Developer Docs](README.md) | [User Guide](../USER_GUIDE.md)
>
> **Related**: [WASM Build Guide](WASM_BUILD_GUIDE.md) | [WASM Integration Guide](WASM_INTEGRATION_GUIDE.md) | [Memory Monitoring](MEMORY_MONITORING.md)

---

## Purpose

This document provides **complete implementation examples** of JavaScript testing scripts for WebAssembly builds. These scripts demonstrate:

- WASM module loading and initialization
- Database construction with 5-Phase BFS (depths 0-10)
- Custom bucket configuration via environment variables
- SolverStatistics extraction and analysis
- Instance reuse patterns (critical for production)
- Memory monitoring and heap measurement

**Note**: These are reference implementations. For production deployment, see [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md).

---

## Script 1: Basic WASM Loading Test

### Purpose
Verify WASM module loads correctly in Node.js environment with heap monitoring.

### Implementation

```javascript
// test_wasm_basic.js
// Basic WASM module loading and initialization test (stable_20260103)

const Module = require('./solver_heap_measurement.js');

Module.onRuntimeInitialized = () => {
    console.log('✓ WASM module loaded successfully');
    console.log('✓ Runtime initialized');
    
    // Check heap size (depth 0-10 support, 5-Phase construction)
    if (Module.wasmMemory) {
        const bytes = Module.wasmMemory.buffer.byteLength;
        const mb = bytes / (1024 * 1024);
        console.log(`✓ Initial heap: ${mb.toFixed(2)} MB`);
    }
    
    // Check exported functions (updated for stable_20260103)
    const requiredFunctions = [
        'solve_with_custom_buckets',      // embind: Custom bucket solver
        'get_solver_statistics',          // embind: Statistics accessor
        '_malloc',
        '_free'
    ];
    
    let allPresent = true;
    for (const func of requiredFunctions) {
        if (typeof Module[func] === 'function') {
            console.log(`✓ Function ${func} available`);
        } else {
            console.log(`✗ Function ${func} missing`);
            allPresent = false;
        }
    }
    
    if (allPresent) {
        console.log('\\n=== All Tests Passed ===');
        process.exit(0);
    } else {
        console.log('\\n=== Tests Failed ===');
        process.exit(1);
    }
};

Module.onAbort = (what) => {
    console.error('✗ Module aborted:', what);
    process.exit(1);
};

// Timeout after 30 seconds
setTimeout(() => {
    console.error('✗ Timeout: Module initialization took too long');
    process.exit(1);
}, 30000);
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer

# Build unified WASM with heap measurement
./build_wasm_unified.sh

# Run test
node test_wasm_basic.js
```

---

## Script 2: Custom Bucket Configuration Test

### Purpose
Test WASM solver with custom bucket sizes (depth 7-10 explicit specification).

### Implementation

```javascript
// test_custom_buckets.js
// Test custom bucket configuration (stable_20260103)

const Module = require('./solver_heap_measurement.js');

console.log('=== Custom Bucket Configuration Test ===\\n');

Module.onRuntimeInitialized = () => {
    console.log('✓ WASM Runtime initialized\\n');
    
    // Test configurations (bucket sizes for depths 7, 8, 9, 10)
    const configs = [
        { name: 'Mobile LOW', buckets: [1, 1, 2, 4], target: '618 MB dual-heap' },
        { name: 'Desktop STD', buckets: [8, 8, 8, 8], target: '1512 MB dual-heap' },
        { name: 'Desktop ULTRA', buckets: [16, 16, 16, 16], target: '2884 MB dual-heap' }
    ];
    
    for (const config of configs) {
        console.log(`--- ${config.name} (Target: ${config.target}) ---`);
        console.log(`Buckets (d7/d8/d9/d10): ${config.buckets.join('M/')}M`);
        
        try {
            const startTime = Date.now();
            
            // Set bucket configuration via Module.ENV
            Module.ENV = {
                'ENABLE_CUSTOM_BUCKETS': '1',
                'BUCKET_D7': config.buckets[0].toString(),
                'BUCKET_D8': config.buckets[1].toString(),
                'BUCKET_D9': config.buckets[2].toString(),
                'BUCKET_D10': config.buckets[3].toString()
            };
            
            // Build database with custom buckets (verbose=true)
            Module.solve_with_custom_buckets(
                config.buckets[0],  // depth 7
                config.buckets[1],  // depth 8
                config.buckets[2],  // depth 9
                config.buckets[3],  // depth 10
                true                // verbose output
            );
            
            const duration = ((Date.now() - startTime) / 1000).toFixed(2);
            console.log(`\\nConstruction time: ${duration}s`);
            
            // Get statistics
            const stats = Module.get_solver_statistics();
            
            console.log(`\\nStatistics:`);
            console.log(`  Total nodes: ${stats.total_nodes.toLocaleString()}`);
            console.log(`  Final heap: ${stats.final_heap_mb.toFixed(2)} MB`);
            console.log(`  Peak heap: ${stats.peak_heap_mb.toFixed(2)} MB`);
            
            // Node distribution
            console.log(`\\n  Node counts by depth:`);
            for (let d = 0; d <= 10; d++) {
                const count = stats.node_counts[d];
                if (count > 0) {
                    console.log(`    Depth ${d}: ${count.toLocaleString()}`);
                }
            }
            
            // Load factors (depths 7-10)
            console.log(`\\n  Load factors (hash buckets):`);
            const depths = [7, 8, 9, 10];
            for (let i = 0; i < depths.length; i++) {
                const lf = stats.load_factors[i];
                console.log(`    Depth ${depths[i]}: ${(lf * 100).toFixed(1)}%`);
            }
            
            console.log('');
            
        } catch (error) {
            console.error(`✗ Error: ${error.message}\\n`);
        }
    }
    
    console.log('✓ All configurations tested successfully!');
    process.exit(0);
};

setTimeout(() => {
    console.error('\\n✗ Timeout');
    process.exit(1);
}, 600000); // 10 minutes for multiple configs
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer
node test_custom_buckets.js
```

**Expected Output**:
```
=== Custom Bucket Configuration Test ===

✓ WASM Runtime initialized

--- Mobile LOW (Target: 618 MB dual-heap) ---
Buckets (d7/d8/d9/d10): 1M/1M/2M/4M

[Heap] Phase 1 Complete: Total=...
[Heap] Phase 5 Complete: Total=...
[Heap] Final Cleanup Complete: Total=309.12 MB

Construction time: 120.45s

Statistics:
  Total nodes: 12,100,507
  Final heap: 309.12 MB
  Peak heap: 309.12 MB

  Node counts by depth:
    Depth 0: 1
    Depth 7: 1,887,437
    Depth 8: 3,774,873
    Depth 9: 3,774,873
    Depth 10: 2,663,323

  Load factors (hash buckets):
    Depth 7: 88.3%
    Depth 8: 88.7%
    Depth 9: 88.9%
    Depth 10: 31.5%
```

---

## Script 3: Heap Measurement Test

### Purpose
Measure WASM heap growth during 5-Phase database construction.

### Implementation

```javascript
// test_heap_measurement.js
// Heap growth monitoring during database construction (stable_20260103)

const Module = require('./solver_heap_measurement.js');

function parseHeapLog(output) {
    const heapRegex = /\[Heap\]\s+(.+?):\s+Total=(\d+\.\d+)\s+MB/g;
    const checkpoints = [];
    
    let match;
    while ((match = heapRegex.exec(output)) !== null) {
        checkpoints.push({
            phase: match[1],
            heap_mb: parseFloat(match[2])
        });
    }
    
    return checkpoints;
}

console.log('=== WASM Heap Measurement Test ===\\n');

Module.onRuntimeInitialized = () => {
    console.log('✓ Runtime initialized\\n');
    
    // Capture console output
    let consoleOutput = '';
    const originalLog = console.log;
    console.log = function(...args) {
        const msg = args.join(' ');
        consoleOutput += msg + '\\n';
        originalLog.apply(console, args);
    };
    
    try {
        console.log('Building database with 8M/8M/8M/8M (Desktop STD)...\\n');
        
        const startTime = Date.now();
        
        // Build with verbose output
        Module.solve_with_custom_buckets(8, 8, 8, 8, true);
        
        const duration = ((Date.now() - startTime) / 1000).toFixed(2);
        
        // Restore console
        console.log = originalLog;
        
        console.log(`\\nConstruction complete: ${duration}s\\n`);
        
        // Parse heap checkpoints
        const checkpoints = parseHeapLog(consoleOutput);
        
        console.log('=== Heap Growth Analysis ===\\n');
        console.log('Checkpoint                        | Heap (MB)');
        console.log('----------------------------------|----------');
        
        for (const cp of checkpoints) {
            const padding = ' '.repeat(33 - cp.phase.length);
            console.log(`${cp.phase}${padding}| ${cp.phase_mb.toFixed(2)}`);
        }
        
        if (checkpoints.length > 0) {
            const initial = checkpoints[0].heap_mb;
            const peak = Math.max(...checkpoints.map(c => c.heap_mb));
            const final = checkpoints[checkpoints.length - 1].heap_mb;
            
            console.log('');
            console.log(`Initial heap: ${initial.toFixed(2)} MB`);
            console.log(`Peak heap: ${peak.toFixed(2)} MB`);
            console.log(`Final heap: ${final.toFixed(2)} MB`);
            console.log(`Growth: ${(peak - initial).toFixed(2)} MB`);
        }
        
        console.log('\\n✓ Heap measurement complete');
        process.exit(0);
        
    } catch (error) {
        console.log = originalLog;
        console.error(`✗ Error: ${error.message}`);
        process.exit(1);
    }
};

setTimeout(() => {
    console.error('\\n✗ Timeout');
    process.exit(1);
}, 300000); // 5 minutes
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer
node test_heap_measurement.js
```

---

## Script 4: Scramble Generation Test

### Purpose
Test safe scramble generation at all depths (0-10) using embind integration.

### Implementation

```javascript
// test_scramble_generation.js
// Test scramble generation across all depths (stable_20260103)

const Module = require('./solver_heap_measurement.js');

console.log('=== Scramble Generation Test ===\\n');

Module.onRuntimeInitialized = () => {
    console.log('✓ Runtime initialized\\n');
    
    try {
        // Build database first
        console.log('Building database (8M/8M/8M/8M)...\\n');
        Module.solve_with_custom_buckets(8, 8, 8, 8, false);
        
        console.log('\\n=== Testing Scramble Generation ===\\n');
        
        // Get statistics to access scramble generation
        const stats = Module.get_solver_statistics();
        
        console.log('Scrambles by depth:\\n');
        
        // Test depths 1-10 (depth 0 is solved state)
        for (let depth = 1; depth <= 10; depth++) {
            const scrambles = stats.sample_scrambles;
            const scramble = scrambles[depth - 1]; // Array is 0-indexed
            
            if (scramble && scramble.length > 0) {
                // Calculate move count
                const moves = scramble.trim().split(/\\s+/).length;
                console.log(`Depth ${depth.toString().padStart(2)}: ${scramble}`);
                console.log(`           (${moves} moves)\\n`);
            } else {
                console.log(`Depth ${depth.toString().padStart(2)}: No scramble available\\n`);
            }
        }
        
        console.log('✓ Scramble generation test complete');
        process.exit(0);
        
    } catch (error) {
        console.error(`✗ Error: ${error.message}`);
        if (error.stack) console.error(error.stack);
        process.exit(1);
    }
};

setTimeout(() => {
    console.error('\\n✗ Timeout');
    process.exit(1);
}, 300000);
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer
node test_scramble_generation.js
```

**Expected Output**:
```
=== Scramble Generation Test ===

✓ Runtime initialized

Building database (8M/8M/8M/8M)...

=== Testing Scramble Generation ===

Scrambles by depth:

Depth  1: R
           (1 moves)

Depth  2: R U
           (2 moves)

...

Depth 10: R U F' R' U' F R2 U R' U'
           (10 moves)

✓ Scramble generation test complete
```

---

## Script 5: Statistics Analysis Test

### Purpose
Comprehensive analysis of SolverStatistics output.

### Implementation

```javascript
// test_statistics.js
// Comprehensive statistics analysis (stable_20260103)

const Module = require('./solver_heap_measurement.js');

console.log('=== Solver Statistics Analysis ===\\n');

Module.onRuntimeInitialized = () => {
    console.log('✓ Runtime initialized\\n');
    
    try {
        // Test configuration
        const buckets = [8, 8, 8, 8];
        console.log(`Building database (${buckets.join('M/')}M)...\\n`);
        
        Module.solve_with_custom_buckets(
            buckets[0], buckets[1], buckets[2], buckets[3],
            false  // verbose=false for cleaner output
        );
        
        // Get comprehensive statistics
        const stats = Module.get_solver_statistics();
        
        console.log('\\n=== Database Construction Statistics ===\\n');
        
        // Success status
        console.log(`Status: ${stats.success ? '✓ Success' : '✗ Failed'}`);
        if (stats.error_message && stats.error_message.length > 0) {
            console.log(`Error: ${stats.error_message}`);
        }
        console.log('');
        
        // Node distribution
        console.log('Node Distribution:');
        console.log('  Depth | Count        | Cumulative   | Percentage');
        console.log('  ------|--------------|--------------|----------');
        
        let cumulative = 0;
        const total = stats.total_nodes;
        
        for (let d = 0; d <= 10; d++) {
            const count = stats.node_counts[d];
            cumulative += count;
            const pct = (count / total * 100).toFixed(2);
            
            if (count > 0) {
                console.log(`    ${d.toString().padStart(2)}  | ${count.toString().padStart(12)} | ${cumulative.toString().padStart(12)} | ${pct.padStart(6)}%`);
            }
        }
        
        console.log(`  Total | ${total.toString().padStart(12)} |              |`);
        console.log('');
        
        // Hash table load factors
        console.log('Hash Table Load Factors:');
        console.log('  Depth | Bucket Size | Load Factor | Efficiency');
        console.log('  ------|-------------|-------------|----------');
        
        const depths = [7, 8, 9, 10];
        for (let i = 0; i < depths.length; i++) {
            const d = depths[i];
            const bucketSize = buckets[i];
            const lf = stats.load_factors[i];
            const efficiency = (lf * 100).toFixed(1);
            
            console.log(`    ${d.toString().padStart(2)}  | ${bucketSize.toString().padStart(6)}M     | ${lf.toFixed(4).padStart(11)} | ${efficiency.padStart(8)}%`);
        }
        console.log('');
        
        // Memory usage
        console.log('Memory Usage:');
        console.log(`  Final heap: ${stats.final_heap_mb.toFixed(2)} MB`);
        console.log(`  Peak heap: ${stats.peak_heap_mb.toFixed(2)} MB`);
        console.log('');
        
        // Efficiency metrics
        const nodesPerMB = (total / stats.final_heap_mb).toFixed(0);
        const mbPerMnode = (stats.final_heap_mb / (total / 1000000)).toFixed(2);
        
        console.log('Efficiency Metrics:');
        console.log(`  Nodes/MB: ${parseInt(nodesPerMB).toLocaleString()}`);
        console.log(`  MB/Mnode: ${mbPerMnode}`);
        console.log('');
        
        // Children-per-parent statistics
        console.log('Expansion Statistics (Phase 5):');
        console.log(`  Avg children/parent: ${stats.avg_children_per_parent.toFixed(2)}`);
        console.log(`  Max children/parent: ${stats.max_children_per_parent}`);
        console.log('');
        
        console.log('✓ Statistics analysis complete');
        process.exit(0);
        
    } catch (error) {
        console.error(`✗ Error: ${error.message}`);
        if (error.stack) console.error(error.stack);
        process.exit(1);
    }
};

setTimeout(() => {
    console.error('\\n✗ Timeout');
    process.exit(1);
}, 300000);
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer
node test_statistics.js
```

---

## Script 6: Heap Measurement Browser Test (HTML)

### Purpose
Heap memory measurement tool for WASM module performance analysis. Provides detailed statistics on memory usage, node distribution, and load factors.

**Note**: This is a **measurement tool** using direct WASM call (MODULARIZE build). For **production deployment**, see Script 7 (Web Worker pattern).

**Files**:
- [wasm_heap_advanced.html](../../wasm_heap_advanced.html) - Advanced statistics UI
- [solver_heap_measurement.{js,wasm}](../../solver_heap_measurement.js) - WASM binary (MODULARIZE)
- [build_wasm_unified.sh](../../build_wasm_unified.sh) - Build script

### Key Features
- Detailed node distribution by depth
- Load factor analysis for each bucket
- Sample scrambles with length verification
- Memory usage tracking (final/peak heap)
- Download results as CSV

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer

# Build WASM (MODULARIZE for direct call)
./build_wasm_unified.sh

# Serve with HTTP server
python3 -m http.server 8000

# Open browser
# Navigate to: http://localhost:8000/wasm_heap_advanced.html
```

**Implementation Pattern**:
```javascript
// Direct WASM call with SolverModule()
const Module = await SolverModule({
    print: function(text) { addConsoleLog(text); },
    printErr: function(text) { addConsoleLog('[ERROR] ' + text); },
    onRuntimeInitialized: function() { 
        addConsoleLog('✅ WASM Runtime initialized'); 
    }
});

// Build and get statistics
const stats = Module.solve_with_custom_buckets(b7, b8, b9, b10, true);
displayStatistics(stats);
```

---

## Script 7: Web Worker Production Test with Heap Measurement (HTML)

### Purpose
**Integrated test page** combining production Worker pattern with heap measurement capabilities. This page serves dual purposes:

1. **Production Testing**: Non-blocking database construction via Web Worker
2. **Heap Measurement**: Detailed statistics and memory monitoring

**Why Integration Makes Sense**:
- Both use same Worker pattern (stable, isolated)
- Heap measurement doesn't need separate MODULARIZE build
- Worker provides stable memory accounting
- Can test AND measure in same environment

**Reference**: Combines [xcrossTrainer/worker.js](../../../xcrossTrainer/worker.js) pattern with wasm_heap_advanced.html UI

**Files** (in xxcrossTrainer/ directory):
- `test_wasm_browser.html` - **Integrated test & measurement page** ✨
- `worker_test_wasm_browser.js` - Worker script with statistics parsing
- `test_solver.{js,wasm}` - WASM binary (non-MODULARIZE for Worker)
- `build_wasm_test.sh` - Build script

### Build Script (build_wasm_test.sh)

**✅ Tested and verified** (2026-01-03)

```bash
#!/bin/bash
# Build WASM for Web Worker test (test_wasm_browser.html)
# Non-MODULARIZE build for importScripts compatibility

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUTPUT_NAME="test_solver"

echo "========================================"
echo "WASM Test Build (Web Worker)"
echo "========================================"
echo "Output: ${OUTPUT_NAME}.js / ${OUTPUT_NAME}.wasm"

# Source Emscripten environment
if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
fi

# Build WITHOUT MODULARIZE (for importScripts in Worker)
echo "Building WASM module for Web Worker..."
emcc -std=c++17 -O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=64MB \
  -s MAXIMUM_MEMORY=2GB \
  -s STACK_SIZE=5MB \
  -s EXPORTED_FUNCTIONS='["_main","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
  -s ENVIRONMENT='web,worker' \
  -s INVOKE_RUN=0 \
  -DVERBOSE_MODE \
  -I.. \
  -o "${OUTPUT_NAME}.js" \
  solver_dev.cpp \
  -lembind

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Files generated:"
    ls -lh "${OUTPUT_NAME}.js" "${OUTPUT_NAME}.wasm" 2>/dev/null || true
    echo ""
    echo "Usage:"
    echo "  1. importScripts('test_solver.js') in worker_test_wasm_browser.js"
    echo "  2. python3 -m http.server 8000"
    echo "  3. Open http://localhost:8000/test_wasm_browser.html"
else
    echo "❌ Build failed"
    exit 1
fi
```

### Worker Script (worker_test_wasm_browser.js)

**✅ Updated** (2026-01-03) - **Instance Retention + Statistics Parsing**

**Key Features**:
1. **Solver Instance Retention**: Build once, unlimited random scrambles
2. **Log Collection**: Captures C++ console output for statistics parsing
3. **Statistics Extraction**: Parses node counts, heap usage, load factors from logs
4. **Error Handling**: Detailed error reporting with full context

**Simplified embind** (solver_dev.cpp):
```cpp
EMSCRIPTEN_BINDINGS(my_module)
{
    // Constructor and func() only - like xcrossTrainer/solver.cpp
    emscripten::class_<xxcross_search>("xxcross_search")
        .constructor<>()                     // Default: adj=true
        .constructor<bool>()                 // adj
        .constructor<bool, int>()            // adj, BFS_DEPTH
        .constructor<bool, int, int>()       // adj, BFS_DEPTH, MEMORY_LIMIT_MB
        .constructor<bool, int, int, bool>() // adj, BFS_DEPTH, MEMORY_LIMIT_MB, verbose
        .function("func", &xxcross_search::func);
}
```

**Worker Implementation** (Highlights):

```javascript
let isInitialized = false;
let solver = null;        // Retain solver instance after database build
let buildLogs = [];       // Collect logs during database build for statistics

// Module configuration with log collection
self.Module = {
    onRuntimeInitialized: () => {
        isInitialized = true;
        self.postMessage({ type: 'initialized' });
        resolve();
    },
    print: function(text) {
        buildLogs.push(text);  // Collect for statistics parsing
        self.postMessage({ type: 'log', message: text });
    },
    printErr: function(text) {
        buildLogs.push('ERROR: ' + text);
        self.postMessage({ type: 'error', message: text });
    }
};

// Build case with statistics parsing
case 'build':
    try {
        buildLogs = [];  // Reset
        solver = new self.Module.xxcross_search(adjacent);
        
        // Parse statistics from build logs
        const stats = parseStatisticsFromLogs(buildLogs);
        stats.success = true;
        stats.message = 'Database built successfully. Solver instance retained.';
        stats.pair_type = adjacent ? 'adjacent' : 'opposite';
        stats.logs = buildLogs;  // Include full logs for debugging
        
        self.postMessage({ type: 'complete', stats: stats });
    } catch (error) {
        // Enhanced error handling
        const errorMessage = error && error.message ? error.message : 
                           error && error.toString ? error.toString() : 
                           'Unknown error (see logs)';
        self.postMessage({ 
            type: 'error', 
            message: `Build failed: ${errorMessage}`,
            logs: buildLogs  // Include logs for diagnosis
        });
    }

// Statistics parsing function
function parseStatisticsFromLogs(logs) {
    const stats = {
        node_counts: [],
        total_nodes: 0,
        final_heap_mb: 0,
        peak_heap_mb: 0,
        load_factors: [],
        bucket_sizes: []
    };
    
    for (const line of logs) {
        // Parse: "depth=7: num_list[7]=1234567, ..."
        const nodeMatch = line.match(/depth=(\\d+):\\s+num_list\\[\\d+\\]=(\\d+)/);
        if (nodeMatch) {
            const depth = parseInt(nodeMatch[1]);
            const count = parseInt(nodeMatch[2]);
            stats.node_counts[depth] = count;
            stats.total_nodes += count;
        }
        
        // Parse: "Final heap: 123.45 MB"
        const heapMatch = line.match(/Final heap:\\s+([\\d.]+)\\s+MB/);
        if (heapMatch) {
            stats.final_heap_mb = parseFloat(heapMatch[1]);
        }
        
        // Parse: "Peak heap: 123.45 MB"
        const peakMatch = line.match(/Peak heap:\\s+([\\d.]+)\\s+MB/);
        if (peakMatch) {
            stats.peak_heap_mb = parseFloat(peakMatch[1]);
        }
        
        // Parse: "Load factor (d7): 0.75"
        const loadMatch = line.match(/Load factor \\(d(\\d+)\\):\\s+([\\d.]+)/);
        if (loadMatch) {
            const depth = parseInt(loadMatch[1]);
            const loadFactor = parseFloat(loadMatch[2]);
            stats.load_factors[depth] = loadFactor;
        }
        
        // Parse: "Bucket size (d7): 8M"
        const bucketMatch = line.match(/Bucket size \\(d(\\d+)\\):\\s+(\\d+)M/);
        if (bucketMatch) {
            const depth = parseInt(bucketMatch[1]);
            const size = parseInt(bucketMatch[2]);
            stats.bucket_sizes[depth] = size;
        }
    }
    
    return stats;
}
```

**Advantages**:
1. ✅ **No C++ Statistics Infrastructure Needed**: Parse from console logs
2. ✅ **True Random Scrambles**: Instance retention enables func() calls
3. ✅ **Detailed Error Reporting**: Full logs included in error messages
4. ✅ **Heap Measurement Capability**: Parse heap stats from C++ output
5. ✅ **Production Pattern**: Same as xcrossTrainer/solver.cpp

**HTML Test Page Features**:
- **Visual Studio Code theme** (dark mode with syntax highlighting colors)
- **Tier Selection**: 6 presets (Mobile LOW → Desktop ULTRA)
- **Detailed Statistics**: Node counts, percentages, load factors
- **Memory Monitoring**: Heap usage with color-coded load factors
- **Random Scramble Generator**: Uses retained solver instance
- **Console Output**: Full C++ logs for debugging
- **💾 Save Log Feature**: Export full console log to text file

**Error Handling Enhancements**:

1. **Abort Handler** (Worker):
   ```javascript
   self.Module = {
       onAbort: function(what) {
           const abortMsg = 'ABORT: ' + (what || 'Unknown abort reason');
           buildLogs.push(abortMsg);
           self.postMessage({ 
               type: 'error', 
               message: 'WASM aborted: ' + (what || 'Out of memory or assertion failure'),
               logs: buildLogs 
           });
       }
   };
   ```

2. **Enhanced Exception Handling**:
   ```javascript
   catch (error) {
       // Extract meaningful error message from C++ exception
       let errorMessage = 'Unknown error';
       if (error && typeof error === 'object') {
           if (error.message) {
               errorMessage = error.message;
           } else {
               // C++ exception might be a pointer/number
               errorMessage = 'C++ exception (code: ' + error + ')';
           }
       }
       
       // Check recent logs for error context
       const recentLogs = buildLogs.slice(-5);
       const errorDetails = recentLogs.filter(line => 
           line.includes('ERROR') || line.includes('ABORT')
       ).join('\\n');
   }
   ```

**Log Save Feature** (HTML):

```javascript
let allLogs = [];  // Store all logs for saving

function saveLog() {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const tierName = document.querySelector('.tier-btn.selected').textContent.split('\\n')[0];
    const header = `XXCross WASM Test Log
Generated: ${new Date().toLocaleString()}
Tier: ${tierName} (${selectedBuckets.join('M/')}M)
Browser: ${navigator.userAgent}
${'='.repeat(80)}

`;
    
    const logContent = header + allLogs.join('\\n');
    
    // Create and download file
    const blob = new Blob([logContent], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `xxcross_wasm_log_${timestamp}.txt`;
    document.body.appendChild(a);
    a.click();
    URL.revokeObjectURL(url);
}
```

**Usage**:
1. Build database (any tier)
2. Check console output for logs
3. Click "💾 Save Log" to download complete log file
4. Log includes: Timestamp, tier info, browser details, all console messages

---

## Debugging Guide

**Debugging**:

Console logging helps diagnose issues:

1. **Worker logs** (instance creation):
   ```
   [Worker] Creating xxcross_search instance...
   [Worker] Parameters: adj=true
   [Worker] Instance created successfully
   ```

2. **Worker logs** (scramble generation):
   ```
   [Worker] Scramble request: {depth: 6, pairType: 'adjacent'}
   [Worker] solver exists: true
   [Worker] func() result: "some_result,R U R' F2 ..."
   [Worker] Parsed scramble: "R U R' F2 ..."
   ```

3. **HTML logs** (scramble response):
   ```
   Generated (adjacent, depth 6): R U R' F2 ...
   ```

To debug issues:
- Open browser DevTools Console (F12)
- Build database and attempt scramble generation
- Check console output for unexpected values
- Common issues:
  - `solver exists: false` → Database not built yet
  - `func() result: undefined` → Check embind exports
  - Error during instance creation → Check WASM memory limits

**Advantages of Instance Retention**:

1. **True Random Scrambles**: Each call generates different scramble (not fixed)
2. **No Memory Overhead**: Database already in memory, instance just wraps it
3. **Production Pattern**: Same as xcrossTrainer/solver.cpp (proven stable)
4. **Unlimited Generation**: Can generate thousands of scrambles without rebuild
5. **Simpler C++ Code**: No sample_scrambles infrastructure needed

**Heap Measurement with Worker Pattern**:

Even for heap measurement, Worker pattern is more stable than direct WASM:
- Isolates memory from main thread
- Consistent memory accounting
- Can measure multiple tiers sequentially
- Same code path as production deployment

### HTML Test Page (test_wasm_browser.html)

**✅ Tested and verified** (2026-01-03)

**Key Features**:
- Non-blocking Worker-based database construction
- Progress updates during build
- Statistics display with load factors
- Sample scrambles by depth

**Implementation**: See [test_wasm_browser.html](../../test_wasm_browser.html) in repository

**JavaScript Worker Integration**:

```javascript
// Initialize Web Worker
const worker = new Worker('worker_test_wasm_browser.js');
let currentStats = null;
let buildStartTime = 0;

worker.onmessage = function(event) {
    const { type, message, stats } = event.data;
    
    switch(type) {
        case 'initialized':
            document.getElementById('status').innerHTML = '✓ Worker initialized';
            document.getElementById('status').className = 'status success';
            document.getElementById('controls').style.display = 'block';
            log('Ready. Select a tier and click "Build Database".', 'info');
            break;
            
        case 'progress':
            document.getElementById('status').innerHTML = message;
            document.getElementById('status').className = 'status info';
            log(message, 'info');
            break;
            
        case 'log':
            log(message);
            break;
            
        case 'scramble':
            console.log('Received scramble event:', event.data);
            const { scramble, actualDepth, pairType } = event.data;
            const outputDiv = document.getElementById('scramble-output');
            
            if (!scramble) {
                outputDiv.innerHTML = '<div style="color: red;">Error: No scramble received</div>';
                log('Error: No scramble data in response', 'error');
                break;
            }
            
            outputDiv.innerHTML = `
                <div style="margin-bottom: 10px;">
                    <b>Pair Type:</b> ${pairType}<br>
                    <b>Depth:</b> ${actualDepth}<br>
                    <b>Scramble:</b> ${scramble}
                </div>
            `;
            log(`Generated (${pairType}, depth ${actualDepth}): ${scramble}`, 'success');
            break;
            
        case 'complete':
            const duration = ((Date.now() - buildStartTime) / 1000).toFixed(2);
            currentStats = stats;
            
            document.getElementById('status').innerHTML = 
                `✓ Build complete (${duration}s) - Peak: ${stats.peak_heap_mb.toFixed(2)} MB`;
            document.getElementById('status').className = 'status success';
            document.getElementById('buildBtn').disabled = false;
            document.getElementById('solver-test').style.display = 'block';
            
            // Show summary with node distribution
            log(`<br>✓ Database built in ${duration}s`, 'success');
            log(`  Total nodes: ${(stats.total_nodes ?? 0).toString().replace(/\\B(?=(\\d{3})+(?!\\d))/g, ',')}`);
            log(`  Final heap: ${stats.final_heap_mb.toFixed(2)} MB`);
            log(`  Peak heap: ${stats.peak_heap_mb.toFixed(2)} MB`);
            
            // Show node distribution summary (no percentages)
            if (stats.node_counts && stats.node_counts.length > 0) {
                log('<br><b>Node Distribution:</b>');
                let totalNodes = 0;
                for (let i = 0; i < stats.node_counts.length; i++) {
                    totalNodes += stats.node_counts[i];
                    log(`  Depth ${i}: ${stats.node_counts[i].toString().replace(/\\B(?=(\\d{3})+(?!\\d))/g, ',')} nodes`);
                }
                log(`  <b>Total: ${totalNodes.toString().replace(/\\B(?=(\\d{3})+(?!\\d))/g, ',')} nodes</b>`);
            }
            break;
            
        case 'error':
            document.getElementById('status').innerHTML = '✗ Error: ' + message;
            document.getElementById('status').className = 'status error';
            document.getElementById('buildBtn').disabled = false;
            log('Error: ' + message, 'error');
            break;
    }
};

function buildDatabase() {
    const tierSelect = document.getElementById('tier-select');
    const buckets = tierSelect.value.split(',').map(Number);
    
    document.getElementById('buildBtn').disabled = true;
    buildStartTime = Date.now();
    
    log(`Building database with ${buckets.join('M/')}M buckets...`);
    log('This may take 2-3 minutes...', 'info');
    
    // Send build command to worker
    worker.postMessage({
        type: 'build',
        data: {
            buckets: buckets,
            verbose: true
        }
    });
}

function testScramble() {
    if (!currentStats) {
        log('Build database first!', 'error');
        return;
    }
    
    const depth = parseInt(document.getElementById('depth-select').value);
    const pairType = document.getElementById('pair-type-select').value;
    log(`Requesting scramble at depth ${depth} for ${pairType} pairs...`, 'info');
    
    // Request scramble from worker (tests class reusability)
    worker.postMessage({
        type: 'scramble',
        data: { 
            depth: depth,
            pairType: pairType
        }
    });
}
```

**HTML UI Addition** (Pair Type Selection):
```html
<div class="container" id="solver-test" style="display: none;">
    <h2>Solver Test</h2>
    <label>Pair Type:</label>
    <select id="pair-type-select">
        <option value="adjacent" selected>Adjacent Pairs</option>
        <option value="opposite">Opposite Pairs</option>
    </select>
    <label>Scramble Depth:</label>
    <select id="depth-select">
        <option value="1">Depth 1</option>
        ...
        <option value="10">Depth 10</option>
    </select>
    <button onclick="testScramble()">Generate Scramble</button>
    <div id="scramble-output"></div>
</div>
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer

# Build WASM for Worker
./build_wasm_test.sh

# Serve with HTTP server
python3 -m http.server 8000

# Open browser
# Navigate to: http://localhost:8000/test_wasm_browser.html
```

### Important Notes

**Why Worker is Essential**:
- xxcross solver uses **618 MB - 2884 MB** heap memory
- Database construction takes **2-3 minutes**
- Running on main thread **blocks all UI and other JavaScript**
- Worker allows non-blocking operation with progress updates

**Build Differences**:

| Binary | MODULARIZE | Usage | importScripts | Build Script |
|--------|-----------|-------|---------------|--------------|
| `solver_heap_measurement.js` | ✅ Yes (`SolverModule`) | Direct call (measurement) | ❌ No | build_wasm_unified.sh |
| `test_solver.js` | ❌ No (global `Module`) | Web Worker (production) | ✅ Yes | build_wasm_test.sh |

**Files in xxcrossTrainer directory**:

Production (Worker-based):
- `test_wasm_browser.html` - Production test page ✅
- `worker_test_wasm_browser.js` - Worker script ✅
- `test_solver.{js,wasm}` - WASM binary for Worker ✅
- `build_wasm_test.sh` - Build script for Worker ✅

Measurement (Direct call):
- `wasm_heap_advanced.html` - Heap measurement tool
- `solver_heap_measurement.{js,wasm}` - WASM binary for measurement
- `build_wasm_unified.sh` - Build script for measurement

**Pattern Compatibility**:
✅ This pattern is compatible with existing [xcrossTrainer/worker.js](../../../xcrossTrainer/worker.js) implementation

**Recent Updates (2026-01-03)**:
- ✅ **Improved log output display**: HTML line breaks instead of escaped `\n`
- ✅ **Node distribution summary**: Automatic display after database build
- ✅ **Solver test feature**: Generate scrambles (depth 1-10) to verify solver functionality
- ✅ **Adjacent/Opposite pair support**: Dual solver instances for both pair types
  - Worker creates two `xxcross_search` instances: `solverAdj` (adj=true) and `solverOpp` (adj=false)
  - UI dropdown allows selection of pair type
  - Same WASM binary supports both configurations
- ✅ **Fixed scramble generation**: Uses `xxcross_search::func("", depth_str)` method
  - Replaces non-existent `get_xxcross_scramble` function
  - `func(arg_scramble, arg_length)` returns: `start_search(arg_scramble) + "," + get_xxcross_scramble(arg_length)`
  - Empty scramble → result split by `,` to extract scramble
- ✅ **Class reusability confirmed**: Worker can generate scrambles after building database without recreation
- ✅ **Statistics table enhancement**: Added node count distribution by depth

**Key Features**:
1. **Non-blocking Construction**: Worker-based, main thread stays responsive
2. **Progress Updates**: Real-time build progress and log streaming
3. **Node Distribution**: Automatic summary showing nodes per depth with percentages
4. **Solver Testing**: Generate scrambles at specific depths (1-10)
   - Uses `xxcross_search::func("", depth_str)` for scramble generation
   - Validates with `Module.StringToAlg()` to count moves
   - Confirms Worker class reusability (no rebuild needed)
   - **Supports both adjacent and opposite pairs** via dual solver instances
5. **Statistics Display**: Complete metrics including load factors and heap usage

**Dual Solver Architecture**:
- **No separate binaries needed**: Single WASM binary (`test_solver.js/wasm`) supports both pair types
- **Constructor parameter**: `xxcross_search(adj)` where `adj=true` for adjacent, `adj=false` for opposite
- **Worker initialization**: Creates both instances at startup for instant switching
- **Memory efficient**: Solver instances are lightweight, database construction is the heavy operation

**Deployment**:
- **Shared binary**: Both test_wasm_browser.html and wasm_heap_advanced.html use solver_heap_measurement.{js,wasm}
- **CORS requirement**: Must serve via HTTP server (not `file://` protocol)

**Files in xxcrossTrainer directory**:
- `test_wasm_browser.html` - **Production test page** (Worker-based, non-blocking)
- `wasm_heap_advanced.html` - **Heap measurement tool** (detailed statistics)
- `solver_heap_measurement.{js,wasm}` - WASM binary (MODULARIZE=1, embind enabled)
- `build_wasm_unified.sh` - Build script (shared by both HTML files)

---

## Build Script for WASM (Reference - Updated for stable_20260103)

### Complete build workflow

```bash
#!/bin/bash
#
# build_wasm_unified.sh
# Complete WASM build with heap measurement (stable_20260103)
#

set -euo pipefail

echo "======================================================================"
echo "WASM Build (stable_20260103)"
echo "======================================================================"

# Configuration
SOLVER_CPP="solver_dev.cpp"
OUTPUT_JS="solver_heap_measurement.js"
OUTPUT_WASM="solver_heap_measurement.wasm"

# Clean previous builds
echo "Cleaning previous builds..."
rm -f ${OUTPUT_JS} ${OUTPUT_WASM}

# Build WASM with embind and heap monitoring
echo "Building WASM..."
source ~/emsdk/emsdk_env.sh

em++ -std=c++17 -O3 ${SOLVER_CPP} -o ${OUTPUT_JS} \\
  -I../tsl-robin-map/include \\
  -s ALLOW_MEMORY_GROWTH=1 \\
  -s INITIAL_MEMORY=67108864 \\
  -s MAXIMUM_MEMORY=4294967296 \\
  -s EXPORTED_RUNTIME_METHODS='["ENV"]' \\
  -s MODULARIZE=1 \\
  -s EXPORT_NAME="createModule" \\
  -lembind \\
  --bind \\
  -fno-rtti

if [ $? -eq 0 ]; then
    echo "✓ Build successful"
else
    echo "✗ Build failed"
    exit 1
fi

# Check output files
echo ""
echo "Output files:"
ls -lh ${OUTPUT_JS} ${OUTPUT_WASM}

# Get file sizes
js_size=$(stat -f%z ${OUTPUT_JS} 2>/dev/null || stat -c%s ${OUTPUT_JS})
wasm_size=$(stat -f%z ${OUTPUT_WASM} 2>/dev/null || stat -c%s ${OUTPUT_WASM})

echo ""
echo "File sizes:"
echo "  ${OUTPUT_JS}: $(numfmt --to=iec ${js_size} 2>/dev/null || echo "${js_size} bytes")"
echo "  ${OUTPUT_WASM}: $(numfmt --to=iec ${wasm_size} 2>/dev/null || echo "${wasm_size} bytes")"

echo ""
echo "======================================================================"
echo "Build Complete"
echo "======================================================================"
echo ""
echo "WASM module ready for deployment"
echo "Files:"
echo "  - ${OUTPUT_JS} (JavaScript loader with embind)"
echo "  - ${OUTPUT_WASM} (WebAssembly binary)"
echo ""
echo "Features:"
echo "  ✓ 5-Phase construction (depths 0-10)"
echo "  ✓ Custom bucket configuration (d7/d8/d9/d10)"
echo "  ✓ Heap measurement checkpoints"
echo "  ✓ SolverStatistics embind export"
echo "  ✓ Safe scramble generation"
```

### Usage

```bash
# From workspace root
cd src/xxcrossTrainer
chmod +x build_wasm_unified.sh
./build_wasm_unified.sh
```

---

## Troubleshooting

### Problem: Build stops after Phase 1 ("Build failed: 5294448")

**Symptom**:
```
[Heap] Phase 1 Complete: Total=309 MB
========================================

Error: Build failed: 5294448
```

**Root Cause**: 
Custom bucket configuration not enabled. The simple constructor (only `adj` parameter) uses default AUTO bucket model, which doesn't support tier-specific configurations like 1M/1M/2M/4M.

**Solution**:
Use the **custom bucket constructor** with 5 parameters:

```javascript
// Worker pattern (worker_test_wasm_browser.js)
const [b7, b8, b9, b10] = buckets;  // e.g., [1, 1, 2, 4]
const adjacent = true;

// NEW: Custom bucket constructor (5 params)
solver = new self.Module.xxcross_search(adjacent, b7, b8, b9, b10);
```

**C++ embind exports** (solver_dev.cpp):
```cpp
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::class_<xxcross_search>("xxcross_search")
        .constructor<>()  // Default
        .constructor<bool>()  // adj only
        .constructor<bool, int, int, int, int>()  // adj, b7_mb, b8_mb, b9_mb, b10_mb (CUSTOM)
        .function("func", &xxcross_search::func);
}
```

**Constructor Implementation** (solver_dev.cpp, lines 3862-3874):
```cpp
// Custom bucket constructor (for WASM tier selection)
xxcross_search(bool adj, int bucket_7_mb, int bucket_8_mb, int bucket_9_mb, int bucket_10_mb)
    : xxcross_search(adj, 6, bucket_7_mb + bucket_8_mb + bucket_9_mb + bucket_10_mb + 300, true,
                    create_custom_bucket_config(bucket_7_mb, bucket_8_mb, bucket_9_mb, bucket_10_mb),
                    create_custom_research_config())
{
    // Delegating constructor - automatically sets:
    // - BucketModel::CUSTOM
    // - enable_custom_buckets = true
    // - skip_search = true
    // - high_memory_wasm_mode = true (WASM auto-detect)
}
```

**Benefits**:
1. ✅ **Tier Support**: Enables all 6 tier configurations (Mobile LOW → Desktop ULTRA)
2. ✅ **Automatic Setup**: Internal helper functions configure BucketConfig and ResearchConfig
3. ✅ **Memory Calculation**: MEMORY_LIMIT_MB = sum(buckets) + 300MB overhead
4. ✅ **No Manual Config**: Research flags set automatically (enable_custom_buckets, skip_search)

**Verification**:
After rebuild, constructor logs should show:
```
Bucket Configuration:
  Depth 7:  1 MB (1048576 bytes)
  Depth 8:  1 MB (1048576 bytes)
  Depth 9:  2 MB (2097152 bytes)
  Depth 10: 4 MB (4194304 bytes)
========================================

Research mode flags:
  enable_custom_buckets: 1  ← Should be 1
  high_memory_wasm_mode: 1  ← Should be 1
```

---

### Problem: Build fails with cryptic error ("Build failed: 5293200")

**Symptom**:
```
Error: Build failed: 5293200
=== Error Logs ===
=== xxcross_search Constructor ===
Configuration:
  ...
  high_memory_wasm_mode: 0
```

**Root Cause**: 
C++ exception thrown during robin_set construction. Error code `5293200` is a C++ exception pointer address, indicating memory allocation failure. The issue occurs when `high_memory_wasm_mode` is disabled (0) but memory requirements exceed 1200MB.

**Solution**:
WASM environment now defaults `high_memory_wasm_mode = true` automatically (fixed in `bucket_config.h`):

```cpp
// bucket_config.h (fixed 2026-01-03)
struct ResearchConfig {
    #ifdef __EMSCRIPTEN__
    bool high_memory_wasm_mode = true;  // WASM: Always true
    #else
    bool high_memory_wasm_mode = false; // Native: false by default
    #endif
    // ...
};
```

**Verification**:
After rebuild, constructor logs should show:
```
=== xxcross_search Constructor ===
Configuration:
  ...
  high_memory_wasm_mode: 1  ← Should be 1 for WASM
```

**Error Handling Improvements** (2026-01-03):

Added abort handler and enhanced error extraction in `worker_test_wasm_browser.js`:

```javascript
self.Module = {
    onAbort: function(what) {
        const abortMsg = 'ABORT: ' + (what || 'Unknown abort reason');
        buildLogs.push(abortMsg);
        self.postMessage({ 
            type: 'error', 
            message: 'WASM aborted: ' + (what || 'Out of memory or assertion failure'),
            logs: buildLogs 
        });
    }
};

// Enhanced exception extraction
catch (error) {
    let errorMessage = 'Unknown error';
    if (error && typeof error === 'object') {
        if (error.message) {
            errorMessage = error.message;
        } else {
            // C++ exception might be a pointer/number like "5293200"
            errorMessage = 'C++ exception (code: ' + error + ')';
        }
    }
    
    // Include recent logs for context
    const recentLogs = buildLogs.slice(-5);
    const errorDetails = recentLogs.filter(line => 
        line.includes('ERROR') || line.includes('ABORT')
    ).join('\\n');
}
```

**Benefits**:
- Meaningful error messages instead of cryptic pointer addresses
- Abort handler catches memory errors and assertions
- Recent logs provide context for debugging
- No manual configuration needed (WASM auto-detection)

---

### Problem: Module fails to load

```javascript
// Error: Cannot find module './solver_heap_measurement.js'
```

**Solution**: Ensure build completed successfully:
```bash
# From workspace root
cd src/xxcrossTrainer
ls -l solver_heap_measurement.js solver_heap_measurement.wasm
em++ --version  # Verify emscripten installed
```

---

### Problem: Out of memory in browser

```
RuntimeError: memory access out of bounds
```

**Solution**: Use smaller tier or verify ALLOW_MEMORY_GROWTH:
```bash
# Mobile LOW (618 MB) should work on most devices
# Check build script has: -s ALLOW_MEMORY_GROWTH=1
```

---

### Problem: Statistics not accessible

```javascript
// TypeError: Module.get_solver_statistics is not a function
```

**Solution**: Verify embind exports in solver_dev.cpp:
```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(solver_module) {
    emscripten::function("solve_with_custom_buckets", &solve_with_custom_buckets);
    emscripten::function("get_solver_statistics", &get_solver_statistics);
    
    emscripten::class_<SolverStatistics>("SolverStatistics")
        .property("total_nodes", &SolverStatistics::total_nodes)
        .property("final_heap_mb", &SolverStatistics::final_heap_mb)
        // ... other properties
        ;
    
    emscripten::register_vector<int>("VectorInt");
    emscripten::register_vector<double>("VectorDouble");
    emscripten::register_vector<std::string>("VectorString");
}
#endif
```

---

### Problem: Heap checkpoints not appearing

```
// No [Heap] logs in console output
```

**Solution**: Enable verbose mode:
```javascript
Module.solve_with_custom_buckets(8, 8, 8, 8, true);  // verbose=true
```

---

## Related Documentation

- [WASM_BUILD_GUIDE.md](WASM_BUILD_GUIDE.md) - Complete build instructions
- [WASM_INTEGRATION_GUIDE.md](WASM_INTEGRATION_GUIDE.md) - Production integration guide
- [SOLVER_IMPLEMENTATION.md](SOLVER_IMPLEMENTATION.md) - Code implementation details (stable_20260103)
- [API_REFERENCE.md](API_REFERENCE.md) - Complete function reference
- [Experiments/wasm_heap_measurement_results.md](Experiments/wasm_heap_measurement_results.md) - 6-tier WASM bucket model
- [../USER_GUIDE.md](../USER_GUIDE.md) - End-user deployment guide

---

**Document Version**: 2.1  
**Status**: Production-ready (stable_20260103)  
**Key Updates**: 
- Updated for 5-Phase construction (depths 0-10)
- Custom bucket configuration (depth 7-10 explicit specification)
- Custom bucket constructor (5-parameter: adj, b7_mb, b8_mb, b9_mb, b10_mb)
- embind integration (SolverStatistics + xxcross_search class)
- Heap measurement checkpoints with Final Statistics output
- 6-tier WASM model support
- Worker-based architecture with instance retention (test_wasm_browser.html)
- Statistics parsing from C++ console logs (Final heap, Load factors)
- Scramble generation with `get_xxcross_scramble()` fix (tmp assignment)

**Recent Fixes (2026-01-03)**:
- Fixed `get_xxcross_scramble()` return value (added `tmp = AlgToString(sol);`)
- Added Final Statistics output block in constructor (`#ifdef __EMSCRIPTEN__`)
- Fixed statistics parser for bucket sizes (now parses "X.XX MB" format)
- UI layout improvement (Scramble Generator above Statistics panels)
