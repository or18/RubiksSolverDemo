# Changelog - Filter.js Improvements

## 2026-01-13: Parameter Normalization & Subgroup Display Improvements

### Major Changes

#### 1. Parameter Normalization System
**Problem**: Different parameters had vastly different scales, making UI adjustments non-intuitive.
- `rankWeight=40`, `sizeWeight=30` vs `rotationEfficiencyBonus×0.8 = 0.8 points`
- Adjusting `rotationEfficiencyBonus` had minimal visible impact on results

**Solution**: Normalized all parameters to similar scale (~10 points at multiplier=1.0)
```javascript
// Normalization constants
const TYPICAL_HTM = 8;          // Typical solution length in HTM
const TYPICAL_QTM = 10;         // Typical solution length in QTM
const TYPICAL_EFFICIENCY = 0.8; // Typical rotation efficiency
const PENALTY_SCALE = 10;       // Scale factor

// Normalized formulas (each ~10 points at multiplier=1.0)
htmPenalty = (htm / TYPICAL_HTM) × multiplier × PENALTY_SCALE
qtmPenalty = (qtm / TYPICAL_QTM) × multiplier × PENALTY_SCALE
efficiencyBonus = (efficiency / TYPICAL_EFFICIENCY) × multiplier × PENALTY_SCALE
rankScore = rankScore × rankWeight × PENALTY_SCALE × 4.0  (~40 points at 1.0)
sizeScore = sizeScore × sizeWeight × PENALTY_SCALE × 3.0  (~30 points at 1.0)
```

**Updated Parameters**:
- `rankWeight`: 40 → 1.0 (range: 0-3.0)
- `sizeWeight`: 30 → 1.0 (range: 0-3.0)
- `qtmSubgroup`: 0.3 → 1.0 (range: 0-3.0)
- `htmPenalty`: 0.5 → 1.0 (range: 0-3.0)
- `qtmCase1`: 1.5 (range: 0-5.0)
- `qtmFinal`: 0.2 → 0.5 (range: 0-2.0)
- `rotationEfficiencyBonus`: 1.0 (range: 0-3.0)

**Files Modified**:
- `filter.js`: Updated all 6 scoring locations + `calculateRecommendationScore()`
- `filter_worker_example_enhanced.html`: Updated all UI ranges and presets
- `docs/PARAMETERS.md`: Updated documentation

#### 2. Subgroup Information Display Improvements
**Problem**: Subgroup information was missing for many solutions

**Root Causes**:
1. Labeled solutions (Representative, Alternative) didn't have subgroup info
2. Only subgroups with size ≥ minSubgroupSize had info recorded
3. Solutions selected as "supplemental Alternatives" showed "too small for labeling" despite being labeled

**Solution**:
1. ✅ Added subgroup info to ALL solutions (labeled and unlabeled)
2. ✅ Record subgroup info for ALL subgroups (regardless of size)
3. ✅ Distinguish three cases in display:
   - Valid subgroup (size ≥ minSubgroupSize): `Sub Group 1/5 (Size 3)`
   - Small subgroup but selected: `Sub Group 2/5 (Size 1, selected from school-wide candidates)`
   - Small subgroup not selected: `Sub Group 3/5 (Size 1, too small)`

**Labeling Logic Explained**:
- **Subgroup-based selection**: From valid subgroups (size ≥ minSubgroupSize)
- **Supplemental selection**: When not enough Alternatives, select from entire School
  - Case 1 (no valid subgroups): Select top 2-3 by QTM efficiency
  - Case 2 (1 valid subgroup): Supplement 1-2 Alternatives from remaining solutions
  - Case 3 (multiple subgroups): Supplement if needed

**Files Modified**:
- `filter.js`: Updated subgroup info recording logic
- `filter_worker_example_enhanced.html`: Improved display with contextual messages

#### 3. School Sorting Fix
**Problem**: School_11 appeared before School_2 (string comparison)

**Solution**: Numeric extraction and comparison
```javascript
const getSchoolNum = (school) => {
    const match = school.match(/School_(\d+)/);
    return match ? parseInt(match[1], 10) : 999;
};
filtered.sort((a, b) => getSchoolNum(a.school) - getSchoolNum(b.school));
```

### Testing Results

**Before Normalization**:
```
Score differentiation: 88.19 vs 83.27
rotationEfficiencyBonus impact: minimal
```

**After Normalization**:
```
Score differentiation: 96.89 vs 94.27
All parameters have clear visible impact
UI adjustments are responsive and predictable
```

**Subgroup Display Example**:
```
School_1 (7 solutions, 5 subgroups):
#9 - Representative - Sub Group 1/5 (Size 3)
#8 - Shortest Alternative - Sub Group 1/5 (Size 3)
#2 - Alternative - Sub Group 2/5 (Size 1, selected from school-wide candidates)
#1 - Unlabeled - Sub Group 4/5 (Size 1, too small)
```

### Migration Guide

If you have existing config files, update parameter values:

**Old Config**:
```javascript
{
    rankWeight: 40,
    sizeWeight: 30,
    qtmSubgroup: 0.3,
    htmPenalty: 0.5,
    qtmFinal: 0.2
}
```

**New Config**:
```javascript
{
    rankWeight: 1.0,    // 40 → 1.0
    sizeWeight: 1.0,    // 30 → 1.0
    qtmSubgroup: 1.0,   // 0.3 → 1.0
    htmPenalty: 1.0,    // 0.5 → 1.0
    qtmFinal: 0.5       // 0.2 → 0.5
}
```

### Breaking Changes

⚠️ **Parameter value ranges changed** - existing configs need manual conversion
⚠️ **Default values changed** - unspecified parameters will use new defaults

### Backward Compatibility

The algorithm logic remains unchanged - only the parameter scaling is different.
If you use default values, the relative impact is maintained.

---

## Previous Changes

See git history for earlier modifications including:
- QTM penalty system (bonus → penalty conversion)
- HTM penalty addition
- Rotation efficiency bonus system
- Focus moves parameter
- Web Worker implementation
