// node-example.js
// Run: node node-example.js

let currentMessageHandler = (msg) => console.log('Unhandled:', msg);

// CRITICAL: Define globalThis.postMessage BEFORE loading module
globalThis.postMessage = function(msg) {
    currentMessageHandler(msg);
};

const createModule = require('./solver.js');

// URF moves only (optimal for 2x2x2)
const urfMoves = "U_U2_U-_R_R2_R-_F_F2_F-";

createModule().then(Module => {
    console.log('=== PersistentSolver2x2 Example ===\n');
    
    // Create persistent solver instance
    let solver = new Module.PersistentSolver2x2();
    console.log('✓ Created solver instance\n');
    
    // Solve first scramble
    console.log('Solving: R U R\' U\'');
    let solutionCount = 0;
    
    currentMessageHandler = (msg) => {
        if (msg.startsWith('depth=')) {
            console.log('  ' + msg.trim());
        } else if (msg === 'Search finished.') {
            console.log('  ' + msg + '\n');
        } else if (msg !== 'Already solved.') {
            solutionCount++;
            console.log('  Solution ' + solutionCount + ': ' + msg.trim());
        }
    };
    
    // solve(scramble, rotation, maxSolutions, maxLength, pruneDepth, allowedMoves, preMove, moveOrder, moveCount)
    solver.solve("R U R' U'", "", 3, 11, 1, urfMoves, "", "", "");
    console.log(`✓ Found ${solutionCount} solutions\n`);
    
    // Solve second scramble (reuses prune table)
    console.log('Solving: R2 U2 (reusing prune table)');
    solutionCount = 0;
    solver.solve("R2 U2", "", 3, 11, 1, urfMoves, "", "", "");
    console.log(`✓ Found ${solutionCount} solutions\n`);
    
    console.log('=== Complete! ===');
    console.log('Prune table was built once and reused for both solves.');
});
