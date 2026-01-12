# Filter.js Developer Guide

## Overview

`filter.js` is a filtering system that strategically classifies similar solutions from a list of Rubik's Cube solutions and selects recommended solutions.

**Related Documentation**:
- **PARAMETERS.md** - Complete parameter reference and configuration examples
- **USAGE_HTML_TOOL.md** - HTML interface usage guide
- **UNUSED_FUNCTIONS_REPORT.md** - Functions available but currently unused

### Main Features

1. **School Classification**: Group solutions with similar end phases (aiming for the same intermediate goal)
2. **Subgroup Division**: Classify solutions within each school based on different approaches in the first half
3. **Selection of Representative and Alternative Solutions**: Select the best solutions from each subgroup and label them
4. **Recommendation Ranking**: Comprehensive evaluation considering statistical quality and QTM efficiency

---

## Architecture

### Algorithm Flow

```
Input: List of solutions (string array)
  ↓
1. Sequential Leader Clustering (School Classification)
   - Grouping by end phase similarity (calculateEndSimilarity)
   - Solutions aiming for the same goal are grouped into the same school
  ↓
2. K-Medoids (Representative Selection)
   - Select central solutions for each school
  ↓
3. Merge Small Clusters
   - Merge clusters smaller than the minimum school size
  ↓
4. Subgroup Division (divideIntoSubgroups)
   - Subgrouping by first half similarity
   - Classify "different approaches to the same goal"
  ↓
5. Labeling
   - Representative: Best solution in the largest subgroup
   - Shortest Alternative: Shortest solution (top 3 subgroups)
   - Alternative: High-quality solutions (top 5 subgroups)
   - Member: Other high-quality solutions (top 7 subgroups)
  ↓
6. Scoring & Ranking
   - Base Score: Statistical Quality (0-100 points)
   - Adjusted Score: Base + Bonuses (School, Label, QTM)
  ↓
Output: Labeled result array
```

---

## Key Parameters

### 1. School Classification Parameters

#### `threshold` (Default: 4)
Distance threshold for school classification.

- **Usage**: Used in `computeSchoolDistance`
- **Range**: 2-10 (Recommended: 3-6)
- **Effect**:
  - Smaller values (2-3): Schools are subdivided, resulting in many small schools
  - Larger values (7-10): Schools are merged, resulting in a few large schools
- **Recommended Values**:
  - 100 solutions: 4
  - 500 solutions: 5
  - Over 1000 solutions: 6

#### `minClusterSize` (Dynamic Adjustment)
Minimum size recognized as a school.

- **Default Values**:
  - n ≤ 150: 5
  - n ≤ 500: 3
  - n > 500: 2
- **Effect**: Excludes schools that are too small, retaining only meaningful groups 

#### `maxClusterSize` (Dynamic Adjustment)
Maximum size of a single school.

- **Default Values**:
  - n ≤ 150: 15
  - n ≤ 500: 30
  - n > 500: ∞
- **Effect**: Splits large schools to ensure diversity
---

### 2. Subgroup Division Parameters

#### `subgroupThreshold` (Dynamic Adjustment)
Distance threshold for subgroup division.

- **Default Values**:
  - n ≤ 150: 2.5
  - n > 150: 3.0
- **Range**: 1.5-4.0 (Recommended: 2.0-3.5)
- **Effect**:
  - Smaller values (1.5-2.0): Subgroups are subdivided
  - Larger values (3.5-4.0): Subgroups are merged
- **Recommended**: Automatically adjusted based on sample size, usually no need to change

#### `minSubgroupSizeForLabel` (Fixed: 3)
Minimum subgroup size required for labeling.

- **Effect**: Subgroups with fewer than 3 members are not labeled

---

### 3. Scoring Parameters

#### Base Score Weight Distribution

```javascript
// Inside calculateRecommendationScore
const totalScore = (
    lengthScore * 40 +        // Move Length: 40% (Most Important)
    rankScore * 25 +          // School Rank: 25%
    sizeScore * 15 +          // School Size: 15%
    distanceScore * 10 +      // Similarity to Representative: 10%
    silhouetteScore * 8 +     // Cluster Quality: 8%
    representativeScore * 2   // Representativeness / Shortest in School: 2%
);
```

**Adjustment Guidelines**:
- To prioritize move length: increase lengthScore to 50%
- To emphasize school diversity: increase rankScore to 30%
- To favor large schools: increase sizeScore to 20%

#### Decay Rate of Move Length Score

```javascript
// Exponential Decay
let lengthScore = Math.pow(0.7, lengthDiff * 1.5);
```

- **`0.7`**: Base of decay (smaller is stricter)
  - 0.6: Very strict (strongly penalizes move length difference)
  - 0.7: Standard (Recommended)
  - 0.8: Lenient (Allows move length difference)

- **`1.5`**: Exponent of decay (larger is stricter)
  - 1.0: Linear decay
  - 1.5: Standard (Recommended)
  - 2.0: Rapid decay

---

### 4. QTM Bonus Parameters

#### Subgroup Candidate Selection

```javascript
const qtmBonus = Math.max(0, (15 - qtm) * 0.3);
```

- **Multiplier**: 0.3 (Recommended Range: 0.2-0.5)
- **Effect**: Degree of QTM emphasis when selecting the best solution within a subgroup

#### Alternative Completion

**Case 1 (Subgroup Formation Failure)**:
```javascript
const qtmBonus = Math.max(0, (15 - qtm) * 1.5);
```

- **Multiplier**: 1.5 (Recommended Range: 1.0-2.0)
- **Effect**: Emphasizes QTM efficiency when selecting the representative solution

**Case 2 (Single Subgroup Only)**:
```javascript
const qtmBonus = Math.max(0, (15 - qtm) * 1.0);
```
- **Multiplier**: 1.0 (Recommended Range: 0.8-1.5)

**Case 3 (Multiple Subgroups)**:
```javascript
const qtmBonus = Math.max(0, (15 - qtm) * 0.7);
```
- **Multiplier**: 0.7 (Recommended Range: 0.5-1.0)
#### Final Ranking 

```javascript
const qtmBonus = Math.max(0, (15 - qtm) * 0.2);
```

- **Multiplier**: 0.2 (Recommended Range: 0.1-0.3)
- **Effect**: Fine-tuning up to 3 points (Prevents QTM from dominating the ranking)

---

### 5. Label Bonus

```javascript
// Representative
labelBonus = rank === 0 ? 15 : (rank < 3 ? 10 : 5);

// Shortest Alternative
labelBonus = 8;

// Alternative
labelBonus = 5;

// Member
labelBonus = 2;
```

**Adjustment Guidelines**:
- Strengthen Representative priority: Increase School_1 bonus to 20
- Emphasize Alternative: Increase Alternative bonus to 8
- Equalize: Unify all bonuses around 5

---

### 6. Similarity Calculation Parameters
#### End Similarity

```javascript
// Inside calculateEndSimilarity
const endChunkSize = avgLength <= 7 ? 3 :
                     avgLength <= 10 ? 4 : 5;

// Weight Distribution
return jaccardSim * 0.3 + editSim * 0.3 + faceSim * 0.4;
```

**Weight Adjustment**:
- **Face Similarity 40%**: Detects "same face used but different order"
- **Jaccard 30%**: Overlap of hand sets
- **Edit Distance 30%**: Similarity of order

**Adjustment Examples**:
- Emphasize order: Change editSim to 40%, faceSim to 30%
- Emphasize face (current recommendation): Increase faceSim to 50%

#### Chunk Similarity (Front Half Similarity)

```javascript
// Inside calculateChunkSimilarityForSubgroup
const frontChunkSize = Math.min(chunkSize, Math.floor(avgLength * 0.5));

// Weight Distribution
return jaccardSim * 0.4 + editSim * 0.4 + faceSim * 0.2;
```

**For Subgroup Division (Emphasizing Front Half)**:
- Jaccard 40%: Overlap of hand sets
- Edit Distance 40%: Similarity of order
- Face Similarity 20%: Usage of faces

---

### 7. Outlier Detection (Advanced - Currently Unused)

#### `outlierThreshold` (Default: 3.5)
Threshold for statistical outlier detection using Modified Z-Score method.

- **Current Status**: ⚠️ **This parameter is defined but NOT currently used in the code**
- **Range**: 2.0-5.0 (Recommended: 3.0-4.0)
- **Effect**: Lower values detect more outliers, higher values are more lenient

#### `detectOutliers()` Function
The `detectOutliers()` function exists in filter.js but is currently not called anywhere. It was originally used in the `isAnomalous()` function (now removed) to filter out statistical outliers from schools.

**Original Implementation Context** (from test version):
```javascript
// In isAnomalous() function - previously used for outlier filtering
function isAnomalous(idx, family, allMoves, families, sampleSize) {
    const outlierThreshold = sampleSize <= 150 ? 3.5 : 3.0;
    
    // 1. Statistical outlier detection using Modified Z-Score
    const outliers = detectOutliers(family, allMoves, outlierThreshold);
    if (outliers.includes(idx)) return true;
    
    // 2. Extreme length check
    // 3. Extreme distance from representative
    // ... (other checks)
}
```

**How to Implement** (if needed):

You can add outlier filtering **after subgroup division** or **during subgroup candidate selection**. Here's a recommended implementation:

```javascript
// Option 1: Filter outliers after divideIntoSubgroups
const subgroups = divideIntoSubgroups(family, allMoves, actualSubgroupThreshold);

// Filter out outliers from each subgroup
const filteredSubgroups = subgroups.map(subgroup => {
    if (subgroup.memberIndices.length < 5) {
        // Skip outlier detection for small subgroups
        return subgroup;
    }
    
    const outlierIndices = detectOutliers(
        { ...subgroup, leaderIdx: subgroup.leaderIdx }, 
        allMoves, 
        outlierThreshold
    );
    
    // Remove outliers from subgroup
    const filteredMembers = subgroup.memberIndices.filter(
        idx => !outlierIndices.includes(idx)
    );
    
    return {
        ...subgroup,
        memberIndices: filteredMembers
    };
});

// Option 2: Use in candidate selection
candidates.forEach(({ idx, length, qtm, prefix, score }) => {
    // Check if this candidate is an outlier
    const outliers = detectOutliers(subgroup, allMoves, outlierThreshold);
    if (outliers.includes(idx)) {
        return; // Skip outlier candidates
    }
    
    // ... rest of selection logic
});
```

**When to Use Outlier Detection**:
- Large sample sizes (n > 500): More reliable statistical detection
- Quality-focused filtering: Remove extreme/unusual solutions
- Subgroups with high variance: Filter out inconsistent members

**When NOT to Use**:
- Small samples (n < 150): Outlier detection may be unreliable
- Diversity-focused filtering: May remove interesting edge cases
- Well-clustered data: Additional filtering may be unnecessary

**Note**: The current implementation prioritizes **inclusiveness** over strict filtering. Outlier detection can be enabled by uncommenting the relevant code sections and passing `outlierThreshold` from the config object.

---

## Input/Output Specifications
### Input

```javascript
getFinalLabelList(rawMovesList, threshold, prefixLen)
```

**Arguments**:
- `rawMovesList`: `string[]` - Array of solutions (e.g., `["R U R' U'", "F R U' R'"]`)
- `threshold`: `number` - Threshold for style classification (default: 4)
- `prefixLen`: `number` - Length of prefix (default: 4, usually no need to change)

### Output

```javascript
Array<{
    school: string,              // Style name (e.g., "School_1")
    label: string,               // Label (Representative, Shortest Alternative, Alternative, Member, Unlabeled)
    prefix: string,              // Prefix string
    schoolRank: number,          // Style rank (1-based, larger styles have smaller ranks)
    score: number,               // Base score (0-100)
    qtm: number,                 // Quarter Turn Metric
    htm: number,                 // Half Turn Metric
    originalIdx: number,         // Original index in the input solutions array
    recommendationRank: number,  // Recommendation rank (1-based, null = out of rank)
    adjustedScore: number,       // Adjusted score (including bonuses)
    totalCandidates: number,     // Total number of labeled solutions
    subgroupIndex: number|null,  // Subgroup index (0-based, null if not in subgroup)
    subgroupSize: number|null,   // Size of the subgroup (null if not in subgroup)
    totalSubgroups: number       // Total number of subgroups in the school (0 if no division occurred)
}>
```

**Meaning of Labels**:
- **Representative**: Representative solution of each style (from the largest/best subgroup)
- **Shortest Alternative**: Shortest solution (shortest solution from the top 3 subgroups)
- **Alternative**: Good alternative solutions (from the top 5 subgroups)
- **Member**: Other good solutions (from the top 7 subgroups)
- **Unlabeled**: Solutions that were not labeled

**Subgroup Information**:
- **subgroupIndex**: Index of the subgroup within the school (0-based, null if not in any subgroup)
- **subgroupSize**: Number of solutions in the subgroup (null if not in any subgroup)
- **totalSubgroups**: Total number of subgroups formed within the school (0 if no division occurred)
- **Note**: Unlabeled solutions also have subgroup information if they belong to a school with subgroups
- **Example**: A solution with `subgroupIndex: 2, subgroupSize: 5, totalSubgroups: 8` belongs to the 3rd subgroup (size 5) out of 8 total subgroups in that school

---

## Testing

### Basic Test

```bash
cd /workspaces/RubiksSolverDemo/src/filter
node test_filter_detailed.js
```

**Output Example**:
```
Loaded 100 solutions
Labeled solutions: 15

School Statistics:
School_1: 3 labels (Representative:1, Alternative:2)
School_2: 2 labels (Representative:1, Shortest Alternative:1)
...

Top 20 Ranking:
Rank 1: #3 School_1 Representative HTM:8 QTM:11 Score:100.0
Rank 2: #5 School_2 Representative HTM:8 QTM:9 Score:95.5
...
```

### Comprehensive Test

```bash
node test_comprehensive.js
```

**Batch testing with multiple sample files**:
- sol.txt
- sol2.txt
- samples/1.txt ~ samples/8.txt

**Output Example**:
```
File           | Loaded | Labeled | Schools | 100pt | 90+ | BaseRange  |
---------------|--------|---------|---------|-------|-----|------------|
sol.txt        |    100 |      15 |       7 |     2 |   5 | 56.5-100.0 |
sol2.txt       |    100 |      19 |       9 |     4 |   7 | 76.6-100.0 |
...
```

### QTM Bias Test

```bash
node test_qtm_bias.js
```

**Verifying that QTM does not dominate the ranking**:
- Distribution of QTM in the top 15
- Relationship between QTM and rank within the same style
- Examples of high rank despite poor QTM

### Detailed Score Test

```bash
node test_detailed_scores.js
```

**Displaying both Base Score and Adjusted Score**:
```
Rank | School   | Label          | HTM | QTM | Base  | Adj   |
-----|----------|----------------|-----|-----|-------|-------|
   1 | School_1 | Representative |   9 |  11 | 100.0 | 135.8 |
   2 | School_1 | Shortest Alt   |   9 |  12 | 100.0 | 128.6 |
...
```

### Custom Test

```javascript
const { getFinalLabelList } = require('./filter.js');

const solutions = [
    "R U R' U'",
    "R U2 R' U'",
    // ... Add more solutions
];

const result = getFinalLabelList(solutions, 4, 4);

// Extract only labeled solutions
const labeled = result.filter(r => r && r.label !== 'Unlabeled');

// Sort by recommendation rank
labeled.sort((a, b) => (a.recommendationRank || 999) - (b.recommendationRank || 999));

console.log(labeled);

```
---

## Parameter Tuning Guide

### Scenario 1: Too Many Styles

**Symptom**: More than 15 styles formed with 100 solutions
**Countermeasure**:
1. Increase `threshold` (4 → 5 or 6)
2. Increase `minClusterSize` (5 → 7)

### Scenario 2: Too Few Styles
**Symptom**: 3 or fewer styles formed with 100 solutions

**Countermeasure**:
1. Decrease `threshold` (4 → 3)
2. Decrease `maxClusterSize` (15 → 10)
### Scenario 3: No Subgroups Formed

**Symptom**: Frequent occurrences of validSubgroupCount === 0

**Countermeasure**:
1. Increase `subgroupThreshold` (2.5 → 3.5)
2. Decrease `minSubgroupSizeForLabel` (3 → 2)

### Scenario 4: Scores Biased Towards 100

**Symptom**: Majority of Top 15 have a score of 100

**Countermeasure**:
1. Make step length decay stricter (`Math.pow(0.7, ...)` → `Math.pow(0.6, ...)`)
2. Increase silhouette coefficient weight (8% → 15%)

### Scenario 5: QTM Dominates Ranking

**Symptom**: Top 10 are ordered by QTM

**Countermeasure**:
1. Decrease QTM multiplier in final ranking (0.2 → 0.1)
2. Decrease QTM multiplier in subgroup selection (0.3 → 0.2)

### Scenario 6: Short Solutions Overly Favored

**Symptom**: All HTM+1 solutions are ranked out

**Countermeasure**:
1. Decrease step length weight (40% → 30%)
2. Increase style rank weight (25% → 35%)

---

## Troubleshooting

### Error: "Cannot read properties of undefined"

**Cause**: Solution list is empty or in an invalid format

**Countermeasure**:
```javascript
// Input validation
if (!rawMovesList || rawMovesList.length === 0) {
    return [];
}
```

### Warning: All Solutions are Unlabeled

**Cause**: 
1. `minClusterSize` is too large
2. Solutions are highly diverse, preventing cluster formation
**Countermeasure**:
1. Decrease `minClusterSize` (5 → 3)
2. Increase `threshold` (4 → 6)

### Performance: Slow Processing

**Cause**: Large number of solutions (1000+)

**Optimization**:
1. Set `maxClusterSize` (Infinity → 50)
2. Consider sampling (process only the first 500 solutions)

---

### Recommended Development Flow

1. **Initial Testing with New Samples**
   ```bash
   node test_filter_detailed.js
   ```

2. **Parameter Tuning**
   - Adjust via UI in `filter_worker_example.html`
   - Or directly modify default values in the code

3. **Comprehensive Testing**
   ```bash
   node test_comprehensive.js
   ```

4. **QTM Bias Verification**
   ```bash
   node test_qtm_bias.js
   ```

5. **Score Distribution Verification**
   ```bash
   node test_detailed_scores.js
   ```

6. **Production Environment Testing**
   - Verify operation with actual solution lists
   - Measure performance  

---

## Best Practices

### 1. Adjusting According to Sample Size

```javascript
// Recommended parameter sets
if (n <= 150) {
    threshold = 4;
    subgroupThreshold = 2.5;
    minClusterSize = 5;
} else if (n <= 500) {
    threshold = 5;
    subgroupThreshold = 3.0;
    minClusterSize = 3;
} else {
    threshold = 6;
    subgroupThreshold = 3.5;
    minClusterSize = 2;
}
```

### 2. Interpretation of Scores

- **Base Score 90-100**: Excellent solutions
- **Base Score 70-89**: Good solutions
- **Base Score 50-69**: Standard solutions
- **Base Score < 50**: Not recommended solutions

### 3. Interpretation of Rankings

- **Rank 1-5**: Highest priority recommendations (for beginners)
- **Rank 6-15**: Recommended (for intermediate users)
- **Rank 16-30**: Reference (for advanced users)
- **Rank > 30 or null**: Do not display

### 4. Utilization of QTM
QTM is evaluated in combination with HTM:
- **HTM=8, QTM=8**: Very efficient (no 180-degree rotations)
- **HTM=8, QTM=12**: Standard (includes 180-degree rotations)
- **HTM=8, QTM=14**: Slightly inefficient (many 180-degree rotations)
---

## Summary

filter.js is a highly parameterized filtering system. With appropriate parameter tuning, it can handle various solution lists.

**Key Points**:
1. Dynamic parameter adjustment according to sample size is important
2. QTM is only fine-tuned (do not let it dominate the ranking)
3. Style classification focuses on the "endgame", while subgroups focus on the "early game"
4. Check both Base Score and Adjusted Score
5. Verify stability with comprehensive testing

**See Also**:
- **PARAMETERS.md** - For detailed parameter descriptions and recommended ranges
- **USAGE_HTML_TOOL.md** - For practical usage examples with the HTML interface

---

**Last Updated**: 2026-01-12
**Version**: 2.0 (End Similarity + Subgroup Division)