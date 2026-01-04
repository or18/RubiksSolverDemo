#!/usr/bin/env node

/**
 * Extract completed test results from test_output.log
 * Parses initialization times and scramble generation statistics
 */

const fs = require('fs');

const logContent = fs.readFileSync('test_output.log', 'utf-8');
const lines = logContent.split('\n');

const results = {
    testDate: new Date().toISOString(),
    environment: 'Node.js (Direct WASM Module)',
    optimization: '-O3 -msimd128 -flto',
    testConfig: {
        depths: [6, 7, 8, 9, 10],
        trialsPerDepth: 100
    },
    initialization: {},
    scrambleGeneration: {}
};

const BUCKET_MODELS = ['MOBILE_LOW', 'MOBILE_MIDDLE', 'MOBILE_HIGH', 'DESKTOP_STD', 'DESKTOP_HIGH', 'DESKTOP_ULTRA'];
const PAIR_TYPES = ['Adjacent', 'Opposite'];

// Parse test sections
let currentModel = null;
let currentPairType = null;
let inInitialization = false;

for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    
    // Detect test section start
    const testMatch = line.match(/\[(\d+)\/12\] Testing: ([A-Z_]+) - (Adjacent|Opposite)/);
    if (testMatch) {
        currentModel = testMatch[2];
        currentPairType = testMatch[3];
        
        if (!results.initialization[currentModel]) {
            results.initialization[currentModel] = {};
            results.scrambleGeneration[currentModel] = {};
        }
        
        inInitialization = true;
        continue;
    }
    
    // Parse initialization time
    if (line.includes('✓ Initialized in')) {
        const timeMatch = line.match(/✓ Initialized in ([\d.]+)s/);
        if (timeMatch && currentModel && currentPairType) {
            const timeSec = parseFloat(timeMatch[1]);
            results.initialization[currentModel][currentPairType] = {
                timeMs: Math.round(timeSec * 1000),
                timeSec: timeSec.toFixed(1)
            };
            inInitialization = false;
        }
    }
    
    // Parse depth test results
    const depthMatch = line.match(/Testing depth (\d+) \(100 trials\)\.\.\. done \(avg: ([\d.]+)ms, retries: ([\d.]+)\)/);
    if (depthMatch && currentModel && currentPairType) {
        const depth = parseInt(depthMatch[1]);
        const avgTime = parseFloat(depthMatch[2]);
        const retries = parseFloat(depthMatch[3]);
        
        if (!results.scrambleGeneration[currentModel][currentPairType]) {
            results.scrambleGeneration[currentModel][currentPairType] = {};
        }
        
        // We need to find min/max/median/stdDev from the log
        // For now, store what we have
        results.scrambleGeneration[currentModel][currentPairType][`depth${depth}`] = {
            mean: avgTime.toFixed(2),
            estimatedRetries: retries.toFixed(2),
            count: 100
        };
    }
}

// Count completed configurations
let completedCount = 0;
for (const model of BUCKET_MODELS) {
    if (results.initialization[model]) {
        for (const pairType of PAIR_TYPES) {
            if (results.initialization[model][pairType]) {
                completedCount++;
            }
        }
    }
}

console.log(`Extracted ${completedCount}/12 completed configurations`);
console.log('');

// Display what we got
for (const model of BUCKET_MODELS) {
    if (results.initialization[model]) {
        console.log(`${model}:`);
        for (const pairType of PAIR_TYPES) {
            if (results.initialization[model][pairType]) {
                const initTime = results.initialization[model][pairType].timeSec;
                const depthCount = Object.keys(results.scrambleGeneration[model][pairType] || {}).length;
                console.log(`  ${pairType}: init ${initTime}s, ${depthCount} depths tested`);
            }
        }
    }
}

// Save results
fs.writeFileSync('results_partial.json', JSON.stringify(results, null, 2));
console.log('\nSaved to results_partial.json');
