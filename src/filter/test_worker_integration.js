/**
 * Web Worker integration test
 * Confirm that filter.js works properly as a Web Worker
 */

// Web Simulate worker environment
global.self = global;
global.importScripts = function() {};

// Load filter.js
require('./filter.js');

console.log('='.repeat(80));
console.log('Web Worker Integration Test');
console.log('='.repeat(80));

// test data
const testSolutions = [
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

console.log('\n📊 Test Input:');
console.log(`  Solutions: ${testSolutions.length}`);

console.log('\n🔧 Simulating Worker Message:');
console.log(`  config.threshold = 4`);
console.log(`  config.prefixLen = 4`);

// Worker Test the message handler
let receivedMessage = null;

// Mock postMessage
global.self.postMessage = function(data) {
    receivedMessage = data;
};

console.log('\n⏳ Processing...');

//Send message to worker (simulate)
global.self.onmessage({
    data: {
        solutions: testSolutions,
        config: {
            threshold: 4,
            prefixLen: 4
        }
    }
});

console.log('\n✅ Worker Response Received!');

if (receivedMessage.error) {
    console.error('\n❌ Error:', receivedMessage.error);
    process.exit(1);
}

const { results, stats } = receivedMessage;

console.log('\n📈 Statistics:');
console.log(`  Total: ${stats.total}`);
console.log(`  Labeled: ${stats.labeled}`);
console.log(`  Unlabeled: ${stats.unlabeled}`);
console.log(`  Schools: ${stats.schools}`);
console.log(`  Representative: ${stats.representative}`);
console.log(`  Alternative: ${stats.alternative}`);
console.log(`  Member: ${stats.member}`);
console.log(`  Score100: ${stats.score100}`);

console.log('\n📋 Results Structure (first 5):');
results.slice(0, 5).forEach((result, idx) => {
    console.log(`  ${idx}. ${result.school} - ${result.label} - Rank: ${result.recommendationRank} - Score: ${result.score} - HTM/QTM: ${result.htm}/${result.qtm}`);
});

// verification
console.log('\n🔍 Validation:');
const checks = [
    { name: 'Results count matches solutions', pass: results.length === testSolutions.length },
    { name: 'All results have school property', pass: results.every(r => 'school' in r) },
    { name: 'All results have label property', pass: results.every(r => 'label' in r) },
    { name: 'All results have recommendationRank property', pass: results.every(r => 'recommendationRank' in r) },
    { name: 'All results have score property', pass: results.every(r => 'score' in r) },
    { name: 'All results have qtm property', pass: results.every(r => 'qtm' in r) },
    { name: 'All results have htm property', pass: results.every(r => 'htm' in r) },
    { name: 'All results have subgroupIndex property', pass: results.every(r => 'subgroupIndex' in r) },
    { name: 'All results have subgroupSize property', pass: results.every(r => 'subgroupSize' in r) },
    { name: 'All results have totalSubgroups property', pass: results.every(r => 'totalSubgroups' in r) },
    { name: 'No undefined subgroup values', pass: results.every(r => r.subgroupIndex !== undefined && r.subgroupSize !== undefined && r.totalSubgroups !== undefined) },
    { name: 'Stats total matches', pass: stats.total === testSolutions.length },
    { name: 'Labeled + Unlabeled = Total', pass: stats.labeled + stats.unlabeled === stats.total },
    { name: 'At least one school found', pass: stats.schools >= 1 },
    { name: 'At least one representative', pass: stats.representative >= 1 }
];

let allPassed = true;
checks.forEach(check => {
    const status = check.pass ? '✅' : '❌';
    console.log(`  ${status} ${check.name}`);
    if (!check.pass) allPassed = false;
});

console.log('\n' + '='.repeat(80));
if (allPassed) {
    console.log('✅ All tests PASSED! Web Worker integration is working correctly.');
    console.log('='.repeat(80));
    process.exit(0);
} else {
    console.log('❌ Some tests FAILED!');
    console.log('='.repeat(80));
    process.exit(1);
}
