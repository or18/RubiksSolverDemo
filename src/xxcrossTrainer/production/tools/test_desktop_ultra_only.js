#!/usr/bin/env node

/**
 * DESKTOP_ULTRA Opposite Test - Isolated test for debugging
 */

const createModule = require('./solver_prod.js');

(async () => {
    try {
        console.log('='.repeat(80));
        console.log('DESKTOP_ULTRA Opposite - Isolated Test');
        console.log('='.repeat(80));
        
        console.log('\n[1/3] Initializing WASM module...');
        const Module = await createModule();
        console.log('✓ WASM module initialized');
        
        console.log('\n[2/3] Creating DESKTOP_ULTRA Opposite solver...');
        console.log('  Model: DESKTOP_ULTRA (16/16/16/16 MB)');
        console.log('  Pair type: Opposite (adj=false)');
        console.log('  Expected memory: ~1.4 GB');
        console.log('');
        
        const startTime = performance.now();
        const instance = new Module.xxcross_search(false, "DESKTOP_ULTRA");
        const initTime = ((performance.now() - startTime) / 1000).toFixed(1);
        
        console.log(`✓ Solver initialized in ${initTime}s`);
        
        console.log('\n[3/3] Testing scramble generation (depth 10, 5 trials)...');
        const times = [];
        for (let i = 0; i < 5; i++) {
            const t0 = performance.now();
            const result = instance.func("", "10");
            const elapsed = performance.now() - t0;
            times.push(elapsed);
            
            const parts = result.split(',');
            const scramble = parts[1] || parts[0];
            console.log(`  Trial ${i + 1}: ${elapsed.toFixed(2)}ms - ${scramble}`);
        }
        
        const avgTime = (times.reduce((a, b) => a + b, 0) / times.length).toFixed(2);
        console.log(`\n✓ Average time: ${avgTime}ms`);
        
        console.log('\n' + '='.repeat(80));
        console.log('SUCCESS: DESKTOP_ULTRA Opposite works in Node.js!');
        console.log('='.repeat(80));
        
    } catch (error) {
        console.error('\n' + '='.repeat(80));
        console.error('ERROR:', error.message);
        console.error('='.repeat(80));
        console.error(error);
        process.exit(1);
    }
})();
