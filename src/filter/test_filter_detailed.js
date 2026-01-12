const { getFinalLabelList } = require('./filter.js');
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

console.log('='.repeat(80));
console.log('FILTER.JS COMPREHENSIVE TEST');
console.log('='.repeat(80));

// Test 1: sol.txt (first 100)
console.log('\n[Test 1] sol.txt (first 100)');
try {
    const solutions = loadSolutions('sol.txt', 100);
    console.log(`Loaded ${solutions.length} solutions`);
    
    const result = getFinalLabelList(solutions, 4, 4);
    
    // Extract only labeled solutions
    const labeled = result.filter(r => r !== null && r.label !== 'Unlabeled');
    console.log(`Labeled solutions: ${labeled.length}`);
    
    // Statistics by school
    const schoolStats = new Map();
    labeled.forEach(sol => {
        if (!schoolStats.has(sol.school)) {
            schoolStats.set(sol.school, { total: 0, byLabel: {} });
        }
        const stats = schoolStats.get(sol.school);
        stats.total++;
        stats.byLabel[sol.label] = (stats.byLabel[sol.label] || 0) + 1;
    });
    
    console.log('\n--- School Statistics ---');
    Array.from(schoolStats.entries())
        .sort((a, b) => {
            const rankA = parseInt(a[0].replace('School_', ''));
            const rankB = parseInt(b[0].replace('School_', ''));
            return rankA - rankB;
        })
        .forEach(([school, stats]) => {
            const labels = Object.entries(stats.byLabel)
                .map(([label, count]) => `${label}:${count}`)
                .join(', ');
            console.log(`${school}: ${stats.total} labels (${labels})`);
        });
    
    // Show ranking top 20
    const ranked = labeled
        .filter(r => r.recommendationRank !== null)
        .sort((a, b) => a.recommendationRank - b.recommendationRank);
    
    console.log('\n--- Top 20 Ranking ---');
    ranked.slice(0, 20).forEach((sol, i) => {
        const idx = result.indexOf(sol);
        console.log(`Rank ${sol.recommendationRank}: #${idx} ${sol.school} - ${sol.label} (Score: ${sol.score.toFixed(1)})`);
    });
    
    // Investigating the relationship between QTM and rankings
    console.log('\n--- QTM vs Ranking Analysis ---');
    const withMoves = ranked.slice(0, 10).map((sol, i) => {
        const idx = result.indexOf(sol);
        const moves = solutions[idx].split(' ');
        let qtm = 0;
        for (const move of moves) {
            if (move.includes('2')) qtm += 2;
            else qtm += 1;
        }
        return { rank: sol.recommendationRank, school: sol.school, label: sol.label, htm: moves.length, qtm };
    });
    
    withMoves.forEach(item => {
        console.log(`Rank ${item.rank}: ${item.school} ${item.label} - HTM:${item.htm} QTM:${item.qtm}`);
    });
    
} catch (error) {
    console.error('Error:', error.message);
    console.error(error.stack);
}

console.log('\n' + '='.repeat(80));
