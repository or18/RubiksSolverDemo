/**
 * HTML integration test
 * Test the functionality used in filter_worker_example_enhanced.html
 */

const { getFinalLabelList } = require('./filter.js');

console.log('='.repeat(80));
console.log('HTML Integration Test');
console.log('='.repeat(80));

// Sample data (same as HTML loadSampleData)
const solutions = [
    "R U R' U'",
    "F R U' R' F'",
    "R U2 R' U'",
    "F' U' F U",
    "R U R' U R U2 R'",
    "F R U R' U' F'",
    "R U R' U' R' F R F'",
    "F' U' F U R U R'",
    "R U R' F' U' F",
    "F U R U' R' F'"
];

console.log('\n📊 Input:');
console.log(`  Solutions: ${solutions.length}`);

console.log('\n🔧 Testing with default parameters:');
console.log(`  threshold = 4`);
console.log(`  prefixLen = 4`);

try {
    const startTime = Date.now();
    const results = getFinalLabelList(solutions, 4, 4);
    const endTime = Date.now();
    
    console.log('\n✅ Success!');
    console.log(`  Processing time: ${(endTime - startTime) / 1000} seconds`);
    
    // Statistics
    const labeled = results.filter(r => r && r.label !== 'Unlabeled').length;
    const schools = new Set(results.filter(r => r).map(r => r.school)).size;
    const representatives = results.filter(r => r && r.label === 'Representative').length;
    const alternatives = results.filter(r => r && r.label && r.label.includes('Alternative')).length;
    const score100 = results.filter(r => r && r.score >= 100).length;
    
    console.log('\n📈 Statistics:');
    console.log(`  Total: ${solutions.length}`);
    console.log(`  Labeled: ${labeled}`);
    console.log(`  Schools: ${schools}`);
    console.log(`  Representatives: ${representatives}`);
    console.log(`  Alternatives: ${alternatives}`);
    console.log(`  Score 100: ${score100}`);
    
    console.log('\n📋 Top 5 Results:');
    results
        .filter(r => r && r.recommendationRank)
        .sort((a, b) => a.recommendationRank - b.recommendationRank)
        .slice(0, 5)
        .forEach((r, idx) => {
            const solutionIdx = results.indexOf(r);
            console.log(`  ${idx + 1}. [${r.recommendationRank}] ${r.school} ${r.label}`);
            console.log(`     Solution: ${solutions[solutionIdx]}`);
            console.log(`     Score: ${r.score.toFixed(1)}, Adjusted: ${r.adjustedScore.toFixed(1)}, QTM: ${r.qtm}`);
        });
    
    console.log('\n✅ You should be able to get the same result with any HTML tool!');
    console.log('='.repeat(80));
    
} catch (error) {
    console.error('\n❌ Error:', error.message);
    console.error(error.stack);
    process.exit(1);
}
