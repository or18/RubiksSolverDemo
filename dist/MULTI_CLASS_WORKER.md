# Advanced Pattern: Multi-Class Single Worker

Guide for managing multiple solver classes in a single Web Worker to optimize memory usage.

---

## Overview

When a C++ file contains multiple solver classes (e.g., `CrossSolver` and `XCrossSolver`), you can instantiate and manage them within a single Web Worker instead of creating separate workers for each class.

**Benefits**:
- **Memory Efficiency**: Share WASM module memory across classes
- **Reduced Overhead**: One module load instead of multiple
- **Simplified Management**: Single Worker postMessage interface

**Trade-offs**:
- **Higher Total Memory**: Combined memory of all class instances
- **Shared Worker**: One class blocking affects all classes in that worker

---

## When to Use This Pattern

### ✅ **Use Single Worker** when:
- Multiple classes are compiled into one WASM module
- Classes share similar prune tables or data structures
- Total combined memory < browser/Node.js limit (~2GB per worker)
- You need to solve with multiple classes sequentially or concurrently

### ❌ **Use Separate Workers** when:
- Each class requires >1GB memory (risk of hitting worker memory limit)
- You need true parallel execution (multiple CPU cores)
- Classes are completely independent and rarely used together
- Isolating one class failure from affecting others is critical

---

## Example Implementation

### C++ Side (solver.cpp)

```cpp
#include <emscripten/bind.h>
#include <string>
#include <vector>

// Example: Cross solver (simpler, smaller prune table)
class CrossSolver {
private:
    std::vector<int> crossPruneTable;  // ~10MB
    bool tableInitialized = false;

public:
    CrossSolver() {
        // Constructor
    }

    void solve(const std::string& scramble, int maxSolutions, int maxLength) {
        if (!tableInitialized) {
            // Initialize prune table (~1-2s, 10MB)
            crossPruneTable.resize(1000000);
            tableInitialized = true;
        }
        
        // Solve logic...
        postMessage("Cross solution: U R F");
    }
};

// Example: XCross solver (more complex, larger prune table)
class XCrossSolver {
private:
    std::vector<int> xcrossPruneTable;  // ~100MB
    bool tableInitialized = false;

public:
    XCrossSolver() {
        // Constructor
    }

    void solve(const std::string& scramble, int maxSolutions, int maxLength) {
        if (!tableInitialized) {
            // Initialize prune table (~5-10s, 100MB)
            xcrossPruneTable.resize(10000000);
            tableInitialized = true;
        }
        
        // Solve logic...
        postMessage("XCross solution: U R F D");
    }
};

// Export both classes
EMSCRIPTEN_BINDINGS(solver) {
    emscripten::class_<CrossSolver>("CrossSolver")
        .constructor<>()
        .function("solve", &CrossSolver::solve);
    
    emscripten::class_<XCrossSolver>("XCrossSolver")
        .constructor<>()
        .function("solve", &XCrossSolver::solve);
}
```

### JavaScript Worker Side (worker_multi_solver.js)

```javascript
// worker_multi_solver.js
// Manages both CrossSolver and XCrossSolver in a single worker

importScripts('solver.js');

let crossSolver = null;
let xcrossSolver = null;
let moduleReady = false;

// Load WASM module once
createSolverModule().then(Module => {
    // Create instances of both classes
    crossSolver = new Module.CrossSolver();
    xcrossSolver = new Module.XCrossSolver();
    moduleReady = true;
    
    postMessage({ 
        type: 'ready', 
        message: 'Both CrossSolver and XCrossSolver initialized' 
    });
}).catch(err => {
    postMessage({ 
        type: 'error', 
        message: 'Failed to load solver module: ' + err 
    });
});

// Handle messages from main thread
self.onmessage = function(e) {
    if (!moduleReady) {
        postMessage({ 
            type: 'error', 
            message: 'Module not ready yet' 
        });
        return;
    }
    
    const { solverType, action, scramble, maxSolutions, maxLength } = e.data;
    
    try {
        if (solverType === 'cross') {
            if (action === 'solve') {
                crossSolver.solve(scramble, maxSolutions, maxLength);
            }
        } else if (solverType === 'xcross') {
            if (action === 'solve') {
                xcrossSolver.solve(scramble, maxSolutions, maxLength);
            }
        } else {
            postMessage({ 
                type: 'error', 
                message: 'Unknown solver type: ' + solverType 
            });
        }
    } catch (err) {
        postMessage({ 
            type: 'error', 
            message: err.toString() 
        });
    }
};

// Override postMessage for C++ output
self.postMessage = function(msg) {
    // Relay messages from C++ to main thread
    self.postMessage({ type: 'solution', data: msg });
};
```

### Main Thread Side (app.js)

```javascript
// app.js
// Use single worker for both solvers

class MultiSolverManager {
    constructor() {
        this.worker = new Worker('worker_multi_solver.js');
        this.ready = false;
        this.messageHandlers = {
            cross: null,
            xcross: null
        };
        
        this.worker.onmessage = (e) => {
            const { type, data, message } = e.data;
            
            if (type === 'ready') {
                this.ready = true;
                console.log(message);
            } else if (type === 'solution') {
                // Determine which solver sent this (based on context)
                // You may need to add solver identification in the message
                console.log(data);
            } else if (type === 'error') {
                console.error('Worker error:', message);
            }
        };
    }
    
    solveCross(scramble, maxSolutions = 3, maxLength = 8) {
        if (!this.ready) {
            console.error('Worker not ready');
            return Promise.reject('Worker not ready');
        }
        
        return new Promise((resolve, reject) => {
            const solutions = [];
            
            this.messageHandlers.cross = (data) => {
                if (data.startsWith('Cross solution:')) {
                    solutions.push(data);
                } else if (data === 'Search finished.') {
                    resolve(solutions);
                }
            };
            
            this.worker.postMessage({
                solverType: 'cross',
                action: 'solve',
                scramble,
                maxSolutions,
                maxLength
            });
        });
    }
    
    solveXCross(scramble, maxSolutions = 3, maxLength = 11) {
        if (!this.ready) {
            console.error('Worker not ready');
            return Promise.reject('Worker not ready');
        }
        
        return new Promise((resolve, reject) => {
            const solutions = [];
            
            this.messageHandlers.xcross = (data) => {
                if (data.startsWith('XCross solution:')) {
                    solutions.push(data);
                } else if (data === 'Search finished.') {
                    resolve(solutions);
                }
            };
            
            this.worker.postMessage({
                solverType: 'xcross',
                action: 'solve',
                scramble,
                maxSolutions,
                maxLength
            });
        });
    }
    
    terminate() {
        this.worker.terminate();
    }
}

// Usage
const manager = new MultiSolverManager();

// Wait for worker to be ready (or use event listener)
setTimeout(async () => {
    // Solve cross
    const crossSolutions = await manager.solveCross("R U R' U'");
    console.log('Cross solutions:', crossSolutions);
    
    // Solve xcross (reusing same worker, different class instance)
    const xcrossSolutions = await manager.solveXCross("R U R' U' F D");
    console.log('XCross solutions:', xcrossSolutions);
}, 2000);
```

---

## Memory Considerations

### Example Memory Usage

Assuming:
- CrossSolver prune table: ~10MB
- XCrossSolver prune table: ~100MB
- WASM module overhead: ~50MB

**Single Worker Approach**:
- Total: ~10MB + ~100MB + ~50MB = **~160MB** per worker
- One worker handles both solvers

**Separate Workers Approach**:
- Worker 1 (Cross): ~10MB + ~50MB = ~60MB
- Worker 2 (XCross): ~100MB + ~50MB = ~150MB
- Total: **~210MB** (two workers)

**Savings**: ~50MB (23% reduction) by sharing WASM module overhead

### Memory Limits

- **Browser Worker**: Typically ~2GB per worker
- **Node.js Worker**: `--max-old-space-size` flag (default ~2GB)
- **WASM Memory**: Can grow dynamically up to `maximumMemoryPages`

If combined memory of all class instances + overhead > ~1.5GB, consider separate workers.

---

## Advanced: Request ID Pattern

For better message routing, use request IDs:

### Worker (worker_multi_solver.js)

```javascript
self.onmessage = function(e) {
    const { requestId, solverType, action, ...params } = e.data;
    
    // Store requestId for this solve session
    self.currentRequestId = requestId;
    self.currentSolverType = solverType;
    
    // Solve...
};

// Override postMessage to include requestId
const originalPostMessage = self.postMessage;
self.postMessage = function(msg) {
    originalPostMessage({
        type: 'solution',
        requestId: self.currentRequestId,
        solverType: self.currentSolverType,
        data: msg
    });
};
```

### Main Thread

```javascript
let nextRequestId = 1;

function solveCross(scramble) {
    const requestId = nextRequestId++;
    
    return new Promise((resolve) => {
        const solutions = [];
        
        const handler = (e) => {
            if (e.data.requestId === requestId) {
                if (e.data.type === 'solution') {
                    solutions.push(e.data.data);
                } else if (e.data.data === 'Search finished.') {
                    worker.removeEventListener('message', handler);
                    resolve(solutions);
                }
            }
        };
        
        worker.addEventListener('message', handler);
        worker.postMessage({
            requestId,
            solverType: 'cross',
            action: 'solve',
            scramble
        });
    });
}
```

---

## Compilation

Compile with MODULARIZE to export both classes:

```bash
emcc solver.cpp -o solver.js \
  --bind \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="'createSolverModule'" \
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -O3
```

---

## Best Practices

1. **Initialize Once**: Create class instances when worker loads, not per-solve
2. **Clear Identification**: Use `requestId` or `solverType` to route messages
3. **Error Handling**: Catch errors per-solver, don't let one crash the worker
4. **Memory Monitoring**: Use `performance.memory` to track usage
5. **Graceful Degradation**: Fall back to separate workers if memory is tight

---

## When NOT to Use This Pattern

❌ **Avoid if**:
- Total memory > 1.5GB (risk hitting worker limit)
- Need true CPU parallelism (multiple cores)
- Classes have conflicting dependencies or initialization
- One solver is critical and must be isolated from failures

In these cases, use separate workers with independent WASM modules.

---

## Summary

**Single Worker Multi-Class**:
- ✅ Memory efficient (shared module overhead)
- ✅ Simpler management (one worker)
- ⚠️ Higher total memory per worker
- ⚠️ One class can block others

**Recommendation**: Use for 2-4 related solver classes with combined memory < 1.5GB.

For the current 2x2 solver (single `PersistentSolver2x2` class), this pattern is not needed. However, future cross/xcross/xxcross solvers could benefit from this approach.
