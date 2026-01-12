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

function calculateQTM(solution) {
    const moves = solution.split(' ');
    let qtm = 0;
    for (const move of moves) {
        if (move.includes('2')) qtm += 2;
        else qtm += 1;
    }
    return qtm;
}

console.log('Testing QTM Bias Analysis');
console.log('='.repeat(80));

const solutions = loadSolutions('sol.txt', 100);
const result = getFinalLabelList(solutions, 4, 4);

const labeled = result.filter(r => r !== null && r.label !== 'Unlabeled');
const ranked = labeled
    .filter(r => r.recommendationRank !== null)
    .sort((a, b) => a.recommendationRank - b.recommendationRank);

console.log('\nDetailed Analysis of Top 15:');
console.log('Rank | Idx | School | Label | HTM | QTM | Score | Solution');
console.log('-'.repeat(120));

ranked.slice(0, 15).forEach((sol) => {
    const idx = result.indexOf(sol);
    const solution = solutions[idx];
    const htm = solution.split(' ').length;
    const qtm = calculateQTM(solution);
    const label = sol.label.padEnd(20);
    console.log(`${String(sol.recommendationRank).padStart(4)} | ${String(idx).padStart(3)} | ${sol.school.padEnd(8)} | ${label} | ${String(htm).padStart(3)} | ${String(qtm).padStart(3)} | ${sol.score.toFixed(1).padStart(5)} | ${solution}`);
});

// Investigate the distribution of QTM
console.log('\n' + '='.repeat(80));
console.log('QTM Distribution in Top 15:');
const qtms = ranked.slice(0, 15).map((sol) => {
    const idx = result.indexOf(sol);
    return calculateQTM(solutions[idx]);
});
qtms.sort((a, b) => a - b);
console.log('Min QTM:', Math.min(...qtms));
console.log('Max QTM:', Math.max(...qtms));
console.log('Average QTM:', (qtms.reduce((a, b) => a + b, 0) / qtms.length).toFixed(2));
console.log('QTMs:', qtms.join(', '));

// School_1 statistics
console.log('\n' + '='.repeat(80));
console.log('School_1 Details:');
const school1 = labeled.filter(sol => sol.school === 'School_1');
school1.forEach((sol) => {
    const idx = result.indexOf(sol);
    const solution = solutions[idx];
    const htm = solution.split(' ').length;
    const qtm = calculateQTM(solution);
    console.log(`Rank ${sol.recommendationRank}: #${idx} ${sol.label} - HTM:${htm} QTM:${qtm} Score:${sol.score.toFixed(1)}`);
});
