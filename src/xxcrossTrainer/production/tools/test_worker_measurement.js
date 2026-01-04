#!/usr/bin/env node

/**
 * XXcross Solver - Production Performance Measurement
 * 
 * Enhanced version with:
 * - 100 trials per depth (statistically significant)
 * - Depth 6-10 only (meaningful measurement)
 * - Statistical analysis (mean, std dev, min, max)
 * - Estimated retry count based on fastest time
 * 
 * Usage: node test_worker_measurement.js
 * Output: performance_results_detailed.md, performance_results_detailed.json
 */

const fs = require('fs');
const path = require('path');

// Load WASM module (MODULARIZE=1, so it exports a factory function)
const createModule = require('./solver_prod.js');

// Wait for WASM initialization
async function initializeModule() {
    console.log('Initializing WASM module...');
    const Module = await createModule();
    console.log('WASM module initialized successfully');
    return Module;
}

// Bucket models configuration
const BUCKET_MODELS = [
    { name: 'MOBILE_LOW', memory: 618 },
    { name: 'MOBILE_MIDDLE', memory: 896 },
    { name: 'MOBILE_HIGH', memory: 1070 },
    { name: 'DESKTOP_STD', memory: 1512 },
    { name: 'DESKTOP_HIGH', memory: 2786 },
    { name: 'DESKTOP_ULTRA', memory: 2884 }
];

const PAIR_TYPES = [
    { name: 'Adjacent', adj: true },
    { name: 'Opposite', adj: false }
];

// Test depths (6-10 only for meaningful measurement)
const TEST_DEPTHS = [6, 7, 8, 9, 10];
const TRIALS_PER_DEPTH = 100; // Increased for statistical significance

// Calculate statistics
function calculateStats(times) {
    if (times.length === 0) return null;
    
    const sorted = [...times].sort((a, b) => a - b);
    const min = sorted[0];
    const max = sorted[sorted.length - 1];
    const sum = times.reduce((a, b) => a + b, 0);
    const mean = sum / times.length;
    
    // Standard deviation
    const squaredDiffs = times.map(t => Math.pow(t - mean, 2));
    const variance = squaredDiffs.reduce((a, b) => a + b, 0) / times.length;
    const stdDev = Math.sqrt(variance);
    
    // Median
    const mid = Math.floor(sorted.length / 2);
    const median = sorted.length % 2 === 0 
        ? (sorted[mid - 1] + sorted[mid]) / 2 
        : sorted[mid];
    
    // Estimate retry count (assuming fastest time is 1 retry)
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

// Test scramble generation for a specific configuration
async function testScrambleGeneration(instance, depth, trials) {
    const times = [];
    
    for (let i = 0; i < trials; i++) {
        const startTime = performance.now();
        const result = instance.func("", depth.toString());
        const elapsed = performance.now() - startTime;
        times.push(elapsed);
        
        // Verify result exists
        if (!result || result.length === 0) {
            console.error(`ERROR: Empty result for depth ${depth}, trial ${i + 1}`);
        }
    }
    
    return calculateStats(times);
}

// Main test runner
async function runPerformanceTests() {
    console.log('='.repeat(80));
    console.log('XXcross Solver - Enhanced Performance Measurement');
    console.log('='.repeat(80));
    console.log(`Test Configuration:`);
    console.log(`  - Depths: ${TEST_DEPTHS.join(', ')}`);
    console.log(`  - Trials per depth: ${TRIALS_PER_DEPTH}`);
    console.log(`  - Bucket models: ${BUCKET_MODELS.length}`);
    console.log(`  - Pair types: ${PAIR_TYPES.length}`);
    console.log(`  - Total tests: ${BUCKET_MODELS.length * PAIR_TYPES.length * TEST_DEPTHS.length * TRIALS_PER_DEPTH}`);
    console.log('='.repeat(80));
    console.log('');
    
    // Initialize WASM module
    const Module = await initializeModule();
    console.log('');
    
    const results = {
        testDate: new Date().toISOString(),
        environment: 'Node.js (Direct WASM Module)',
        optimization: '-O3 -msimd128 -flto',
        testConfig: {
            depths: TEST_DEPTHS,
            trialsPerDepth: TRIALS_PER_DEPTH
        },
        initialization: {},
        scrambleGeneration: {}
    };
    
    let testCount = 0;
    const totalTests = BUCKET_MODELS.length * PAIR_TYPES.length;
    
    for (const model of BUCKET_MODELS) {
        results.initialization[model.name] = {};
        results.scrambleGeneration[model.name] = {};
        
        for (const pairType of PAIR_TYPES) {
            testCount++;
            console.log(`\n[${testCount}/${totalTests}] Testing: ${model.name} - ${pairType.name}`);
            console.log('-'.repeat(80));
            
            // Measure initialization time
            console.log('  Initializing solver (database construction)...');
            const initStart = performance.now();
            const instance = new Module.xxcross_search(pairType.adj, model.name);
            const initTime = performance.now() - initStart;
            console.log(`  ✓ Initialized in ${(initTime / 1000).toFixed(1)}s`);
            
            results.initialization[model.name][pairType.name] = {
                timeMs: Math.round(initTime),
                timeSec: (initTime / 1000).toFixed(1)
            };
            
            // Test each depth
            results.scrambleGeneration[model.name][pairType.name] = {};
            
            for (const depth of TEST_DEPTHS) {
                process.stdout.write(`  Testing depth ${depth} (${TRIALS_PER_DEPTH} trials)... `);
                
                const stats = await testScrambleGeneration(instance, depth, TRIALS_PER_DEPTH);
                results.scrambleGeneration[model.name][pairType.name][`depth${depth}`] = stats;
                
                console.log(`done (avg: ${stats.mean}ms, retries: ${stats.estimatedRetries})`);
            }
        }
    }
    
    console.log('\n' + '='.repeat(80));
    console.log('All tests completed!');
    console.log('='.repeat(80));
    
    return results;
}

// Generate Markdown report
function generateMarkdownReport(results) {
    let md = `# XXcross Solver - Enhanced Performance Analysis\n\n`;
    md += `**Test Date**: ${results.testDate}\n\n`;
    md += `**Environment**: ${results.environment}\n\n`;
    md += `**Optimization**: ${results.optimization}\n\n`;
    md += `**Test Configuration**:\n`;
    md += `- Depths tested: ${results.testConfig.depths.join(', ')}\n`;
    md += `- Trials per depth: ${results.testConfig.trialsPerDepth}\n`;
    md += `- Total trials per configuration: ${results.testConfig.trialsPerDepth * results.testConfig.depths.length}\n\n`;
    
    md += `---\n\n`;
    
    // Initialization times
    md += `## Initialization Time (Database Construction + First Scramble)\n\n`;
    md += `| Bucket Model | Adjacent (s) | Opposite (s) | Dual Total (s) | Memory (MB) |\n`;
    md += `|--------------|--------------|--------------|----------------|-------------|\n`;
    
    for (const model of BUCKET_MODELS) {
        const adjTime = (results.initialization[model.name].Adjacent.timeMs / 1000).toFixed(1);
        const oppTime = (results.initialization[model.name].Opposite.timeMs / 1000).toFixed(1);
        const totalTime = (parseFloat(adjTime) + parseFloat(oppTime)).toFixed(1);
        md += `| ${model.name} | ${adjTime} | ${oppTime} | ${totalTime} | ${model.memory} |\n`;
    }
    
    md += `\n---\n\n`;
    
    // Summary table for each depth
    for (const depth of results.testConfig.depths) {
        md += `## Depth ${depth} Performance Summary\n\n`;
        md += `| Bucket Model | Pair Type | Mean (ms) | Median (ms) | Std Dev (ms) | Min-Max (ms) | Est. Retries |\n`;
        md += `|--------------|-----------|-----------|-------------|--------------|--------------|-------------|\n`;
        
        for (const model of BUCKET_MODELS) {
            for (const pairType of PAIR_TYPES) {
                const stats = results.scrambleGeneration[model.name][pairType.name][`depth${depth}`];
                md += `| ${model.name} | ${pairType.name} | ${stats.mean} | ${stats.median} | ${stats.stdDev} | ${stats.min}-${stats.max} | ${stats.estimatedRetries} |\n`;
            }
        }
        
        md += `\n`;
    }
    
    md += `---\n\n`;
    
    // Key findings
    md += `## Key Findings\n\n`;
    
    // Find best/worst for depth 10
    let bestDepth10 = { mean: Infinity, config: '' };
    let worstDepth10 = { mean: 0, config: '' };
    
    for (const model of BUCKET_MODELS) {
        for (const pairType of PAIR_TYPES) {
            const stats = results.scrambleGeneration[model.name][pairType.name]['depth10'];
            const mean = parseFloat(stats.mean);
            
            if (mean < bestDepth10.mean) {
                bestDepth10 = { mean, config: `${model.name} ${pairType.name}` };
            }
            if (mean > worstDepth10.mean) {
                worstDepth10 = { mean, config: `${model.name} ${pairType.name}` };
            }
        }
    }
    
    md += `### Depth 10 Performance\n\n`;
    md += `- **Fastest average**: ${bestDepth10.config} (${bestDepth10.mean.toFixed(2)}ms)\n`;
    md += `- **Slowest average**: ${worstDepth10.config} (${worstDepth10.mean.toFixed(2)}ms)\n\n`;
    
    md += `### Estimated Retry Counts\n\n`;
    md += `The "Est. Retries" column shows the estimated number of depth guarantee attempts, `;
    md += `calculated as (mean time / min time). Values close to 1.0 indicate the depth guarantee `;
    md += `succeeds on the first attempt most of the time.\n\n`;
    
    // Analyze retry patterns
    md += `**Retry Pattern Analysis**:\n\n`;
    for (const depth of results.testConfig.depths) {
        const retries = [];
        for (const model of BUCKET_MODELS) {
            for (const pairType of PAIR_TYPES) {
                const stats = results.scrambleGeneration[model.name][pairType.name][`depth${depth}`];
                retries.push(parseFloat(stats.estimatedRetries));
            }
        }
        const avgRetries = (retries.reduce((a, b) => a + b, 0) / retries.length).toFixed(2);
        md += `- **Depth ${depth}**: Average ${avgRetries} retries across all configurations\n`;
    }
    
    return md;
}

// Main execution
(async () => {
    try {
        const results = await runPerformanceTests();
        
        // Save JSON results
        const jsonPath = path.join(__dirname, 'performance_results_detailed.json');
        fs.writeFileSync(jsonPath, JSON.stringify(results, null, 2));
        console.log(`\nJSON results saved to: ${jsonPath}`);
        
        // Generate and save markdown report
        const markdown = generateMarkdownReport(results);
        const mdPath = path.join(__dirname, 'performance_results_detailed.md');
        fs.writeFileSync(mdPath, markdown);
        console.log(`Markdown report saved to: ${mdPath}`);
        
        console.log('\n✓ All tests completed successfully!');
        
    } catch (error) {
        console.error('Error during testing:', error);
        process.exit(1);
    }
})();
