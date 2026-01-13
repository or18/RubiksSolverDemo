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

// Test with minSubgroupSize = 4 to verify filtering
const config = {
    threshold: 4, prefixLen: 4, minClusterSize: 2,
    maxClusterSize: 30, subgroupThreshold: 2.5,
    minSubgroupSize: 4,  // Should filter out subgroups with size < 4
    rankWeight: 40, sizeWeight: 30, focusMoves: 5
};

const results = getFinalLabelList(testSolutions, config);

console.log('\n📊 Subgroup Filtering Test (minSubgroupSize = 4):\n');
const resultsWithSubgroup = results.filter(r => r.subgroupIndex !== null && r.subgroupIndex !== undefined);

console.log('Results with subgroup info:');
resultsWithSubgroup.forEach((r) => {
    console.log(`  #${r.originalIdx} [${r.school}] ${r.label.padEnd(20)} - SG ${r.subgroupIndex + 1}/${r.totalSubgroups} (size: ${r.subgroupSize})`);
});

// Check for violations
const violations = resultsWithSubgroup.filter(r => r.subgroupSize < 4);
if (violations.length > 0) {
    console.log(`\n❌ FAILED: Found ${violations.length} subgroups with size < 4:`);
    violations.forEach(v => {
        console.log(`  #${v.originalIdx} - SG ${v.subgroupIndex + 1} has size ${v.subgroupSize}`);
    });
} else {
    console.log(`\n✅ PASSED: All subgroups have size >= 4`);
}

console.log(`\nTotal subgroups assigned: ${resultsWithSubgroup.length}`);
console.log(`Unique subgroups: ${new Set(resultsWithSubgroup.map(r => `${r.school}-${r.subgroupIndex}`)).size}\n`);
