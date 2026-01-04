#!/usr/bin/env node

/**
 * DESKTOP_ULTRA Opposite - Dedicated 100-trial test
 * Runs separately to avoid memory issues from sequential testing
 */

const fs = require('fs');
const path = require('path');

const createModule = require('./solver_prod.js');

const TEST_DEPTHS = [6, 7, 8, 9, 10];
const TRIALS_PER_DEPTH = 100;

function calculateStats(times) {
    if (times.length === 0) return null;
    
    const sorted = [...times].sort((a, b) => a - b);
    const min = sorted[0];
    const max = sorted[sorted.length - 1];
    const sum = times.reduce((a, b) => a + b, 0);
    const mean = sum / times.length;
    
    const squaredDiffs = times.map(t => Math.pow(t - mean, 2));
    const variance = squaredDiffs.reduce((a, b) => a + b, 0) / times.length;
    const stdDev = Math.sqrt(variance);
    
    const mid = Math.floor(sorted.length / 2);
    const median = sorted.length % 2 === 0 
        ? (sorted[mid - 1] + sorted[mid]) / 2 
        : sorted[mid];
    
    const estimatedRetryCount = mean / min;
    
    return {
        min: min.toFixed(2),
        max: max.toFixed(2),
        mean: mean.toFixed(2),
        median: median.toFixed(2),
        stdDev: stdDev.toFixed(2),
        estimatedRetries: estimatedRetryCount.toFixed(2),
        count: times.length
    };
}

async function testScrambleGeneration(instance, depth, trials) {
    const times = [];
    
    for (let i = 0; i < trials; i++) {
        const startTime = performance.now();
        const result = instance.func("", depth.toString());
        const elapsed = performance.now() - startTime;
        times.push(elapsed);
        
        if (!result || result.length === 0) {
            console.error(`ERROR: Empty result for depth ${depth}, trial ${i + 1}`);
        }
        
        // Progress indicator every 10 trials
        if ((i + 1) % 10 === 0) {
            process.stdout.write('.');
        }
    }
    
    return calculateStats(times);
}

(async () => {
    try {
        console.log('='.repeat(80));
        console.log('DESKTOP_ULTRA Opposite - Dedicated 100-Trial Test');
        console.log('='.repeat(80));
        console.log('');
        
        console.log('[1/3] Initializing WASM module...');
        const Module = await createModule();
        console.log('✓ WASM module initialized');
        console.log('');
        
        console.log('[2/3] Creating DESKTOP_ULTRA Opposite solver...');
        console.log('  This may take 80-100 seconds...');
        const startTime = performance.now();
        const instance = new Module.xxcross_search(false, "DESKTOP_ULTRA");
        const initTime = ((performance.now() - startTime) / 1000).toFixed(1);
        console.log(`✓ Solver initialized in ${initTime}s`);
        console.log('');
        
        console.log('[3/3] Testing scramble generation...');
        console.log(`  Depths: ${TEST_DEPTHS.join(', ')}`);
        console.log(`  Trials per depth: ${TRIALS_PER_DEPTH}`);
        console.log('');
        
        const results = {
            model: 'DESKTOP_ULTRA',
            pairType: 'Opposite',
            initialization: {
                timeMs: Math.round(parseFloat(initTime) * 1000),
                timeSec: initTime
            },
            depths: {}
        };
        
        for (const depth of TEST_DEPTHS) {
            process.stdout.write(`  Depth ${depth} (${TRIALS_PER_DEPTH} trials): `);
            
            const stats = await testScrambleGeneration(instance, depth, TRIALS_PER_DEPTH);
            results.depths[`depth${depth}`] = stats;
            
            console.log(` done`);
            console.log(`    Mean: ${stats.mean}ms | Median: ${stats.median}ms | Retries: ${stats.estimatedRetries}`);
        }
        
        console.log('');
        console.log('='.repeat(80));
        console.log('Test completed successfully!');
        console.log('='.repeat(80));
        
        // Save results
        const outputPath = path.join(__dirname, 'desktop_ultra_opposite_results.json');
        fs.writeFileSync(outputPath, JSON.stringify(results, null, 2));
        console.log(`\nResults saved to: ${outputPath}`);
        
    } catch (error) {
        console.error('\n' + '='.repeat(80));
        console.error('ERROR:', error.message);
        console.error('='.repeat(80));
        console.error(error);
        process.exit(1);
    }
})();
