const fs = require('fs');
const code = fs.readFileSync('filter.js', 'utf8');
eval(code);

const testSolutions = [
    'R U R\' U\' R\' F R2 U\' R\' U\' R U R\' F\'',
    'R U R\' U\' R\' F R F\' U2 R U2 R\'',
    'R U R\' U\' F\' U\' F',
    'R U R\' U\' R\' F R F\'',
    'F R U R\' U\' F\'',
    'R U R\' F\' U\' F',
    'R U2 R\' U\' R U\' R\'',
    'F\' U\' L\' U L F',
    'L\' U\' L U L F\' L\' F',
    'F U R U\' R\' F\''
];

const config = {
    threshold: 4, prefixLen: 4, minClusterSize: 2,
    maxClusterSize: 30, subgroupThreshold: 2.5,
    bestN: 20, distanceWeight: 1.0, lengthPenalty: 0.5,
    qtmPenalty: 0.8, rankPenalty: 0.3, outlierThreshold: 3.0,
    lengthWeight: 1.0, rankWeight: 0.5, sizeWeight: 1.0,
    endFaceWeight: 1.0, endSequenceWeight: 0.5
};

const results = getFinalLabelList(testSolutions, config);

console.log('\n📊 Detailed Subgroup Analysis:\n');
results.forEach((r) => {
    const sg = r.subgroupIndex !== null && r.subgroupIndex !== undefined
        ? `SG ${r.subgroupIndex + 1}/${r.totalSubgroups} (${r.subgroupSize})`
        : (r.totalSubgroups > 0 ? `No SG (${r.totalSubgroups} total)` : 'No division');
    console.log(`#${r.originalIdx.toString().padStart(2)} [${r.school.padEnd(9)}] ${r.label.padEnd(20)} Rank:${(r.recommendationRank || '-').toString().padStart(2)} | ${sg}`);
});

// Check for undefined properties
const hasUndefined = results.some(r => 
    r.subgroupIndex === undefined || 
    r.subgroupSize === undefined || 
    r.totalSubgroups === undefined
);
console.log(`\n${hasUndefined ? '❌ Found undefined properties!' : '✅ All properties defined correctly'}\n`);

// Count each subgroup category
const withSubgroup = results.filter(r => r.subgroupIndex !== null).length;
const noSubgroup = results.filter(r => r.subgroupIndex === null && r.totalSubgroups > 0).length;
const noDivision = results.filter(r => r.totalSubgroups === 0).length;

console.log('📈 Subgroup Statistics:');
console.log(`  - With subgroup: ${withSubgroup}`);
console.log(`  - No subgroup (but school has): ${noSubgroup}`);
console.log(`  - No division: ${noDivision}`);
console.log(`  - Total: ${results.length}\n`);
