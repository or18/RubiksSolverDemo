// Web Worker for Full BFS Mode Testing
importScripts('test_solver.js');

let Module = null;
let solver = null;

function log(message) {
    postMessage({ type: 'log', message: message });
}

function buildDatabase(depth) {
    const startTime = performance.now();
    
    log(`Initializing WASM module...`);
    
    Module = createModule({
        print: function(text) {
            log(text);
        },
        printErr: function(text) {
            log('ERROR: ' + text);
        },
        onRuntimeInitialized: function() {
            try {
                log(`WASM module initialized.`);
                log(`Building database with Full BFS to depth ${depth}...`);
                log(`Configuration: FORCE_FULL_BFS_TO_DEPTH=${depth}, ENABLE_LOCAL_EXPANSION=0`);
                log('');
                
                // Set environment variables for full BFS mode
                Module.ENV = Module.ENV || {};
                Module.ENV['FORCE_FULL_BFS_TO_DEPTH'] = depth.toString();
                Module.ENV['ENABLE_LOCAL_EXPANSION'] = '0';
                Module.ENV['COLLECT_DETAILED_STATISTICS'] = '1';
                
                // Create solver instance
                // Parameters: adjacent=true, memory_limit=304MB, bucket config (not used in full BFS)
                solver = new Module.xxcross_search(true, 304);
                
                const endTime = performance.now();
                const elapsed = ((endTime - startTime) / 1000).toFixed(2);
                
                log('');
                log(`✅ Database built in ${elapsed}s`);
                
                postMessage({ 
                    type: 'result', 
                    result: {
                        depth: depth,
                        elapsed_seconds: elapsed
                    }
                });
                
            } catch (error) {
                log('Exception during database build: ' + error.toString());
                postMessage({ type: 'error', message: error.toString() });
            }
        }
    });
}

onmessage = function(e) {
    if (e.data.command === 'build') {
        buildDatabase(e.data.depth);
    }
};
