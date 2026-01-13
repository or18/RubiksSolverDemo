# Filter.js Parameter List

**Quick Reference**: This document provides a complete list of all configurable parameters and their usage.

**Related Documentation**:
- **DEVELOPER_GUIDE.md** - Algorithm details and implementation examples
- **USAGE_HTML_TOOL.md** - HTML interface usage guide

---

## All Parameters Can Be Adjusted from the UI

filter.js allows all parameters to be set externally through the `config` object.

## Output result properties

Each solution is given the following properties:

- `school`: school name (e.g. School_1)
- `label`: Label (Representative, Alternative, Member, Unlabeled)
- `prefix`: first few moves of the procedure (string)
- `schoolRank`: School rank (1 is the highest school)
- `score`: Base score (0-100)
- `qtm`: Quarter Turn Metric (180 degree rotation counts as 2)
- `htm`: Half Turn Metric (every rotation counts as 1)
- `originalIdx`: Original solution index (0-based, in input order)
- `recommendationRank`: Recommendation rank (1 is the most recommended)
- `adjustedScore`: Score after bonus is applied
- `totalCandidates`: total number of labeled solutions

### About HTM/QTM

- **HTM (Half Turn Metric)**: Every rotation counts as 1
  - `R`, `R2`, `R'` all count as 1
- **QTM (Quarter Turn Metric)**: 180 degree rotation counts as 2
  - `R`, `R'` is 1 count
  - `R2` is 2 counts
- **Rotational efficiency**: HTM/QTM ratio
  - Close to 1.0: less 180 degree rotation and more efficient
  - Less than 0.7: Many 180 degree rotations

## Parameter list

### 1. School classification parameters

| Parameter | Default | Range | Description |
|------------|------------|------|------|
| `threshold` | 4 | 2-10 | Distance threshold for classifying schools. The smaller the school, the more subdivided the school |
| `prefixLen` | 4 | Fixed | Prefix length (first N moves of the procedure) |
| `minClusterSize` | null (automatic) | 2-15 | Minimum number of members to be recognized as a school. If null, auto adjust according to sample size |
| `maxClusterSize` | 15 | 10-100 | Maximum number of members in one school |
| `minSchoolSize` | 3 | 2-10 | Minimum size accepted as a school. Anything less than this will be excluded |

### 2. Subgroup division parameters

| Parameter | Default | Range | Description |
|------------|------------|------|------|
| `subgroupThreshold` | null (automatic) | 1.5-4.0 | Subgroup division threshold within the school. Auto adjust if null |
| `minSubgroupSize` | 3 | 2-5 | Minimum subgroup size required for labeling |

### 3. Scoring weight distribution (%)

| Parameter | Default | Range | Description |
|------------|------------|------|------|
| `rankWeight` | 40 | 10-60 | Weight of school rank (preferential treatment for large schools) |
| `sizeWeight` | 30 | 5-40 | School size weight |

**Note**: `lengthWeight` was removed as it was redundant with QTM/HTM penalties. Move count is now controlled by `htmPenalty` and QTM penalty multipliers.

### 4. QTM/HTM Penalty Settings

| Parameter | Default | Range | Description |
|------------|------------|------|------|
| `qtmSubgroup` | 0.3 | 0-2.0 | QTM penalty multiplier when selecting candidates within subgroups |
| `htmPenalty` | 0.5 | 0-2.0 | HTM penalty multiplier (penalty = htm × multiplier). Set to 0 to disable |
| `qtmCase1` | 1.5 | 0.5-5.0 | QTM penalty multiplier when subgroup formation fails (HTM NOT used) |
| `qtmFinal` | 0.2 | 0-1.0 | QTM penalty multiplier for final ranking adjustment |
| `rotationEfficiencyBonus` | 1.0 | 0-2.0 | Rotation efficiency bonus multiplier (bonus = HTM/QTM × multiplier) |

**QTM Penalty System**:
- QTM parameters work as **penalties** (penalty = qtm × multiplier)
- Larger QTM values result in lower scores
- This prevents issues where very large QTM (e.g., > 15) would receive zero penalty

**HTM Penalty System**:
- HTM penalty works alongside QTM penalty (penalty = htm × multiplier)
- Can be set to 0 to disable HTM penalty (QTM-only mode)
- **NOT used in Case 1** (subgroup formation failure) where HTM doesn't differentiate well
- In Case 1, only QTM + rotation efficiency are used for evaluation

**Rotation Efficiency Bonus**:
- Efficiency = HTM / QTM (ranges from ~0.5 to 1.0)
- 1.0 = perfect efficiency (no 180° rotations)
- 0.5 = many 180° rotations
- Bonus = efficiency × rotationEfficiencyBonus
- Higher efficiency gives higher bonus, promoting efficient solutions

**Recommended Ranges**:
- `qtmSubgroup`: 0.2-0.5 (moderate penalty for subgroup selection)
- `htmPenalty`: 0.3-0.8 (adjust based on whether you want to consider move count)
- `qtmCase1`: 1.0-2.5 (higher penalty when quality selection is critical, HTM not used)
- `qtmFinal`: 0.1-0.3 (small adjustment to avoid QTM dominating ranking)
- `rotationEfficiencyBonus`: 0.5-1.5 (reward efficient rotations)

**Migration from Bonus to Penalty System (2026-01-12)**:

The QTM calculation was changed from a bonus system to a penalty system to fix issues with large QTM values:

*Problem with the old bonus system*:
```javascript
// Old implementation
const qtmBonus = Math.max(0, (15 - qtm) * multiplier);
const totalScore = recScore + qtmBonus;

// Issue: When qtm > 15, bonus becomes 0 (no penalty!)
// Example: QTM=16 and QTM=30 both get bonus=0 (unfair evaluation)
```

*Solution - Current penalty system*:
```javascript
// New implementation
const qtmPenalty = qtm * multiplier;
const totalScore = recScore - qtmPenalty;

// Benefit: All QTM values are properly penalized
// Example: QTM=16 gets penalty=4.8, QTM=30 gets penalty=9.0 (fair evaluation)
```

This change ensures that solutions with very large QTM values (common in long solutions like those in 6.txt) are properly evaluated. The penalty is now proportional to QTM, making the scoring more linear and intuitive.

Modified locations in filter.js:
- Subgroup candidate selection (line ~867)
- Case 1: Subgroup formation failure (line ~949)
- Case 2: Single subgroup only (line ~1021)
- Case 3: Multiple subgroups (line ~1076)
- Final ranking (line ~1114)

### 5. Weight distribution for similarity calculation

| Parameter | Default | Range | Description |
|------------|------------|------|------|
| `endFaceWeight` | 40 | 20-60 | Weight of face similarity during school classification |
| `endSequenceWeight` | 40 | 20-60 | Jaccard similarity weight when dividing into subgroups |

### 6. Focus Moves Parameter

| Parameter | Default | Range | Description | Status |
|------------|------------|------|------|--------|
| `focusMoves` | 5 | 3-10 | Number of end moves to focus on for school classification | ✅ Active |

**How it works**:
- Controls how many moves from the end of each solution to analyze for school similarity
- If solution is shorter than focusMoves, uses entire solution
- Helps classify solutions based on their ending patterns (which are often more important)

### 7. Advanced Parameters

| Parameter | Default | Range | Description | Status |
|------------|------------|------|------|--------|
| `outlierThreshold` | 3.5 | 2.0-5.0 | Threshold for outlier detection (Modified Z-Score method) | ⚠️ Currently unused |

**Note about outlierThreshold**: This parameter is defined and passed through the config object, but the `detectOutliers()` function is not currently called in the main filtering logic. It was originally used in the `isAnomalous()` function for quality filtering. See DEVELOPER_GUIDE.md for implementation examples if you wish to enable outlier detection.

---

## How to use

### From JavaScript/Node.js

```javascript
const config = {
    threshold: 4.5,
    minClusterSize: 5,
    maxClusterSize: 20,
    subgroupThreshold: 2.5,
    minSubgroupSize: 3,
    rankWeight: 40,
    sizeWeight: 30,
    focusMoves: 5,
    qtmSubgroup: 0.3,
    htmPenalty: 0.5,
    qtmCase1: 1.5,
    qtmFinal: 0.2,
    rotationEfficiencyBonus: 1.0,
    endFaceWeight: 40,
    endSequenceWeight: 40
};

const results = getFinalLabelList(solutions, config);
```

### From Web Worker

```javascript
worker.postMessage({
    solutions: solutions,
    config: {
        threshold: 4.5,
        minClusterSize: 5,
        // ... Other parameters
    }
});
```

### From HTML tools

All parameters are adjustable in the UI.
Open `filter_worker_example_enhanced.html` and adjust using the sliders and input fields.

## Recommended settings

### Small sample (~150 solutions)
```javascript
{
    threshold: 4,
    minClusterSize: 5,
    maxClusterSize: 15,
    subgroupThreshold: 2.5
}
```

### Medium sample (150-500 solutions)
```javascript
{
    threshold: 5,
    minClusterSize: 3,
    maxClusterSize: 30,
    subgroupThreshold: 3.0
}
```

### Large-scale sample (500 solutions ~)
```javascript
{
    threshold: 6,
    minClusterSize: 2,
    maxClusterSize: 50,
    subgroupThreshold: 3.5
}
```

## About automatic adjustment

The following parameters can be set to `null` and will be automatically adjusted according to the sample size:

-`minClusterSize`: n≤150 → 5, n≤500 → 3, other → 2
-`subgroupThreshold`: n≤150 → 2.5, other → 3.0

The valid values of other parameters (threshold, maxClusterSize, etc.) are also adjusted according to the sample size.
