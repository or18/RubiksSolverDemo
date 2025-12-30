# WebAssembly Build and Development Guide

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md) | [Memory Config](../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [Implementation](SOLVER_IMPLEMENTATION.md) | [WASM Test Scripts](WASM_EXPERIMENT_SCRIPTS.md) | [Memory Monitoring](MEMORY_MONITORING.md)

## Overview
The xxcrossTrainer solver has been successfully compiled to WebAssembly with Emscripten, enabling it to run in browsers and Node.js environments. This guide covers compilation, testing, debugging, and source code management.

---

## Table of Contents

1. [Source Code Versions](#source-code-versions)
2. [Build Configuration](#build-configuration)
3. [Usage Examples](#usage-examples)
4. [Testing and Debugging](#testing-and-debugging)
5. [Debugging Scenarios](#debugging-scenarios)
6. [Performance](#performance)
7. [Troubleshooting](#troubleshooting)

---

## Source Code Versions

This directory contains two versions of the XXCross solver source code:

### Development Version (`solver_dev.cpp`)

**Purpose**: Development, debugging, and performance analysis

**Characteristics**:
- ✅ **All debug prints enabled** (321 std::cout statements)
- ✅ **Verbose output** for memory tracking, phase transitions, and internal state
- ✅ **English comments** throughout
- ✅ **Detailed logging** for:
  - Memory allocations and RSS measurements
  - Bucket calculations and load factors
  - Rehash detection and prevention
  - Phase-by-phase progress
  - Element vector operations
  - Node generation statistics

**Use Cases**:
- Debugging memory issues
- Performance profiling and optimization
- Understanding algorithm behavior
- Development and testing new features

**Compilation**:
```bash
g++ -std=c++17 -O3 -march=native -fdiagnostics-color=always -g solver_dev.cpp -o solver_dev
```

### Production Version (`solver.cpp`)

**Purpose**: Emscripten compilation for WebAssembly deployment

**Characteristics**:
- ✅ **Minimal debug prints** (essential only)
- ✅ **English comments** throughout
- ✅ **Commented out verbose logging** (14 commented std::cout lines)
- ✅ **Kept essential prints**:
  - Critical errors (UNEXPECTED REHASH, warnings)
  - User-facing messages (solutions, results)
  - High-level phase completion
  - Memory budget summary
  - Database construction status

**Use Cases**:
- WebAssembly compilation with Emscripten
- Production deployment
- Browser-based solver
- Minimal console output

### Key Differences

| Aspect | Development (`solver_dev.cpp`) | Production (`solver.cpp`) |
|--------|-------------------------------|---------------------------|
| **Debug Prints** | All enabled (321 lines) | Minimal (14 commented out) |
| **Verbose Blocks** | Active | Mostly commented |
| **RSS Tracking** | Detailed | High-level only |
| **Load Factor Logs** | Every phase | Summary only |
| **Bucket Calculations** | Step-by-step | Final result only |
| **Element Vector Logs** | Attach/detach details | Critical only |
| **Compilation Target** | Native C++ | Emscripten/Wasm |

### Supporting Files

**`expansion_parameters.h`**
Parameter table for depth/bucket size optimization. Contains:
- Effective load factors
- Measured memory factors  
- Backtrace load factors
- Empirically tuned parameters for each (depth, bucket_size) pair

**Note**: Both versions require this header file.

---

## Build Configuration

### Compilation Command
```bash
cd src/xxcrossTrainer
source ~/emsdk/emsdk_env.sh
em++ -I.. solver.cpp -o solver.js -O3 -msimd128 -flto \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=734003200 \
  -s MAXIMUM_MEMORY=2147483648 \
  -s WASM=1 \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s INVOKE_RUN=0 \
  --bind
```

### Memory Settings
- **Initial Memory**: 700MB (734,003,200 bytes) - Mobile friendly
- **Maximum Memory**: 2GB (2,147,483,648 bytes) - Mobile browser compatible
- **Growth**: Enabled for dynamic allocation
- **Safe C++ Memory Limit**: 700MB (auto-capped in WASM environment)

## Usage Examples

### Basic Instance Creation
```javascript
const Module = require('./solver.js');

Module.onRuntimeInitialized = () => {
    // Create instance with default parameters
    const solver = new Module.xxcross_search();
    
    // Solve
    const result = solver.func(scramble, "7");
    console.log(result);
};
```

### Parameterized Constructor
```javascript
// Constructor signature:
// xxcross_search(adj, BFS_DEPTH, MEMORY_LIMIT_MB, verbose)

// Default (1600MB requested, auto-capped to 700MB in WASM)
const solver1 = new Module.xxcross_search();

// Custom memory limit (500MB)
const solver2 = new Module.xxcross_search(true, 6, 500, false);

// Parameters:
// - adj: true for adjacent slots (default), false for opposite
// - BFS_DEPTH: BFS depth limit (default: 6)
// - MEMORY_LIMIT_MB: Memory budget (default: 1600, WASM max: 700)
// - verbose: Enable debug output (default: true, auto-disabled in WASM)
```

### Instance Reuse Pattern (Recommended)
```javascript
let solverInstance = null;

const initPromise = new Promise((resolve) => {
    Module.onRuntimeInitialized = () => {
        // Database is built once here
        solverInstance = new Module.xxcross_search(true, 6, 500);
        resolve();
    };
});

async function solve(scramble, depth) {
    await initPromise;
    // Subsequent calls reuse the database (fast!)
    return solverInstance.func(scramble, depth.toString());
}

// Usage
solve("U R2 F B...", 7).then(result => console.log(result));
solve("D F2 L...", 8).then(result => console.log(result));
```

---

## Testing and Debugging

### Test Script Reference

The xxcrossTrainer includes comprehensive test scripts to validate WASM compilation and functionality. **For complete test implementations and detailed usage, see [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md).**

| Test Script | Purpose | Runtime | Documentation |
|-------------|---------|---------|---------------|
| `test_wasm_minimal.js` | Basic loading test | ~30s | [Script 2](WASM_EXPERIMENT_SCRIPTS.md#script-2-minimal-wasm-load-test) |
| `test_wasm_full.js` | Full solving test | ~60s | [Script 3](WASM_EXPERIMENT_SCRIPTS.md#script-3-full-wasm-solve-test) |
| `test_instance_reuse.js` | Instance reuse pattern | ~60s | [Script 4](WASM_EXPERIMENT_SCRIPTS.md#script-4-instance-reuse-test) |
| `test_parameterized.js` | Memory configurations | ~180s | [Script 5](WASM_EXPERIMENT_SCRIPTS.md#script-5-parameterized-constructor-test) |
| `test_verification.js` | Bug fix verification | ~60s | [Script 6](WASM_EXPERIMENT_SCRIPTS.md#script-6-verification-test-num_list8-fix) |

### Basic Testing Workflow

```bash
cd src/xxcrossTrainer

# 1. Build WASM first
emcc -O3 solver_dev.cpp -o solver.js \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \
  --bind

# 2. Run basic loading test
node test_wasm_minimal.js

# 3. Run full functionality test
node test_wasm_full.js
```

**Note**: All test scripts are archived. Complete implementations are documented in [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md) for recreation if needed.

---

## Common Debugging Scenarios

### Scenario 1: Verify WASM Compilation Works

**Problem**: Want to confirm WASM binary loads correctly  
**Test**: `test_wasm_minimal.js` ([detailed docs](WASM_EXPERIMENT_SCRIPTS.md#script-2-minimal-wasm-load-test))

**Expected**: Module loads, instance created, ~700MB heap allocated

**Use when**:
- After recompiling WASM
- Testing on new platform/browser
- Debugging module loading issues

---

### Scenario 2: Validate Solving Functionality

**Problem**: Need to verify database construction and solving work end-to-end  
**Test**: `test_wasm_full.js` ([detailed docs](WASM_EXPERIMENT_SCRIPTS.md#script-3-full-wasm-solve-test))

**Expected**: Solutions found at different depths, consistent solve times

**Use when**:
- Validating a new build
- Testing after algorithm changes
- Benchmarking solve times

---

### Scenario 3: Check Instance Reuse Pattern

**Problem**: Need to confirm database is built once and reused (worker.js pattern)  
**Test**: `test_instance_reuse.js` ([detailed docs](WASM_EXPERIMENT_SCRIPTS.md#script-4-instance-reuse-test))

**Expected**: Database built once, subsequent solves are fast

**Use when**:
- Implementing worker pattern
- Optimizing application architecture
- Debugging performance issues

---

### Scenario 4: Test Memory Configurations

**Problem**: Need to find optimal memory limit for target platform  
**Test**: `test_parameterized.js` ([detailed docs](WASM_EXPERIMENT_SCRIPTS.md#script-5-parameterized-constructor-test))

**Expected**: Different memory limits (500MB, 600MB, 700MB) all work

**Use when**:
- Optimizing for mobile devices
- Balancing memory vs performance
- Platform-specific tuning

---

### Scenario 5: Verify Bug Fixes

**Problem**: Need to confirm recent fixes are working  
**Test**: `test_verification.js` ([detailed docs](WASM_EXPERIMENT_SCRIPTS.md#script-6-verification-test-num_list8-fix))

**Expected**: 700MB memory cap enforced, num_list[8] initialized correctly

**Use when**:
- After applying bug fixes
- Regression testing
- CI/CD validation

---

## Deployment

### Production Files

After successful build, deploy these files:

```
src/xxcrossTrainer/
├── solver.js         # JavaScript loader (generated by emcc)
├── solver.wasm       # WebAssembly binary (generated by emcc)
└── worker.js         # Worker wrapper (instance reuse pattern)
```

### Integration Example

```javascript
// Load WASM module in worker
const Module = require('./solver.js');

let solverInstance = null;

// Initialize once
Module.onRuntimeInitialized = () => {
    solverInstance = new Module.xxcross_search();
    console.log('Solver ready');
};

// Reuse instance for all solves
function solve(scramble, depth) {
    return solverInstance.func(scramble, depth);
}
```

**Pattern**: This matches the implementation in [worker.js](../../src/xxcrossTrainer/worker.js). See [WASM_EXPERIMENT_SCRIPTS.md Script 4](WASM_EXPERIMENT_SCRIPTS.md#script-4-instance-reuse-test) for detailed explanation.

---

## Troubleshooting

### Build Issues

#### emcc not found

```bash
# Error: emcc: command not found
```

**Solution**: Install Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

---

### Runtime Issues

#### Module fails to load

```javascript
// Error: Cannot find module './solver.js'
```

**Solution**: Verify build completed successfully:
```bash
ls -l solver.js solver.wasm
emcc --version
```

---

#### Out of memory in browser

```
RuntimeError: memory access out of bounds
```

**Solution**: Memory growth is enabled by default. Check browser console for limits. Modern browsers support 2GB WASM memory. If issues persist, reduce memory cap in source:

```cpp
// In solver_dev.cpp
const size_t MAX_TOTAL_MEMORY_MB = 600;  // Lower from 700
```

Then rebuild.

---

#### Functions not exported

```javascript
// TypeError: Module.xxcross_search is not a function
```

**Solution**: Verify Emscripten bindings in source:
```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(solver_module) {
    emscripten::class_<xxcross_search>("xxcross_search")
        .constructor<>()
        .constructor<bool, int, int>()
        .function("func", &xxcross_search::func);
    
    emscripten::function("get_xxcross_scramble", &get_xxcross_scramble);
    emscripten::function("start_search", &start_search);
}
#endif
```

---

### Performance Issues

#### Slow initialization

**Cause**: Database construction time scales with memory limit.

**Solutions**:
1. Use lower memory limit for faster initialization:
   ```javascript
   // 500MB instead of 700MB
   const solver = new Module.xxcross_search(true, 6, 500);
   ```

2. Pre-warm instance on application startup
3. Use Web Worker to avoid blocking UI

See [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md) for detailed patterns and performance benchmarks.

---

## Debugging Scenarios (Native Build)

### Scenario 6: Debug Memory Issues (Native)

**Problem**: Memory allocation or RSS tracking issues  
**Test**: Compile and run `solver_dev.cpp`

**Steps**:
```bash
# Compile development version with full debug output
g++ -std=c++17 -O3 -g solver_dev.cpp -o solver_dev

# Run with specific memory limit
MEMORY_LIMIT_MB=1600 ./solver_dev
```

**What to look for**:
```
=== Memory Budget Analysis ===
Total memory limit: 1600.00 MB
RSS after Phase 1: 120.45 MB
Remaining: 1479.55 MB

[Pre-calculated Bucket Allocation]
Wasm safety margin: 295.91 MB
Usable for buckets: 1183.64 MB
  depth 7 bucket: 33554432 (32M)
  depth 8 bucket: 33554432 (32M)
  depth 9 bucket: 16777216 (16M)

=== Phase 2: Depth 7 Local Expansion ===
Allocated bucket: 33554432
Insert loop: processed 4500000 parents...
Load factor: 89.97%
Final RSS: 348.23 MB

... (detailed phase logs)
```

**Use when**:
- Debugging unexpected memory usage
- Analyzing bucket allocation strategy
- Tracking RSS changes per phase
- Optimizing memory parameters

---

### Scenario 7: Debug Solving Issues (Native)

**Problem**: Solver not finding solutions or returning wrong results  
**Test**: Use `solver_dev.cpp` with verbose output

**Steps**:
```bash
# Compile with debug symbols
g++ -std=c++17 -O3 -g solver_dev.cpp -o solver_dev

# Run with environment variable
MEMORY_LIMIT_MB=1600 ./solver_dev

# In code, test specific scramble:
# Edit main() to call:
//   std::string result = func("U R2 F B...", "7");
//   std::cout << "Result: " << result << std::endl;
```

**Debug output shows**:
- IDA* search depth progression
- Prune table hits
- Node expansion counts
- Solution paths found

**Use when**:
- Verifying solve correctness
- Analyzing search efficiency
- Debugging algorithm changes
- Performance profiling

---

### Scenario 8: Test Platform-Specific Behavior

**Problem**: Different behavior between WASM and native  
**Approach**: Compare outputs

**Native test**:
```bash
g++ -std=c++17 -O3 solver.cpp -o solver_native
MEMORY_LIMIT_MB=700 ./solver_native
```

**WASM test**:
```bash
node test_wasm_full.js
```

**Compare**:
- Solve times (should be similar)
- Database sizes (may differ due to memory limits)
- Solution correctness (should be identical)

**Platform differences**:

| Feature | WASM | Native |
|---------|------|--------|
| `verbose` | Auto-disabled | Can enable |
| `get_rss_kb()` | Returns 0 | Returns actual RSS |
| Memory limit | Capped at 700MB | Full system RAM |
| `main()` execution | Disabled (INVOKE_RUN=0) | Runs normally |

**Use when**:
- Debugging platform-specific issues
- Validating cross-platform behavior
- Testing memory constraints
- Performance comparison

---

### Custom Debugging

**Create custom test script**:
```javascript
// my_debug_test.js
const Module = require('./solver.js');

Module.onRuntimeInitialized = () => {
    // Your custom test scenario
    const solver = new Module.xxcross_search(true, 6, 500);
    
    // Test specific scrambles
    const testCases = [
        "U R2 F B...",
        "D L2 B'...",
        // Add your cases
    ];
    
    testCases.forEach((scramble, i) => {
        console.log(`Test ${i+1}:`);
        const result = solver.func(scramble, "7");
        console.log(`  Result: ${result.substring(0, 50)}...`);
    });
};
```

---

## Performance

### Database Construction
- **Memory Limit 700MB**: ~55 seconds (depth 9, ~38M nodes)
- **Memory Limit 500MB**: ~25 seconds (depth 9, ~19M nodes)
- **Memory Limit <500MB**: May fail or have reduced depth

### Solving (after database built)
- **Depth 7**: ~0.5-0.6 seconds
- **Depth 8**: ~0.5-0.6 seconds
- **Depth 9**: ~0.6-0.8 seconds

Note: Database is built once per instance. Reusing the same instance makes subsequent solves very fast.

## Generated Files
- `solver.js` (~93KB) - JavaScript glue code
- `solver.wasm` (~258KB) - WebAssembly binary

## Recent Fixes (2025-12-29)

### 1. Initial Memory Reduction (800MB → 700MB)
- Reduced from 800MB to 700MB for better mobile compatibility
- Average mobile devices can more reliably allocate 700MB
- Still maintains full depth 9 database capability

### 2. num_list[8] Bug Fix
- **Issue**: `num_list[8]` was incorrectly showing 0 instead of actual node count
- **Cause**: Using `depth_8_nodes.size()` after the set was cleared with `swap()`
- **Fix**: Now uses saved `depth_8_final_size` variable before clearing
- **Result**: All depths now show correct node counts in validation

### 3. Native Compilation Compatibility
- Emscripten headers now conditional (`#ifdef __EMSCRIPTEN__`)
- Both solver.cpp and solver_dev.cpp can compile with g++
- Allows testing and debugging in native environment

## Platform-Specific Behavior

### WebAssembly Environment
- `verbose` automatically disabled (no console spam)
- Memory limit auto-capped at 700MB (safety for 800MB initial)
- `get_rss_kb()` returns 0 (no /proc filesystem)
- `main()` does not run (INVOKE_RUN=0)

### Native Environment (g++)
- Full verbose output available
- Memory limit can use full system RAM
- RSS monitoring works via /proc
- `main()` runs normally

## Memory Recommendations

| Use Case | Initial WASM | C++ Memory Limit | Depth Reached | Nodes |
|----------|--------------|------------------|---------------|-------|
| **Mobile (conservative)** | 700MB | 400-500MB | 8-9 | 11-19M |
| **Desktop (standard)** | 700MB | 600-700MB | 9 | 35-38M |
| **Testing/Debug** | 700MB | 300MB | 7-8 | 4-11M |

## Important Notes

1. **Instance Reuse**: Always reuse the same instance for multiple solves. Creating new instances rebuilds the database (~25-55s overhead).

2. **Memory Growth**: The WASM binary can grow from 800MB up to 2GB dynamically. Most solves stay under 1GB.

3. **Browser Compatibility**: 2GB maximum memory is compatible with most mobile browsers. Desktop browsers can handle 4GB, but we cap at 2GB for portability.

4. **Error Handling**: If construction fails with OOM, reduce MEMORY_LIMIT_MB parameter.

## Testing

Test scripts are provided:
- `test_wasm_minimal.js` - Basic loading test
- `test_wasm_full.js` - Full solving test
- `test_instance_reuse.js` - Instance reuse pattern
- `test_parameterized.js` - Different memory limits

Run with:
```bash
node test_instance_reuse.js
```

## Troubleshooting

### Out of Memory Error

**Symptoms**:
- Module aborted during initialization
- "Cannot enlarge memory arrays" error
- Process killed by OS

**Solutions**:
1. **Reduce memory limit in constructor**:
   ```javascript
   const solver = new Module.xxcross_search(true, 6, 400);  // Try 400MB
   ```

2. **Check INITIAL_MEMORY setting**:
   ```bash
   # Recompile with lower initial memory
   em++ ... -s INITIAL_MEMORY=524288000  # 500MB
   ```

3. **Monitor heap growth**:
   ```javascript
   console.log(`Heap: ${(Module.HEAP8.length / 1024 / 1024).toFixed(2)} MB`);
   ```

---

### Slow Performance

**Symptom**: Solving takes 10+ seconds per call

**Common causes**:
1. **Creating new instance per solve** (rebuilds database each time)
   ```javascript
   // ❌ BAD: Creates new instance every time
   function solve(scramble) {
       const solver = new Module.xxcross_search();  // Rebuilds database!
       return solver.func(scramble, "7");
   }
   
   // ✅ GOOD: Reuse instance
   let solver = null;
   Module.onRuntimeInitialized = () => {
       solver = new Module.xxcross_search();  // Build once
   };
   function solve(scramble) {
       return solver.func(scramble, "7");  // Fast reuse
   }
   ```

2. **main() running automatically**
   - Check compilation: should have `-s INVOKE_RUN=0`
   - This prevents main() from running at module load

3. **Verbose output in WASM**
   - Should be auto-disabled
   - Verify in solver.cpp: `#ifdef __EMSCRIPTEN__` blocks

**Expected performance**:
- Database construction: 25-55s (once per instance)
- Solving: 0.5-0.8s per call

---

### No Results / Empty Output

**Possible causes**:

1. **Depth parameter out of range**:
   ```javascript
   // Check database depth reached
   // Lower memory = lower max depth
   // Try depth 7 first
   const result = solver.func(scramble, "7");
   ```

2. **Incorrect scramble format**:
   ```javascript
   // Use standard notation: U R F L B D (with U' R2 etc.)
   const scramble = "U R2 F B R B2 R U2 L B2";  // Correct
   ```

3. **Database construction failed**:
   - Check console for "UNEXPECTED REHASH" errors
   - Reduce memory limit if seeing errors

---

### Module Initialization Timeout

**Symptom**: `onRuntimeInitialized` never fires

**Solutions**:

1. **Increase timeout**:
   ```javascript
   setTimeout(() => {
       console.error('Timeout');
       process.exit(1);
   }, 120000);  // 120 seconds
   ```

2. **Check for silent errors**:
   ```javascript
   Module.onAbort = (what) => {
       console.error('Aborted:', what);
   };
   ```

3. **Verify file paths**:
   ```javascript
   // Ensure solver.wasm is in same directory as solver.js
   const Module = require('./solver.js');  // Not '../solver.js'
   ```

---

### Platform-Specific Issues

**Browser vs Node.js**:

| Issue | Browser | Node.js |
|-------|---------|---------|
| Module loading | Use `<script>` tag | Use `require()` |
| Memory limits | ~2GB on mobile, ~4GB desktop | System RAM |
| File paths | Relative to HTML | Relative to cwd |
| Console output | Browser console | stdout |

**Mobile browsers**:
- Use 500-700MB memory limit
- Test on actual device (not just simulator)
- Some browsers may limit to 512MB

**Desktop browsers**:
- Can handle 1-2GB comfortably
- Use 700MB for compatibility

---

### Compilation Errors

**Emscripten header issues**:
```cpp
// Ensure conditional includes in solver.cpp
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif
```

**Linking errors**:
```bash
# Ensure --bind flag
em++ ... --bind

# Check for EXPORTED_RUNTIME_METHODS
-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
```

**Memory setting errors**:
```bash
# INITIAL_MEMORY must be multiple of 65536
# Good: 734003200 (700MB rounded)
# Bad: 700000000 (not aligned)
```

---

### Debugging Checklist

When encountering issues, check:

- [ ] Test scripts run: `node test_wasm_minimal.js`
- [ ] Instance reuse pattern implemented
- [ ] Memory limit appropriate for platform
- [ ] INVOKE_RUN=0 in compilation
- [ ] solver.wasm file exists and is accessible
- [ ] Timeout set appropriately (60-120s)
- [ ] Error handlers (onAbort) implemented
- [ ] Scramble format is correct
- [ ] Depth parameter within database range

---

## Maintenance

### Updating the Solver

When making changes to the solver:

1. **Edit development version** (`solver_dev.cpp`):
   ```bash
   # Make changes with all debug output
   vim solver_dev.cpp
   ```

2. **Test native compilation**:
   ```bash
   g++ -std=c++17 -O3 -g solver_dev.cpp -o solver_dev
   MEMORY_LIMIT_MB=1600 ./solver_dev
   ```

3. **Update production version**:
   - Manually sync changes to `solver.cpp`
   - Keep debug prints commented
   - Maintain Emscripten compatibility

4. **Recompile WASM**:
   ```bash
   source ~/emsdk/emsdk_env.sh
   em++ -I.. solver.cpp -o solver.js -O3 -msimd128 -flto \
     -s ALLOW_MEMORY_GROWTH=1 \
     -s INITIAL_MEMORY=734003200 \
     -s MAXIMUM_MEMORY=2147483648 \
     -s WASM=1 \
     -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
     -s INVOKE_RUN=0 \
     --bind
   ```

5. **Run verification tests**:
   ```bash
   node test_verification.js
   node test_instance_reuse.js
   ```

6. **Test different memory configs**:
   ```bash
   node test_parameterized.js
   ```

---

## Related Documentation

- [SOLVER_IMPLEMENTATION.md](SOLVER_IMPLEMENTATION.md) - Comprehensive code implementation walkthrough
- [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md) - Complete test script implementations
- [MEMORY_MONITORING.md](MEMORY_MONITORING.md) - /proc monitoring and RSS analysis
- [EXPERIMENT_SCRIPTS.md](EXPERIMENT_SCRIPTS.md) - Shell scripts for experimentation
- [../USER_GUIDE.md](../USER_GUIDE.md) - End-user deployment and configuration
- [../MEMORY_CONFIGURATION_GUIDE.md](../MEMORY_CONFIGURATION_GUIDE.md) - Memory parameter reference

---

## Summary

**Quick Reference**:

```bash
# Compile WASM (production)
emcc -O3 solver_dev.cpp -o solver.js \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=268435456 \
  --bind

# Compile native (development)
g++ -std=c++17 -O3 -g solver_dev.cpp -o solver_dev

# Test WASM loading
node test_wasm_minimal.js

# Test full functionality
node test_instance_reuse.js

# Test memory configs
node test_parameterized.js
```

**Best Practices**:
1. Always reuse solver instance (build database once)
2. Use 500-700MB for mobile, 700MB+ for desktop
3. Set appropriate timeouts (60-120s for initialization)
4. Implement error handlers (onAbort, onRuntimeInitialized)
5. Test on target platform before deployment

**For complete test implementations, see [WASM_EXPERIMENT_SCRIPTS.md](WASM_EXPERIMENT_SCRIPTS.md).**

---

**Document Version**: 2.0  
**Last Updated**: 2025-12-30  
**Status**: Production-ready (restructured for build focus)
