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

console.log('\n📊 Unlabeled Solutions with Subgroup Info:\n');
const unlabeled = results.filter(r => r.label === 'Unlabeled');
unlabeled.forEach((r) => {
    const sg = r.subgroupIndex !== null && r.subgroupIndex !== undefined
        ? `✓ Subgroup ${r.subgroupIndex + 1}/${r.totalSubgroups} (size: ${r.subgroupSize})`
        : (r.totalSubgroups > 0 ? `✗ Not in subgroup (${r.totalSubgroups} total in school)` : '- No subgroup division');
    console.log(`#${r.originalIdx.toString().padStart(2)} [${r.school.padEnd(9)}] ${sg}`);
    console.log(`    Solution: ${testSolutions[r.originalIdx]}`);
});

console.log('\n📈 Summary:');
console.log(`  Total unlabeled: ${unlabeled.length}`);
console.log(`  Unlabeled WITH subgroup info: ${unlabeled.filter(r => r.subgroupIndex !== null).length}`);
console.log(`  Unlabeled WITHOUT subgroup (but school has): ${unlabeled.filter(r => r.subgroupIndex === null && r.totalSubgroups > 0).length}`);
console.log(`  Unlabeled in "None" school: ${unlabeled.filter(r => r.school === 'None').length}\n`);
