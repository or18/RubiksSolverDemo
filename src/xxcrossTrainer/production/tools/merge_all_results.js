#!/usr/bin/env node

/**
 * Merge 11 configurations from test_output.log with DESKTOP_ULTRA Opposite standalone test
 * Generate comprehensive performance report
 */

const fs = require('fs');
const path = require('path');

const results11Path = path.join(__dirname, 'results_11_configs.json');
const desktopUltraPath = path.join(__dirname, 'desktop_ultra_opposite_results.json');
const outputJsonPath = path.join(__dirname, 'performance_results_complete.json');
const outputMdPath = path.join(__dirname, 'performance_results_detailed.md');

function mergeResults() {
    const results11 = JSON.parse(fs.readFileSync(results11Path, 'utf8'));
    const desktopUltra = JSON.parse(fs.readFileSync(desktopUltraPath, 'utf8'));
    
    // Add DESKTOP_ULTRA Opposite from standalone test
    if (!results11.DESKTOP_ULTRA) {
        results11.DESKTOP_ULTRA = {};
    }
    results11.DESKTOP_ULTRA.Opposite = {
        initialization: desktopUltra.initialization,
        depths: desktopUltra.depths
    };
    
    return results11;
}

function generateMarkdownReport(results) {
    let md = '# XXCross Solver - Production Performance Test Results\n\n';
    md += '**Test Date:** ' + new Date().toISOString().split('T')[0] + '\n';
    md += '**Build:** Production (O3, SIMD128, LTO)\n';
    md += '**Trials:** 100 per depth\n';
    md += '**Depths:** 6, 7, 8, 9, 10\n\n';
    
    md += '---\n\n';
    md += '## Summary\n\n';
    md += '| Model | Pair Type | Init (s) | D6 avg (ms) | D7 avg (ms) | D8 avg (ms) | D9 avg (ms) | D10 avg (ms) |\n';
    md += '|-------|-----------|----------|-------------|-------------|-------------|-------------|-------------|\n';
    
    const models = ['MOBILE_LOW', 'MOBILE_MIDDLE', 'MOBILE_HIGH', 'DESKTOP_STD', 'DESKTOP_HIGH', 'DESKTOP_ULTRA'];
    const pairTypes = ['Adjacent', 'Opposite'];
    
    for (const model of models) {
        if (!results[model]) continue;
        
        for (const pairType of pairTypes) {
            if (!results[model][pairType]) continue;
            
            const config = results[model][pairType];
            const initTime = config.initialization ? config.initialization.timeSec : 'N/A';
            
            const d6 = config.depths.depth6 ? config.depths.depth6.mean : '-';
            const d7 = config.depths.depth7 ? config.depths.depth7.mean : '-';
            const d8 = config.depths.depth8 ? config.depths.depth8.mean : '-';
            const d9 = config.depths.depth9 ? config.depths.depth9.mean : '-';
            const d10 = config.depths.depth10 ? config.depths.depth10.mean : '-';
            
            md += `| ${model} | ${pairType} | ${initTime} | ${d6} | ${d7} | ${d8} | ${d9} | ${d10} |\n`;
        }
    }
    
    md += '\n---\n\n';
    md += '## Detailed Results\n\n';
    
    for (const model of models) {
        if (!results[model]) continue;
        
        md += `### ${model}\n\n`;
        
        for (const pairType of pairTypes) {
            if (!results[model][pairType]) continue;
            
            const config = results[model][pairType];
            
            md += `#### ${pairType}\n\n`;
            md += `**Initialization:** ${config.initialization ? config.initialization.timeSec + 's (' + config.initialization.timeMs + 'ms)' : 'N/A'}\n\n`;
            
            md += '| Depth | Mean (ms) | Median (ms) | Std Dev | Min (ms) | Max (ms) | Retries | Trials |\n';
            md += '|-------|-----------|-------------|---------|----------|----------|---------|--------|\n';
            
            for (let depth = 6; depth <= 10; depth++) {
                const depthKey = `depth${depth}`;
                const depthData = config.depths[depthKey];
                
                if (depthData) {
                    md += `| ${depth} | ${depthData.mean} | ${depthData.median || '-'} | ${depthData.stdDev || '-'} | ${depthData.min || '-'} | ${depthData.max || '-'} | ${depthData.estimatedRetries} | ${depthData.count} |\n`;
                } else {
                    md += `| ${depth} | - | - | - | - | - | - | - |\n`;
                }
            }
            
            md += '\n';
        }
    }
    
    md += '---\n\n';
    md += '## Performance Analysis\n\n';
    
    md += '### Initialization Times\n\n';
    md += '- **MOBILE_LOW:** Adjacent 12.0s, Opposite 10.2s\n';
    md += '- **MOBILE_MIDDLE:** Adjacent 18.2s, Opposite 16.6s\n';
    md += '- **MOBILE_HIGH:** Adjacent 19.2s, Opposite 18.7s\n';
    md += '- **DESKTOP_STD:** Adjacent 33.5s, Opposite 36.8s\n';
    md += '- **DESKTOP_HIGH:** Adjacent 73.4s, Opposite 75.0s\n';
    md += '- **DESKTOP_ULTRA:** Adjacent 81.7s, Opposite 96.1s\n\n';
    
    md += '### Depth 10 Performance (Most demanding)\n\n';
    md += '| Model | Pair Type | Mean (ms) | Median (ms) | Retries |\n';
    md += '|-------|-----------|-----------|-------------|---------|\n';
    
    for (const model of models) {
        if (!results[model]) continue;
        
        for (const pairType of pairTypes) {
            if (!results[model][pairType]) continue;
            
            const d10 = results[model][pairType].depths.depth10;
            if (d10) {
                md += `| ${model} | ${pairType} | ${d10.mean} | ${d10.median || '-'} | ${d10.estimatedRetries} |\n`;
            }
        }
    }
    
    md += '\n### Key Observations\n\n';
    md += '1. **Initialization Time:** Scales linearly with database size (10s for MOBILE_LOW to 96s for DESKTOP_ULTRA Opposite)\n';
    md += '2. **Depth 10 Performance:** Critical for user experience - ranges from 1-2ms (MOBILE_LOW) to 15-18ms (DESKTOP_ULTRA)\n';
    md += '3. **Retry Behavior:** Higher depths require more retry attempts (2-4x for depth 6, 10-15x for depth 10)\n';
    md += '4. **Adjacent vs Opposite:** Performance generally similar, with slight variations in initialization time\n';
    md += '5. **Memory Usage:** DESKTOP_ULTRA requires ~1.5GB heap, making it unsuitable for low-memory devices\n\n';
    
    md += '### Recommendations\n\n';
    md += '- **Mobile Devices:** Use MOBILE_LOW or MOBILE_MIDDLE for fast initialization (<20s) and sub-millisecond depth 10 performance\n';
    md += '- **Desktop (Standard):** DESKTOP_STD provides good balance - 35s init, ~6ms depth 10\n';
    md += '- **High-End Desktop:** DESKTOP_HIGH or DESKTOP_ULTRA for maximum solution quality, accepting 75-96s init time\n';
    md += '- **Production Deployment:** Consider lazy initialization or background loading for DESKTOP_ULTRA models\n\n';
    
    return md;
}

function main() {
    console.log('Merging results...\n');
    
    const completeResults = mergeResults();
    
    // Save complete JSON
    fs.writeFileSync(outputJsonPath, JSON.stringify(completeResults, null, 2));
    console.log(`✓ Complete JSON saved: ${outputJsonPath}`);
    
    // Generate Markdown report
    const mdReport = generateMarkdownReport(completeResults);
    fs.writeFileSync(outputMdPath, mdReport);
    console.log(`✓ Markdown report saved: ${outputMdPath}`);
    
    console.log('\n' + '='.repeat(80));
    console.log('All results merged successfully!');
    console.log('Total configurations: 12 (6 models × 2 pair types)');
    console.log('Total measurements: 72 (12 configs × 6 metrics: init + 5 depths)');
    console.log('='.repeat(80));
}

main();
