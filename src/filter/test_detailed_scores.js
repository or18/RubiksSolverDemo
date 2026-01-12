const { getFinalLabelList, calculateQTM } = require('./filter.js');
const fs = require('fs');
const path = require('path');

function loadSolutions(filePath, maxCount = 100) {
    const fullPath = path.resolve(__dirname, filePath);
    const content = fs.readFileSync(fullPath, 'utf-8');
    const lines = content.trim().split('\n');
    
    const solutions = [];
    for (const line of lines) {
        const match = line.match(/^\d+:\s*(.+)$/);
        if (match && match[1]) {
            solutions.push(match[1].trim());
            if (solutions.length >= maxCount) break;
        }
    }
    return solutions;
}

function convertToMoves(solutionStr) {
    const moveMap = {
        'U': 0, 'U2': 1, "U'": 2,
        'R': 3, 'R2': 4, "R'": 5,
        'F': 6, 'F2': 7, "F'": 8,
        'D': 9, 'D2': 10, "D'": 11,
        'L': 12, 'L2': 13, "L'": 14,
        'B': 15, 'B2': 16, "B'": 17
    };
    return solutionStr.split(' ').map(m => moveMap[m] !== undefined ? moveMap[m] : -1).filter(m => m >= 0);
}

const testFiles = ['sol2.txt', 'samples/7.txt'];

testFiles.forEach(file => {
    console.log('\n' + '='.repeat(80));
    console.log(`Testing ${file}`);
    console.log('='.repeat(80));
    
    const solutions = loadSolutions(file, 100);
    console.log(`Loaded ${solutions.length} solutions`);
    
    const result = getFinalLabelList(solutions, 4, 4);
    
    const labeled = result
        .map((r, idx) => ({ ...r, originalIdx: idx, solutionStr: solutions[idx] }))
        .filter(r => r !== null && r.label !== 'Unlabeled')
        .filter(r => r.recommendationRank !== null)
        .sort((a, b) => a.recommendationRank - b.recommendationRank);
    
    console.log(`Labeled solutions: ${labeled.length}`);
    
    const schoolCount = new Set(labeled.map(s => s.school)).size;
    console.log(`Schools formed: ${schoolCount}`);
    
    // Score statistics
    const scores = labeled.slice(0, 15).map(s => s.score);
    const adjustedScores = labeled.slice(0, 15).map(s => s.adjustedScore || 0);
    const scoreCount100 = scores.filter(s => s === 100).length;
    const scoreCount90Plus = scores.filter(s => s >= 90 && s < 100).length;
    
    console.log(`\nScore Analysis (Top 15):`);
    console.log(`  At 100: ${scoreCount100}/15, At 90-99: ${scoreCount90Plus}/15`);
    console.log(`  Base Score range: ${Math.min(...scores).toFixed(1)}-${Math.max(...scores).toFixed(1)}`);
    console.log(`  Adjusted Score range: ${Math.min(...adjustedScores).toFixed(1)}-${Math.max(...adjustedScores).toFixed(1)}`);
    
    // Top 15 details
    console.log('\nTop 15 Detailed:');
    console.log('Rank | School   | Label                  | HTM | QTM | Base | Adj  |');
    console.log('-----|----------|------------------------|-----|-----|------|------|');
    labeled.slice(0, 15).forEach((sol) => {
        const htm = sol.solutionStr.split(' ').length;
        const movesArray = convertToMoves(sol.solutionStr);
        const qtm = calculateQTM(movesArray);
        const baseScore = sol.score.toFixed(1).padStart(5);
        const adjScore = (sol.adjustedScore || 0).toFixed(1).padStart(5);
        console.log(`${String(sol.recommendationRank).padStart(4)} | ${sol.school.padEnd(8)} | ${sol.label.padEnd(22)} | ${String(htm).padStart(3)} | ${String(qtm).padStart(3)} | ${baseScore} | ${adjScore} |`);
    });
});
