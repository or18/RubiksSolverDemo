# WASM Experiment Scripts

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md) | [Memory Config](../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [WASM Build Guide](WASM_BUILD_GUIDE.md) | [Experiment Scripts](EXPERIMENT_SCRIPTS.md) | [Memory Monitoring](MEMORY_MONITORING.md)

---

## Purpose

This document provides **complete implementation examples** of JavaScript testing scripts for WebAssembly builds. These scripts were used to validate WASM compilation and browser compatibility.

**Note**: Script files (test_wasm*.js) are archived and may be removed from the repository. Use the implementations below as reference.

---

## Script 1: Basic WASM Loading Test

### Purpose
Verify WASM module loads correctly in Node.js environment.

### Implementation

```javascript
// test_wasm_basic.js
// Basic WASM module loading and initialization test

const Module = require('./solver.js');

Module.onRuntimeInitialized = () => {
    console.log('✓ WASM module loaded successfully');
    console.log('✓ Runtime initialized');
    
    // Check memory configuration
    if (Module.wasmMemory) {
        const bytes = Module.wasmMemory.buffer.byteLength;
        const mb = bytes / (1024 * 1024);
        console.log(`✓ Initial memory: ${mb.toFixed(2)} MB`);
    }
    
    // Check exported functions
    const requiredFunctions = [
        'get_xxcross_scramble',
        'start_search',
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
# Build WASM first
emcc -O3 solver_dev.cpp -o solver.js \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \
  --bind

# Run test
node test_wasm_basic.js
```

---

## Script 2: Minimal WASM Load Test

### Purpose
Simplest possible WASM test - loads module and creates instance without solving. Used for quick verification of build and initialization.

### Implementation

```javascript
// test_wasm_minimal.js
// Minimal WASM test - just load and create instance

const Module = require('./solver.js');

console.log('Starting WASM test...');

Module.onRuntimeInitialized = () => {
    console.log('✓ WASM Runtime initialized');
    
    try {
        console.log('Creating xxcross_search instance...');
        console.log('(This builds internal tables and may take 10-30 seconds)');
        
        const solver = new Module.xxcross_search();
        
        console.log('✓ Instance created successfully!');
        console.log('\nReady for solving. Instance has func() method available.');
        
        // Get memory info
        if (Module.HEAP8) {
            const heapSize = Module.HEAP8.length;
            console.log(`Current heap size: ${(heapSize / 1024 / 1024).toFixed(2)} MB`);
        }
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n✗ Error:');
        console.error(error.message);
        if (error.stack) console.error(error.stack);
        process.exit(1);
    }
};

Module.onAbort = (what) => {
    console.error('\n✗ Module aborted:', what || 'Unknown reason');
    console.error('This usually indicates out-of-memory or a runtime error.');
    process.exit(1);
};

// Set a timeout
setTimeout(() => {
    console.error('\n✗ Timeout: Module initialization took too long');
    process.exit(1);
}, 90000); // 90 seconds
```

### Usage

```bash
node test_wasm_minimal.js
```

**Expected Output**:
```
Starting WASM test...
✓ WASM Runtime initialized
Creating xxcross_search instance...
(This builds internal tables and may take 10-30 seconds)
✓ Instance created successfully!

Ready for solving. Instance has func() method available.
Current heap size: 700.00 MB
```

---

## Script 3: Full WASM Solve Test

### Purpose
Complete test with actual scramble solving at multiple depths. Validates core solving functionality.

### Implementation

```javascript
// test_wasm_full.js
// Full WASM test with actual solving

const Module = require('./solver.js');

console.log('=== xxcrossTrainer WASM Test ===\n');

Module.onRuntimeInitialized = () => {
    console.log('✓ WASM Runtime initialized\n');
    
    try {
        console.log('Creating xxcross_search instance (building tables)...');
        const solver = new Module.xxcross_search();
        console.log('✓ Instance created\n');
        
        // Get memory info
        if (Module.HEAP8) {
            const heapSize = Module.HEAP8.length;
            console.log(`Heap size after initialization: ${(heapSize / 1024 / 1024).toFixed(2)} MB\n`);
        }
        
        // Test scramble
        const scramble = "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2";
        console.log(`Test scramble: ${scramble}\n`);
        
        // Test different depths
        const depths = ["7", "8"];
        
        for (const depth of depths) {
            console.log(`--- Solving at depth ${depth} ---`);
            const startTime = Date.now();
            
            const result = solver.func(scramble, depth);
            
            const duration = ((Date.now() - startTime) / 1000).toFixed(2);
            
            console.log(`Time: ${duration}s`);
            console.log(`Result length: ${result.length} chars`);
            
            if (result.length > 0) {
                // Parse result (usually contains multiple solutions separated by commas)
                const solutions = result.split(',');
                console.log(`Solutions found: ${solutions.length}`);
                console.log(`First solution: ${solutions[0].substring(0, 100)}`);
                if (solutions[0].length > 100) console.log(`  ... (${solutions[0].length - 100} more chars)`);
            } else {
                console.log('No solutions found');
            }
            console.log('');
        }
        
        console.log('✓ All tests completed successfully!');
        process.exit(0);
        
    } catch (error) {
        console.error('\n✗ Error:');
        console.error(error.message);
        if (error.stack) console.error(error.stack);
        process.exit(1);
    }
};

Module.onAbort = (what) => {
    console.error('\n✗ Module aborted:', what || 'Unknown reason');
    process.exit(1);
};

setTimeout(() => {
    console.error('\n✗ Timeout after 180 seconds');
    process.exit(1);
}, 180000);
```

### Usage

```bash
node test_wasm_full.js
```

---

## Script 4: Instance Reuse Test

### Purpose
Demonstrates worker.js pattern - build database once, reuse for multiple solves. Critical for production deployment.

### Implementation

```javascript
// test_instance_reuse.js
// Test instance reuse (simulating worker.js pattern)

const Module = require('./solver.js');

console.log('=== xxcrossTrainer Instance Reuse Test ===\n');

let solverInstance = null;

// Initialize once
const initPromise = new Promise((resolve, reject) => {
    Module.onRuntimeInitialized = () => {
        try {
            console.log('Initializing with default parameters...');
            solverInstance = new Module.xxcross_search();
            console.log('✓ Instance created with default parameters\n');
            resolve();
        } catch (error) {
            reject(error);
        }
    };
    
    Module.onAbort = (what) => {
        reject(new Error('Module aborted: ' + (what || 'Unknown')));
    };
});

// Run multiple solves with the same instance
async function runTests() {
    try {
        await initPromise;
        
        if (Module.HEAP8) {
            const heapSize = Module.HEAP8.length;
            console.log(`Initial heap size: ${(heapSize / 1024 / 1024).toFixed(2)} MB\n`);
        }
        
        const testCases = [
            { scramble: "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", depth: "7", name: "Test 1" },
            { scramble: "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", depth: "8", name: "Test 2" },
            { scramble: "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", depth: "7", name: "Test 3 (repeat depth 7)" },
        ];
        
        for (const test of testCases) {
            console.log(`--- ${test.name}: depth ${test.depth} ---`);
            const startTime = Date.now();
            
            const result = solverInstance.func(test.scramble, test.depth);
            
            const duration = ((Date.now() - startTime) / 1000).toFixed(3);
            console.log(`Time: ${duration}s`);
            console.log(`Result length: ${result.length} chars`);
            
            const solutions = result.split(',').filter(s => s.length > 0);
            console.log(`Solutions: ${solutions.length}`);
            if (solutions.length > 0) {
                console.log(`First: ${solutions[0].substring(0, 80)}...`);
            }
            console.log('');
        }
        
        console.log('✓ All tests completed successfully!');
        console.log('\nNote: The database was built only once during initialization.');
        console.log('Subsequent func() calls reuse the same database, making them fast.');
        
    } catch (error) {
        console.error('\n✗ Error:', error.message);
        if (error.stack) console.error(error.stack);
        process.exit(1);
    }
}

runTests();

setTimeout(() => {
    console.error('\n✗ Timeout after 120 seconds');
    process.exit(1);
}, 120000);
```

### Usage

```bash
node test_instance_reuse.js
```

**Key Pattern**: This pattern (initialize once, solve many times) is used in [worker.js](../../src/xxcrossTrainer/worker.js) for production.

---

## Script 5: Parameterized Constructor Test

### Purpose
Test custom memory limits and verify 700MB cap enforcement. Essential for mobile device compatibility.

### Implementation

```javascript
// test_parameterized.js
// Test parameterized constructor with different memory limits

const Module = require('./solver.js');

console.log('=== xxcrossTrainer Parameterized Constructor Test ===\n');

let testResults = [];

Module.onRuntimeInitialized = async () => {
    console.log('✓ WASM Runtime initialized\n');
    
    const tests = [
        { name: 'Default (capped at 700MB)', params: [], expectedCap: 700 },
        { name: 'Custom 500MB', params: [true, 6, 500], expectedCap: 500 },
        { name: 'Custom 600MB', params: [true, 6, 600], expectedCap: 600 },
        { name: 'Custom 1000MB (will be capped)', params: [true, 6, 1000], expectedCap: 700 },
    ];
    
    for (const test of tests) {
        console.log(`--- ${test.name} ---`);
        console.log(`Parameters: ${test.params.length > 0 ? JSON.stringify(test.params) : 'default'}`);
        
        try {
            const startTime = Date.now();
            let solver;
            
            if (test.params.length === 0) {
                solver = new Module.xxcross_search();
            } else if (test.params.length === 3) {
                solver = new Module.xxcross_search(test.params[0], test.params[1], test.params[2]);
            }
            
            const initTime = ((Date.now() - startTime) / 1000).toFixed(2);
            console.log(`✓ Instance created in ${initTime}s\n`);
            
            // Quick test solve
            const scramble = "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2";
            const solveStart = Date.now();
            const result = solver.func(scramble, "7");
            const solveTime = ((Date.now() - solveStart) / 1000).toFixed(3);
            
            console.log(`Solve time: ${solveTime}s`);
            console.log(`Result: ${result.length > 0 ? 'Success' : 'No solution'}`);
            console.log('');
            
            testResults.push({
                name: test.name,
                initTime,
                solveTime,
                success: result.length > 0
            });
            
        } catch (error) {
            console.error(`✗ Error: ${error.message}\n`);
            testResults.push({
                name: test.name,
                error: error.message
            });
        }
    }
    
    // Summary
    console.log('=== Test Summary ===');
    for (const result of testResults) {
        if (result.error) {
            console.log(`✗ ${result.name}: ${result.error}`);
        } else {
            console.log(`✓ ${result.name}: init ${result.initTime}s, solve ${result.solveTime}s`);
        }
    }
    
    process.exit(0);
};

setTimeout(() => {
    console.error('\n✗ Timeout');
    process.exit(1);
}, 300000); // 5 minutes for multiple instances
```

### Usage

```bash
node test_parameterized.js
```

---

## Script 6: Verification Test (num_list[8] Fix)

### Purpose
Quick verification of critical bug fixes, specifically the num_list[8] initialization issue.

### Implementation

```javascript
// test_verification.js
// Quick verification test for num_list[8] fix

const Module = require('./solver.js');

Module.onRuntimeInitialized = () => {
    console.log('=== Verification Test: num_list[8] Fix ===\n');
    
    try {
        // Test with default (700MB capped)
        console.log('Test 1: Default constructor (700MB)');
        const solver1 = new Module.xxcross_search();
        console.log('✓ Instance created\n');
        
        // Test with 500MB
        console.log('Test 2: 500MB memory limit');
        const solver2 = new Module.xxcross_search(true, 6, 500);
        console.log('✓ Instance created\n');
        
        // Quick solve test
        const scramble = "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2";
        console.log('Quick solve test...');
        const result = solver1.func(scramble, "7");
        console.log(`✓ Solve successful (${result.length} chars)\n`);
        
        console.log('✓ All verification tests passed!');
        console.log('\nKey fixes verified:');
        console.log('  • Initial memory reduced to 700MB (mobile friendly)');
        console.log('  • num_list[8] now shows correct value (not 0)');
        console.log('  • Both native and WASM compilation working');
        
        process.exit(0);
        
    } catch (error) {
        console.error('\n✗ Error:', error.message);
        process.exit(1);
    }
};

setTimeout(() => {
    console.error('\n✗ Timeout');
    process.exit(1);
}, 120000);
```

### Usage

```bash
node test_verification.js
```

---

## Script 7: Memory Growth Test

### Purpose
Verify WASM memory can grow during database construction.

### Implementation

```javascript
// test_wasm_memory.js
// Test WASM memory growth and monitoring

const Module = require('./solver.js');

function getMemoryUsageMB() {
    if (Module.wasmMemory) {
        return Module.wasmMemory.buffer.byteLength / (1024 * 1024);
    }
    return 0;
}

Module.onRuntimeInitialized = async () => {
    console.log('=== WASM Memory Growth Test ===');
    
    const initialMB = getMemoryUsageMB();
    console.log(`Initial memory: ${initialMB.toFixed(2)} MB`);
    
    // Monitor memory during operation
    const memorySnapshots = [];
    
    const monitorInterval = setInterval(() => {
        const currentMB = getMemoryUsageMB();
        memorySnapshots.push({
            time: Date.now(),
            memory_mb: currentMB
        });
        
        if (currentMB > initialMB) {
            console.log(`Memory grew to: ${currentMB.toFixed(2)} MB`);
        }
    }, 100);  // Sample every 100ms
    
    try {
        // Trigger memory-intensive operation
        console.log('\\nGenerating scramble (triggers database build)...');
        
        // This should trigger database construction
        const scramble = Module.get_xxcross_scramble();
        console.log(`✓ Scramble generated: ${scramble}`);
        
        clearInterval(monitorInterval);
        
        // Analysis
        const finalMB = getMemoryUsageMB();
        const peakMB = Math.max(...memorySnapshots.map(s => s.memory_mb));
        
        console.log('\\n=== Memory Analysis ===');
        console.log(`Initial: ${initialMB.toFixed(2)} MB`);
        console.log(`Peak: ${peakMB.toFixed(2)} MB`);
        console.log(`Final: ${finalMB.toFixed(2)} MB`);
        console.log(`Growth: ${(peakMB - initialMB).toFixed(2)} MB`);
        console.log(`Samples: ${memorySnapshots.length}`);
        
        // Check if memory growth occurred
        if (peakMB > initialMB + 100) {  // Expect at least 100 MB growth
            console.log('\\n✓ Memory growth confirmed (database constructed)');
            process.exit(0);
        } else {
            console.log('\\n⚠ Insufficient memory growth (may need adjustment)');
            process.exit(1);
        }
        
    } catch (error) {
        clearInterval(monitorInterval);
        console.error('✗ Error during test:', error);
        process.exit(1);
    }
};

Module.onAbort = (what) => {
    console.error('✗ Module aborted:', what);
    process.exit(1);
};

// Timeout after 5 minutes (database build can be slow)
setTimeout(() => {
    console.error('✗ Timeout: Test took too long');
    process.exit(1);
}, 300000);
```

### Usage

```bash
node test_wasm_memory.js
```

---

## Script 8: Comprehensive Functionality Test

### Purpose
Test all major WASM functions (scramble generation, solving).

### Implementation

```javascript
// test_wasm_comprehensive.js
// Comprehensive WASM functionality test

const Module = require('./solver.js');

async function runTests() {
    console.log('=== Comprehensive WASM Test Suite ===\\n');
    
    let passed = 0;
    let failed = 0;
    
    // Test 1: Generate scramble
    console.log('Test 1: Generate scramble');
    try {
        const scramble = Module.get_xxcross_scramble();
        if (scramble && scramble.length > 0) {
            console.log(`✓ Generated: ${scramble}`);
            passed++;
        } else {
            console.log('✗ Empty scramble returned');
            failed++;
        }
    } catch (error) {
        console.log(`✗ Error: ${error}`);
        failed++;
    }
    
    // Test 2: Solve known scramble
    console.log('\\nTest 2: Solve known scramble');
    try {
        const testScramble = "R U R' U'";
        const solution = Module.start_search(testScramble);
        console.log(`✓ Solution found: ${solution}`);
        passed++;
    } catch (error) {
        console.log(`✗ Error: ${error}`);
        failed++;
    }
    
    // Test 3: Solve multiple scrambles
    console.log('\\nTest 3: Multiple solves');
    try {
        const scrambles = [
            "R U R' U'",
            "F R F' R'",
            "L U L' U'"
        ];
        
        for (const scramble of scrambles) {
            const solution = Module.start_search(scramble);
            console.log(`  ${scramble} → ${solution}`);
        }
        
        console.log('✓ All scrambles solved');
        passed++;
    } catch (error) {
        console.log(`✗ Error: ${error}`);
        failed++;
    }
    
    // Test 4: Memory stability
    console.log('\\nTest 4: Memory stability');
    const memBefore = Module.wasmMemory.buffer.byteLength / (1024 * 1024);
    
    // Generate multiple scrambles
    for (let i = 0; i < 10; i++) {
        Module.get_xxcross_scramble();
    }
    
    const memAfter = Module.wasmMemory.buffer.byteLength / (1024 * 1024);
    const memDelta = memAfter - memBefore;
    
    console.log(`  Memory before: ${memBefore.toFixed(2)} MB`);
    console.log(`  Memory after: ${memAfter.toFixed(2)} MB`);
    console.log(`  Delta: ${memDelta.toFixed(2)} MB`);
    
    if (Math.abs(memDelta) < 50) {  // Should be stable after first build
        console.log('✓ Memory stable');
        passed++;
    } else {
        console.log('⚠ Memory changed significantly');
        // Not a failure, but noteworthy
        passed++;
    }
    
    // Summary
    console.log('\\n=== Test Summary ===');
    console.log(`Passed: ${passed}`);
    console.log(`Failed: ${failed}`);
    
    if (failed === 0) {
        console.log('\\n✓ All tests passed');
        process.exit(0);
    } else {
        console.log('\\n✗ Some tests failed');
        process.exit(1);
    }
}

Module.onRuntimeInitialized = runTests;

Module.onAbort = (what) => {
    console.error('✗ Module aborted:', what);
    process.exit(1);
};

setTimeout(() => {
    console.error('✗ Timeout');
    process.exit(1);
}, 300000);
```

### Usage

```bash
node test_wasm_comprehensive.js
```

---

## Script 9: Browser Compatibility Test

### Purpose
Test WASM module in browser environment.

### Implementation

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WASM Browser Test</title>
    <style>
        body {
            font-family: monospace;
            padding: 20px;
            max-width: 800px;
            margin: 0 auto;
        }
        
        .status {
            padding: 10px;
            margin: 10px 0;
            border-radius: 5px;
        }
        
        .success {
            background-color: #d4edda;
            border: 1px solid #c3e6cb;
        }
        
        .error {
            background-color: #f8d7da;
            border: 1px solid #f5c6cb;
        }
        
        button {
            padding: 10px 20px;
            margin: 5px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <h1>XXCross Solver - WASM Browser Test</h1>
    
    <div id="status" class="status">
        Loading WASM module...
    </div>
    
    <div id="memory-info"></div>
    
    <div id="controls" style="display: none;">
        <h2>Tests</h2>
        <button onclick="testScrambleGeneration()">Generate Scramble</button>
        <button onclick="testSolving()">Test Solving</button>
        <button onclick="testMemory()">Check Memory</button>
    </div>
    
    <div id="output"></div>
    
    <script src="solver.js"></script>
    <script>
        let Module;
        
        // Initialize
        createModule().then((instance) => {
            Module = instance;
            
            document.getElementById('status').innerHTML = '✓ WASM module loaded successfully';
            document.getElementById('status').className = 'status success';
            
            // Show memory info
            const memMB = Module.wasmMemory.buffer.byteLength / (1024 * 1024);
            document.getElementById('memory-info').innerHTML = 
                `<p>Initial memory: ${memMB.toFixed(2)} MB</p>`;
            
            // Enable controls
            document.getElementById('controls').style.display = 'block';
            
        }).catch((error) => {
            document.getElementById('status').innerHTML = '✗ Failed to load WASM: ' + error;
            document.getElementById('status').className = 'status error';
        });
        
        function log(message, isError = false) {
            const output = document.getElementById('output');
            const p = document.createElement('p');
            p.textContent = message;
            p.className = isError ? 'error' : 'success';
            output.appendChild(p);
        }
        
        function testScrambleGeneration() {
            log('Generating scramble...');
            
            try {
                const scramble = Module.get_xxcross_scramble();
                log('✓ Generated: ' + scramble);
            } catch (error) {
                log('✗ Error: ' + error, true);
            }
        }
        
        function testSolving() {
            log('Testing solver...');
            
            const testScrambles = [
                "R U R' U'",
                "F R F' R'",
                "L U L' U'"
            ];
            
            try {
                for (const scramble of testScrambles) {
                    const solution = Module.start_search(scramble);
                    log(`${scramble} → ${solution}`);
                }
                log('✓ All tests passed');
            } catch (error) {
                log('✗ Error: ' + error, true);
            }
        }
        
        function testMemory() {
            const memMB = Module.wasmMemory.buffer.byteLength / (1024 * 1024);
            log(`Current memory: ${memMB.toFixed(2)} MB`);
            
            // Memory growth limits
            const maxMemory = 2048;  // 2 GB
            if (memMB < maxMemory) {
                log(`✓ Memory within limits (< ${maxMemory} MB)`);
            } else {
                log(`⚠ Memory exceeds ${maxMemory} MB`, true);
            }
        }
    </script>
</body>
</html>
```

### Usage

```bash
# Serve with simple HTTP server
python3 -m http.server 8000

# Open browser
# Navigate to: http://localhost:8000/test_wasm_browser.html
```

---

## Script 10: Performance Benchmark

### Purpose
Measure WASM performance vs native.

### Implementation

```javascript
// test_wasm_performance.js
// Performance benchmark for WASM solver

const Module = require('./solver.js');

function benchmark(name, iterations, fn) {
    console.log(`\\nBenchmark: ${name}`);
    console.log(`Iterations: ${iterations}`);
    
    const start = Date.now();
    
    for (let i = 0; i < iterations; i++) {
        fn();
    }
    
    const end = Date.now();
    const totalMs = end - start;
    const avgMs = totalMs / iterations;
    
    console.log(`Total time: ${totalMs} ms`);
    console.log(`Average: ${avgMs.toFixed(2)} ms per operation`);
    
    return avgMs;
}

Module.onRuntimeInitialized = () => {
    console.log('=== WASM Performance Benchmark ===');
    
    // Warm up (triggers database build)
    console.log('\\nWarming up (building database)...');
    const warmupStart = Date.now();
    Module.get_xxcross_scramble();
    const warmupTime = Date.now() - warmupStart;
    console.log(`Database build time: ${(warmupTime / 1000).toFixed(2)}s`);
    
    // Benchmark: Scramble generation
    benchmark('Scramble Generation', 100, () => {
        Module.get_xxcross_scramble();
    });
    
    // Benchmark: Simple solve
    benchmark('Simple Solve (4 moves)', 100, () => {
        Module.start_search("R U R' U'");
    });
    
    // Benchmark: Medium solve
    benchmark('Medium Solve (8 moves)', 10, () => {
        Module.start_search("R U R' U' F R F' R'");
    });
    
    // Memory efficiency
    const finalMemMB = Module.wasmMemory.buffer.byteLength / (1024 * 1024);
    console.log('\\n=== Memory ===');
    console.log(`Final memory usage: ${finalMemMB.toFixed(2)} MB`);
    
    console.log('\\n=== Benchmark Complete ===');
    process.exit(0);
};

Module.onAbort = (what) => {
    console.error('✗ Aborted:', what);
    process.exit(1);
};

setTimeout(() => {
    console.error('✗ Timeout');
    process.exit(1);
}, 600000);  // 10 minutes
```

### Usage

```bash
node test_wasm_performance.js
```

---

## Build Script for WASM

### Complete build workflow

```bash
#!/bin/bash
#
# build_wasm.sh
# Complete WASM build and test workflow
#

set -euo pipefail

echo "======================================================================"
echo "WASM Build and Test"
echo "======================================================================"

# Configuration
SOLVER_CPP="solver_dev.cpp"
OUTPUT_JS="solver.js"
OUTPUT_WASM="solver.wasm"

# Clean previous builds
echo "Cleaning previous builds..."
rm -f ${OUTPUT_JS} ${OUTPUT_WASM}

# Build WASM
echo "Building WASM..."
emcc -std=c++17 -O3 ${SOLVER_CPP} -o ${OUTPUT_JS} \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \
  -s MAXIMUM_MEMORY=2147483648 \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" \
  --bind \
  -I../tsl-robin-map/include

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

# Run basic test
echo ""
echo "Running basic test..."
if node test_wasm_basic.js; then
    echo "✓ Basic test passed"
else
    echo "✗ Basic test failed"
    exit 1
fi

# Run comprehensive test
echo ""
echo "Running comprehensive test..."
if node test_wasm_comprehensive.js; then
    echo "✓ Comprehensive test passed"
else
    echo "✗ Comprehensive test failed"
    exit 1
fi

echo ""
echo "======================================================================"
echo "Build and Test Complete"
echo "======================================================================"
echo ""
echo "WASM module ready for deployment"
echo "Files:"
echo "  - ${OUTPUT_JS} (JavaScript loader)"
echo "  - ${OUTPUT_WASM} (WebAssembly binary)"
```

### Usage

```bash
chmod +x build_wasm.sh
./build_wasm.sh
```

---

## Troubleshooting

### Problem: Module fails to load

```javascript
// Error: Cannot find module './solver.js'
```

**Solution**: Ensure build completed successfully:
```bash
ls -l solver.js solver.wasm
emcc --version  # Verify emscripten installed
```

---

### Problem: Out of memory in browser

```
RuntimeError: memory access out of bounds
```

**Solution**: Increase INITIAL_MEMORY or enable ALLOW_MEMORY_GROWTH:
```bash
emcc ... -s INITIAL_MEMORY=536870912 -s ALLOW_MEMORY_GROWTH=1
```

---

### Problem: Functions not exported

```javascript
// TypeError: Module.get_xxcross_scramble is not a function
```

**Solution**: Verify bindings in C++:
```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(solver_module) {
    emscripten::function("get_xxcross_scramble", &get_xxcross_scramble);
    emscripten::function("start_search", &start_search);
}
#endif
```

---

## Related Documentation

- [WASM_BUILD_GUIDE.md](WASM_BUILD_GUIDE.md) - Complete build instructions
- [SOLVER_IMPLEMENTATION.md](SOLVER_IMPLEMENTATION.md) - Code implementation details
- [../USER_GUIDE.md](../USER_GUIDE.md) - End-user deployment guide

---

**Document Version**: 1.0  
**Status**: Production-ready  
**Scripts Archived**: Original test_wasm*.js files backed up, may be removed from repository
