// worker_test_wasm_browser.js
// Web Worker for xxcross solver database construction and scramble generation

let isInitialized = false;
let solver = null;  // Persistent xxcross_search instance for random scramble generation
let buildLogs = [];  // Collect logs during database build for statistics

// Initialize WASM module (non-MODULARIZE build)
const initPromise = new Promise((resolve, reject) => {
    self.Module = {
        onRuntimeInitialized: () => {
            // Don't create solver instance here - will be created during build
            isInitialized = true;
            self.postMessage({ type: 'initialized' });
            resolve();
        },
        print: function(text) {
            // Collect logs during build
            buildLogs.push(text);
            // Send console output to main thread
            self.postMessage({ type: 'log', message: text });
        },
        printErr: function(text) {
            buildLogs.push('ERROR: ' + text);
            self.postMessage({ type: 'error', message: text });
        },
        onAbort: function(what) {
            const abortMsg = 'ABORT: ' + (what || 'Unknown abort reason');
            buildLogs.push(abortMsg);
            console.error('[Worker] ' + abortMsg);
            self.postMessage({ 
                type: 'error', 
                message: 'WASM aborted: ' + (what || 'Out of memory or assertion failure'),
                logs: buildLogs 
            });
            reject(new Error(abortMsg));
        }
    };
});

// Load WASM binary (non-MODULARIZE)
importScripts('test_solver.js');

// Handle messages from main thread
self.onmessage = async function(event) {
    const { type, data } = event.data;
    
    try {
        await initPromise;
        
        switch(type) {
            case 'build':
                const { buckets, verbose, adj } = data;
                const [b7, b8, b9, b10] = buckets;
                
                self.postMessage({ 
                    type: 'progress', 
                    message: `Building database (${b7}M/${b8}M/${b9}M/${b10}M)...` 
                });
                
                try {
                    // Reset build logs
                    buildLogs = [];
                    
                    // Create xxcross_search instance with CUSTOM BUCKET configuration
                    // New constructor: xxcross_search(bool adj, int b7_mb, int b8_mb, int b9_mb, int b10_mb)
                    const adjacent = adj !== undefined ? adj : true;
                    
                    console.log('[Worker] Creating xxcross_search instance with custom buckets...');
                    console.log('[Worker] Parameters: adj=' + adjacent + ', buckets=' + b7 + 'M/' + b8 + 'M/' + b9 + 'M/' + b10 + 'M');
                    
                    // Use custom bucket constructor (5 params: adj, b7, b8, b9, b10)
                    solver = new self.Module.xxcross_search(adjacent, b7, b8, b9, b10);
                    
                    console.log('[Worker] Instance created successfully');
                    console.log('[Worker] Solver object:', solver);
                    
                    // Parse statistics from build logs
                    const stats = parseStatisticsFromLogs(buildLogs);
                    stats.success = true;
                    stats.message = 'Database built successfully. Solver instance retained.';
                    stats.pair_type = adjacent ? 'adjacent' : 'opposite';
                    stats.logs = buildLogs;  // Include full logs
                    
                    self.postMessage({ 
                        type: 'complete', 
                        stats: stats
                    });
                    
                } catch (error) {
                    console.error('[Worker] Build error:', error);
                    
                    // Extract meaningful error message from exception
                    let errorMessage = 'Unknown error';
                    if (error && typeof error === 'object') {
                        if (error.message) {
                            errorMessage = error.message;
                        } else if (error.name) {
                            errorMessage = error.name;
                        } else {
                            // C++ exception might be a pointer/number
                            errorMessage = 'C++ exception (code: ' + error + ')';
                        }
                    } else if (error !== undefined && error !== null) {
                        errorMessage = String(error);
                    }
                    
                    // Check last log lines for error details
                    const recentLogs = buildLogs.slice(-5);
                    const errorDetails = recentLogs.filter(line => 
                        line.includes('ERROR') || line.includes('ABORT') || 
                        line.includes('exception') || line.includes('failed')
                    ).join('\n');
                    
                    if (errorDetails) {
                        errorMessage += '\n' + errorDetails;
                    }
                    
                    self.postMessage({ 
                        type: 'error', 
                        message: `Build failed: ${errorMessage}`,
                        logs: buildLogs
                    });
                }
                break;
                
            case 'scramble':
                const { depth, pairType } = data;
                
                console.log('[Worker] Scramble request:', { depth, pairType });
                console.log('[Worker] solver exists:', !!solver);
                
                // Check if solver instance exists
                if (!solver) {
                    self.postMessage({ 
                        type: 'error', 
                        message: 'Database must be built before generating scrambles' 
                    });
                    break;
                }
                
                // Validate depth
                if (depth < 1 || depth > 10) {
                    self.postMessage({ 
                        type: 'error', 
                        message: `Invalid depth: ${depth}. Must be 1-10.` 
                    });
                    break;
                }
                
                try {
                    // Use solver.func("", depth_str) for random scramble generation
                    // func returns: start_search("") + "," + get_xxcross_scramble(depth_str)
                    // Since arg_scramble is empty, start_search returns ""
                    // Result format: ",scramble" or "scramble"
                    const result = solver.func("", depth.toString());
                    console.log(`[Worker] func("", "${depth}") raw result:`, result);
                    console.log(`[Worker] result type: ${typeof result}, length: ${result.length}`);
                    
                    // Parse result: split by comma, take last part
                    const parts = result.split(',');
                    console.log(`[Worker] parts after split:`, parts);
                    const scramble = parts.length > 1 ? parts[parts.length - 1].trim() : parts[0].trim();
                    console.log(`[Worker] extracted scramble: "${scramble}", length: ${scramble.length}`);
                    
                    if (!scramble || scramble.length === 0) {
                        self.postMessage({ 
                            type: 'error', 
                            message: `No scramble generated for depth ${depth}` 
                        });
                    } else {
                        // Use C++ helper to calculate move count (avoids JS/C++ vector issues)
                        let moveCount;
                        try {
                            if (typeof solver.get_scramble_length === 'function') {
                                moveCount = solver.get_scramble_length(scramble);
                                console.log('[Worker] C++ get_scramble_length returned:', moveCount);
                            } else {
                                console.warn('[Worker] get_scramble_length not available, using JS fallback');
                                moveCount = scramble.trim().split(/\s+/).length;
                            }
                        } catch (e) {
                            console.error('[Worker] Error calling get_scramble_length:', e);
                            moveCount = scramble.trim().split(/\s+/).length;
                        }
                        
                        const response = { 
                            type: 'scramble', 
                            scramble: scramble,
                            moveCount: moveCount,  // From C++ helper or fallback
                            actualDepth: depth,
                            pairType: pairType || 'adjacent'
                        };
                        console.log('[Worker] Sending scramble response:', response);
                        self.postMessage(response);
                    }
                } catch (error) {
                    self.postMessage({ 
                        type: 'error', 
                        message: `Failed to generate scramble: ${error.message}` 
                    });
                }
                break;
                
            default:
                self.postMessage({ 
                    type: 'error', 
                    message: 'Unknown command: ' + type 
                });
        }
        
    } catch (error) {
        self.postMessage({ 
            type: 'error', 
            message: error.message 
        });
    }
};

// Parse statistics from C++ console logs
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
        // Parse node counts: "depth=7: num_list[7]=1234567, index_pairs[7].size()=1234567"
        const nodeMatch = line.match(/depth=(\d+):\s+num_list\[\d+\]=(\d+)/);
        if (nodeMatch) {
            const depth = parseInt(nodeMatch[1]);
            const count = parseInt(nodeMatch[2]);
            stats.node_counts[depth] = count;
            stats.total_nodes += count;
        }
        
        // Parse heap info: "Final heap: 123.45 MB"
        const heapMatch = line.match(/Final heap:\s+([\d.]+)\s+MB/);
        if (heapMatch) {
            stats.final_heap_mb = parseFloat(heapMatch[1]);
        }
        
        // Parse peak heap: "Peak heap: 123.45 MB"
        const peakMatch = line.match(/Peak heap:\s+([\d.]+)\s+MB/);
        if (peakMatch) {
            stats.peak_heap_mb = parseFloat(peakMatch[1]);
        }
        
        // Parse load factors: "Load factor (d7): 0.75"
        const loadMatch = line.match(/Load factor \(d(\d+)\):\s+([\d.]+)/);
        if (loadMatch) {
            const depth = parseInt(loadMatch[1]);
            const loadFactor = parseFloat(loadMatch[2]);
            stats.load_factors[depth] = loadFactor;
        }
        
        // Parse bucket sizes: "Bucket size (d7): 1.23 MB"
        const bucketMatch = line.match(/Bucket size \(d(\d+)\):\s+([\d.]+)\s+MB/);
        if (bucketMatch) {
            const depth = parseInt(bucketMatch[1]);
            const size = parseFloat(bucketMatch[2]);
            stats.bucket_sizes[depth] = size;
        }
    }
    
    return stats;
}
