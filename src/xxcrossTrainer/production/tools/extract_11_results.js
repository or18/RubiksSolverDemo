#!/usr/bin/env node

/**
 * Extract 11 completed test configurations from test_output.log
 * Parses initialization times and depth test results
 */

const fs = require('fs');
const path = require('path');

const logFilePath = path.join(__dirname, 'test_output.log');
const outputPath = path.join(__dirname, 'results_11_configs.json');

const BUCKET_MODELS = [
    'MOBILE_LOW',
    'MOBILE_MIDDLE',
    'MOBILE_HIGH',
    'DESKTOP_STD',
    'DESKTOP_HIGH',
    'DESKTOP_ULTRA'
];

const PAIR_TYPES = ['Adjacent', 'Opposite'];

function parseLogFile() {
    const logContent = fs.readFileSync(logFilePath, 'utf8');
    const lines = logContent.split('\n');
    
    const results = {};
    
    let currentModel = null;
    let currentPairType = null;
    let currentDepth = null;
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        
        // Parse test header: [N/12] Testing: MODEL_NAME - PairType
        const headerMatch = line.match(/\[(\d+)\/12\] Testing: (\w+) - (\w+)/);
        if (headerMatch) {
            const model = headerMatch[2];
            const pairType = headerMatch[3];
            
            currentModel = model;
            currentPairType = pairType;
            
            if (!results[model]) {
                results[model] = {};
            }
            if (!results[model][pairType]) {
                results[model][pairType] = {
                    initialization: null,
                    depths: {}
                };
            }
            continue;
        }
        
        // Parse initialization time: ✓ Initialized in X.Xs
        const initMatch = line.match(/✓ Initialized in ([\d.]+)s/);
        if (initMatch && currentModel && currentPairType) {
            const timeSec = parseFloat(initMatch[1]);
            results[currentModel][currentPairType].initialization = {
                timeMs: Math.round(timeSec * 1000),
                timeSec: timeSec.toFixed(1)
            };
            continue;
        }
        
        // Parse depth test start: Testing depth N (100 trials)...
        const depthStartMatch = line.match(/Testing depth (\d+) \(100 trials\)/);
        if (depthStartMatch && currentModel && currentPairType) {
            currentDepth = parseInt(depthStartMatch[1]);
            continue;
        }
        
        // Parse depth test results: done (avg: X.XXms, retries: X.XX)
        const depthResultMatch = line.match(/done \(avg: ([\d.]+)ms, retries: ([\d.]+)\)/);
        if (depthResultMatch && currentModel && currentPairType && currentDepth) {
            const avg = parseFloat(depthResultMatch[1]);
            const retries = parseFloat(depthResultMatch[2]);
            
            results[currentModel][currentPairType].depths[`depth${currentDepth}`] = {
                mean: avg.toFixed(2),
                estimatedRetries: retries.toFixed(2),
                count: 100
            };
            
            currentDepth = null;  // Reset for next depth
            continue;
        }
    }
    
    return results;
}

function main() {
    console.log('Extracting 11 completed configurations from test_output.log...\n');
    
    const results = parseLogFile();
    
    // Count completed configurations
    let completedCount = 0;
    let totalDepthResults = 0;
    
    console.log('Extracted Results:\n');
    
    for (const model of BUCKET_MODELS) {
        if (!results[model]) continue;
        
        for (const pairType of PAIR_TYPES) {
            if (!results[model][pairType] || !results[model][pairType].initialization) {
                continue;
            }
            
            const config = results[model][pairType];
            const depthCount = Object.keys(config.depths).length;
            
            if (depthCount > 0) {
                completedCount++;
                totalDepthResults += depthCount;
                
                console.log(`${model} - ${pairType}:`);
                console.log(`  Init: ${config.initialization.timeSec}s`);
                console.log(`  Depths tested: ${depthCount}/5`);
                console.log('');
            }
        }
    }
    
    // Save results
    fs.writeFileSync(outputPath, JSON.stringify(results, null, 2));
    
    console.log('='.repeat(80));
    console.log(`Completed configurations: ${completedCount}/11`);
    console.log(`Total depth results: ${totalDepthResults}`);
    console.log(`Output saved to: ${outputPath}`);
    console.log('='.repeat(80));
}

main();
