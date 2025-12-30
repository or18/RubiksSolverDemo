# Comprehensive Memory Measurement Results

**Date**: 2025-12-30  
**Test Count**: 47 points (300 MB - 2000 MB)  
**Solver Binary**: solver_dev_test (MD5: 5066d1321c2a02310b4c85ff98dbbfd3)  
**Duration**: ~2 hours (150s per test)

> **Navigation**: [← Back to Experiences](README.md) | [←← Developer Docs](../../README.md) | [User Guide](../../../USER_GUIDE.md)
>
> **Related**: [Summary](MEASUREMENT_SUMMARY.md) | [Theory Analysis](MEMORY_THEORY_ANALYSIS.md) | [Model Revision](THEORETICAL_MODEL_REVISION.md)

## Executive Summary

### Key Findings

1. ✅ **Threshold Precision**: All 6 identified thresholds function perfectly with ±1 MB accuracy
2. ⚠️ **Theory-Practice Gap**: Theoretical predictions show systematic bias:
   - Small buckets (2M/2M/2M, 2M/4M/2M): Theory **underestimates** by +22-27%
   - Medium buckets (4M/4M/4M, 4M/8M/4M): Theory accurate within ±6%
   - Large buckets (8M+): Theory **overestimates** by -10% to -26%
3. 📊 **Memory Spike Characteristics**:
   - Rate: Constant ~2700 MB/s (independent of memory limit)
   - Count: Increases with bucket size (6 → 50 spikes)
   - Variance: High (coefficient of variation ~30%)
4. ✅ **Configuration Stability**: Peak RSS is highly consistent within same bucket configuration (±0.2 MB)

### Verified Thresholds

| Threshold | Before | After | Precision | Status |
|-----------|--------|-------|-----------|--------|
| 569 MB | 2M/2M/2M | 2M/4M/2M | Exact | ✓ Verified |
| 613 MB | 2M/4M/2M | 4M/4M/4M | Exact | ✓ Verified |
| 921 MB | 4M/4M/4M | 4M/8M/4M | Exact | ✓ Verified |
| 1000 MB | 4M/8M/4M | 8M/8M/8M | Exact | ✓ Verified |
| 1537 MB | 8M/8M/8M | 8M/16M/8M | Exact | ✓ Verified |
| 1702 MB | 8M/16M/8M | 16M/16M/16M | Exact | ✓ Verified |

## Detailed Analysis by Configuration

### Configuration 1: 2M/2M/2M (300-559 MB)

**Test Points**: 5 (300, 400, 500, 549, 559 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 231.0 MB |
| Avg Actual Peak | 293.4 MB |
| Difference | +62.4 MB (+27.0%) |
| Peak Stability | ±0.2 MB (0.07%) |
| Avg Spikes | 6.6 |
| Avg Spike Rate | 2692 MB/s |

**Observations**:
- Theory significantly underestimates actual memory usage
- Actual peak extremely stable across all memory limits
- Relatively few memory spikes compared to larger configurations
- **Likely cause**: Fixed overhead not captured in theoretical model (Phase 1 remnants, OS allocator overhead)

**Detailed Results**:
```
Limit   Theo    Actual  Diff      Spikes  Rate
300 MB  231.0   293.4   +62.4     7       2833
400 MB  231.0   293.4   +62.4     7       2675
500 MB  231.0   293.4   +62.4     5       2439
549 MB  231.0   293.2   +62.2     8       2719
559 MB  231.0   293.4   +62.4     6       2735
```

---

### Configuration 2: 2M/4M/2M (569-603 MB)

**Test Points**: 6 (569, 579, 589, 593, 600, 603 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 330.0 MB |
| Avg Actual Peak | 403.8 MB |
| Difference | +73.8 MB (+22.4%) |
| Peak Stability | ±0.1 MB (0.02%) |
| Avg Spikes | 8.0 |
| Avg Spike Rate | 2668 MB/s |

**Observations**:
- Theory still underestimates but gap is narrowing (27% → 22%)
- Peak RSS remarkably stable (403.7-403.8 MB)
- Spike count increased slightly from 2M/2M/2M configuration
- **Bucket upgrade (bucket_8: 2M→4M) adds ~110 MB actual peak**

**Detailed Results**:
```
Limit   Theo    Actual  Diff      Spikes  Rate
569 MB  330.0   403.8   +73.8     8       2798
579 MB  330.0   403.7   +73.7     6       2602
589 MB  330.0   403.7   +73.7     9       2651
593 MB  330.0   403.8   +73.8     8       2489
600 MB  330.0   403.8   +73.8     8       2709
603 MB  330.0   403.8   +73.8     9       2753
```

---

### Configuration 3: 4M/4M/4M (613-911 MB)

**Test Points**: 8 (613, 623, 633, 700, 800, 900, 901, 911 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 462.0 MB |
| Avg Actual Peak | 434.1 MB |
| Difference | -27.9 MB (-6.0%) |
| Peak Stability | ±0.1 MB (0.02%) |
| Avg Spikes | 16.4 |
| Avg Spike Rate | 2731 MB/s |

**Observations**:
- **CROSSOVER POINT**: Theory now overestimates (switched from underestimate)
- Excellent accuracy: Only -6.0% error
- Spike count doubled compared to 2M/4M/2M
- Peak RSS extremely stable across wide memory range (613-911 MB)
- **This configuration represents the "sweet spot" for theoretical predictions**

**Detailed Results**:
```
Limit   Theo    Actual  Diff      Spikes  Rate
613 MB  462.0   434.1   -27.9     19      2751
623 MB  462.0   434.2   -27.8     15      2709
633 MB  462.0   434.1   -27.9     17      2726
700 MB  462.0   434.1   -27.9     18      2704
800 MB  462.0   434.1   -27.9     13      2787
900 MB  462.0   434.1   -27.9     18      2710
901 MB  462.0   434.1   -27.9     17      2783
911 MB  462.0   434.1   -27.9     14      2728
```

---

### Configuration 4: 4M/8M/4M (921-990 MB)

**Test Points**: 5 (921, 931, 941, 980, 990 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 660.0 MB |
| Avg Actual Peak | 654.9 MB |
| Difference | -5.1 MB (-0.8%) |
| Peak Stability | ±0.0 MB (0.0%) |
| Avg Spikes | 17.6 |
| Avg Spike Rate | 2698 MB/s |

**Observations**:
- **BEST ACCURACY**: Only -0.8% error - nearly perfect prediction!
- All 5 test points show identical actual peak (654.9 MB)
- Theory slightly overestimates but within measurement noise
- Spike count similar to 4M/4M/4M configuration

**Detailed Results**:
```
Limit   Theo    Actual  Diff      Spikes  Rate
921 MB  660.0   654.9   -5.1      18      2587
931 MB  660.0   654.9   -5.1      20      2695
941 MB  660.0   654.9   -5.1      20      2784
980 MB  660.0   654.9   -5.1      12      2737
990 MB  660.0   654.9   -5.1      18      2686
```

---

### Configuration 5: 8M/8M/8M (1000-1527 MB)

**Test Points**: 10 (1000, 1010, 1020, 1100, 1200, 1300, 1400, 1500, 1517, 1527 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 924.0 MB |
| Avg Actual Peak | 747.7 MB |
| Difference | -176.3 MB (-19.1%) |
| Peak Stability | ±0.1 MB (0.01%) |
| Avg Spikes | 28.8 |
| Avg Spike Rate | 2743 MB/s |

**Observations**:
- Theory significantly overestimates (-19.1%)
- Spike count nearly doubled from 4M/8M/4M
- Actual peak remarkably consistent (747.6-747.8 MB) across 500+ MB range
- **Hypothesis**: Spike reserve formula (max_bucket × 33 × 0.5) too aggressive for large buckets

**Detailed Results**:
```
Limit    Theo    Actual  Diff      Spikes  Rate
1000 MB  924.0   747.7   -176.3    29      2721
1010 MB  924.0   747.7   -176.3    28      2811
1020 MB  924.0   747.6   -176.4    28      2740
1100 MB  924.0   747.7   -176.3    29      2760
1200 MB  924.0   747.8   -176.2    27      2710
1300 MB  924.0   747.7   -176.3    34      2798
1400 MB  924.0   747.8   -176.2    28      2744
1500 MB  924.0   747.7   -176.3    29      2710
1517 MB  924.0   747.6   -176.4    25      2731
1527 MB  924.0   747.7   -176.3    31      2809
```

---

### Configuration 6: 8M/16M/8M (1537-1700 MB)

**Test Points**: 7 (1537, 1547, 1557, 1600, 1682, 1692, 1700 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 1320.0 MB |
| Avg Actual Peak | 1189.3 MB |
| Difference | -130.7 MB (-9.9%) |
| Peak Stability | ±0.1 MB (0.01%) |
| Avg Spikes | 41.6 |
| Avg Spike Rate | 2740 MB/s |

**Observations**:
- Theory overestimates by -9.9%
- Spike count increased significantly (28.8 → 41.6)
- Peak RSS very stable (1189.2-1189.4 MB)
- Error smaller than 8M/8M/8M but still substantial

**Detailed Results**:
```
Limit    Theo     Actual   Diff      Spikes  Rate
1537 MB  1320.0   1189.2   -130.8    42      2738
1547 MB  1320.0   1189.3   -130.7    41      2625
1557 MB  1320.0   1189.4   -130.6    46      2737
1600 MB  1320.0   1189.3   -130.7    50      2774
1682 MB  1320.0   1189.3   -130.7    40      2738
1692 MB  1320.0   1189.2   -130.8    39      2740
1700 MB  1320.0   1189.2   -130.8    33      2782
```

---

### Configuration 7: 16M/16M/16M (1702-2000 MB)

**Test Points**: 6 (1702, 1712, 1722, 1800, 1900, 2000 MB)

| Metric | Value |
|--------|-------|
| Avg Theoretical Peak | 1848.0 MB |
| Avg Actual Peak | 1374.8 MB |
| Difference | -473.2 MB (-25.6%) |
| Peak Stability | ±0.2 MB (0.01%) |
| Avg Spikes | 50.0 |
| Avg Spike Rate | 2739 MB/s |

**Observations**:
- **Largest prediction error**: Theory overestimates by -25.6%
- Spike count highest of all configurations (50 avg, up to 69 max)
- Peak RSS stable (1374.6-1375.0 MB)
- Error magnitude similar to 2M/2M/2M but in opposite direction
- **Critical for theoretical model revision**

**Detailed Results**:
```
Limit    Theo     Actual   Diff      Spikes  Rate
1702 MB  1848.0   1374.6   -473.4    64      2740
1712 MB  1848.0   1374.9   -473.1    52      2726
1722 MB  1848.0   1374.7   -473.3    69      2708
1800 MB  1848.0   1374.9   -473.1    38      2737
1900 MB  1848.0   1375.0   -473.0    38      2766
2000 MB  1848.0   1374.9   -473.1    39      2726
```

---

## Cross-Configuration Analysis

### Theory vs Actual Error Progression

| Config | Bucket Total | Theo Peak | Actual Peak | Error | Error % |
|--------|--------------|-----------|-------------|-------|---------|
| 2M/2M/2M | 6M | 231.0 MB | 293.4 MB | +62.4 MB | +27.0% |
| 2M/4M/2M | 8M | 330.0 MB | 403.8 MB | +73.8 MB | +22.4% |
| 4M/4M/4M | 12M | 462.0 MB | 434.1 MB | -27.9 MB | -6.0% |
| 4M/8M/4M | 16M | 660.0 MB | 654.9 MB | -5.1 MB | -0.8% |
| 8M/8M/8M | 24M | 924.0 MB | 747.7 MB | -176.3 MB | -19.1% |
| 8M/16M/8M | 32M | 1320.0 MB | 1189.3 MB | -130.7 MB | -9.9% |
| 16M/16M/16M | 48M | 1848.0 MB | 1374.8 MB | -473.2 MB | -25.6% |

**Pattern Identification**:
- **Underestimation zone**: Buckets ≤ 8M total (+22% to +27%)
- **Accurate zone**: Buckets 12M-16M total (-6% to -1%)
- **Overestimation zone**: Buckets ≥ 24M total (-10% to -26%)

**Error appears to correlate with bucket size, not memory limit!**

### Memory Spike Analysis

#### Spike Count vs Bucket Size

| Config | Avg Spikes | Min | Max | StdDev | Notes |
|--------|------------|-----|-----|--------|-------|
| 2M/2M/2M | 6.6 | 5 | 8 | 1.1 | Lowest variance |
| 2M/4M/2M | 8.0 | 6 | 9 | 1.1 | Slight increase |
| 4M/4M/4M | 16.4 | 13 | 19 | 2.1 | Doubled |
| 4M/8M/4M | 17.6 | 12 | 20 | 3.4 | Similar to 4M/4M/4M |
| 8M/8M/8M | 28.8 | 25 | 34 | 2.6 | Jump at 8M |
| 8M/16M/8M | 41.6 | 33 | 50 | 5.8 | Continued increase |
| 16M/16M/16M | 50.0 | 38 | 69 | 12.4 | Highest & most variable |

**Observations**:
- Spike count increases with total bucket capacity
- Variance also increases with bucket size
- Approximate relationship: `Spikes ≈ 1.5 × (Total_Buckets_MB)`

#### Spike Rate Consistency

| Metric | Value |
|--------|-------|
| Overall Average | 2714 MB/s |
| Standard Deviation | 89 MB/s (3.3%) |
| Min | 2439 MB/s |
| Max | 2833 MB/s |
| Range | 394 MB/s |

**Conclusion**: Spike rate is remarkably constant regardless of:
- Memory limit (300 MB - 2000 MB)
- Bucket configuration
- Spike count

This suggests spike rate is limited by **hardware characteristics** (memory bandwidth, CPU cache speed), not algorithmic factors.

---

## Theoretical Model Issues

### Current Theoretical Formula

```
Theoretical Peak = Baseline + Spike Reserve

Baseline = (bucket_7 + bucket_8 + bucket_9) × 33 bytes/slot
Spike Reserve = max(bucket_7, bucket_8, bucket_9) × 33 × 0.5
```

### Problem Diagnosis

#### Issue 1: Fixed Overhead Missing (Small Buckets)

**Evidence**: 2M/2M/2M shows +62 MB constant overhead

**Hypothesis**: 
- Phase 1 memory (217 MB peak) leaves residual allocations
- OS allocator metadata not freed immediately
- Arena/heap fragmentation
- Stack, thread-local storage, globals

**Proposed Addition**:
```
Fixed_Overhead = 60-65 MB  // Empirically derived
```

#### Issue 2: Spike Reserve Overestimation (Large Buckets)

**Evidence**: 
- 8M/8M/8M: Spike reserve theory = 132 MB, but actual suggests ~0 MB
- 16M/16M/16M: Spike reserve theory = 264 MB, but actual suggests minimal

**Current Formula**:
```
Spike Reserve = max_bucket × 33 × 0.5
```

**Analysis**:
```
Config       Max_Bucket  Theory_Spike  Actual_Overage  Implied_Spike
2M/2M/2M     2M          33 MB         +62 MB          95 MB (!)
4M/4M/4M     4M          66 MB         -28 MB          38 MB
8M/8M/8M     8M          132 MB        -176 MB         0 MB (?)
16M/16M/16M  16M         264 MB        -473 MB         0 MB (?)
```

**Hypothesis**:
- Spike reserve formula works for small buckets but becomes progressively inaccurate
- Large hash tables may have better amortized rehashing behavior
- Modern memory allocators (tcmalloc, jemalloc) may reuse freed blocks more efficiently
- Possible double-counting with baseline memory

#### Issue 3: Baseline Calculation Accuracy

**Current Formula**:
```
Baseline = total_nodes × 33 bytes
```

**Evidence from 4M/4M/4M** (most accurate configuration):
```
Theoretical Baseline = 396 MB
Theoretical Spike = 66 MB
Theoretical Total = 462 MB

Actual Peak = 434 MB
Implied Actual Baseline = 434 - overhead = ?
```

If we assume spike reserve is correct (66 MB):
```
Actual Baseline = 434 - 66 = 368 MB
Theory says 396 MB
Difference = -28 MB (-7%)
```

This suggests 33 bytes/slot may be slightly high, or load factor isn't exactly 0.9.

---

## Recommendations for Model Revision

### Priority 1: Add Fixed Overhead Component

```python
FIXED_OVERHEAD_MB = 62  # Empirically measured from 2M/2M/2M config

theoretical_peak = FIXED_OVERHEAD_MB + baseline + spike_reserve
```

### Priority 2: Revise Spike Reserve Formula

**Option A: Piecewise Function**
```python
if max_bucket_mb <= 4:
    spike_reserve = max_bucket_mb * 1024 * 1024 * 33 * 0.5
elif max_bucket_mb <= 8:
    spike_reserve = max_bucket_mb * 1024 * 1024 * 33 * 0.2
else:
    spike_reserve = 0  # Large buckets don't show measurable spikes
```

**Option B: Logarithmic Decay**
```python
import math
spike_factor = 0.5 * math.exp(-0.15 * max_bucket_mb)
spike_reserve = max_bucket_mb * 1024 * 1024 * 33 * spike_factor
```

**Option C: Empirical Table**
```python
SPIKE_FACTORS = {
    2: 0.8,   # 2M buckets: higher relative overhead
    4: 0.4,   # 4M buckets: moderate
    8: 0.1,   # 8M buckets: minimal
    16: 0.0,  # 16M+ buckets: negligible
}
```

### Priority 3: Validate Baseline Calculation

Measure actual load factors and bytes/slot for each configuration:
```bash
# Extract from solver logs:
- Initial bucket sizes
- Final node counts
- Final bucket sizes (after rehashing)
- Calculate actual load factor and memory/node
```

---

## Production Memory Recommendations

### Safe Minimums with 10% Safety Margin

| Configuration | Measured Peak | Recommended Minimum |
|---------------|---------------|---------------------|
| 2M/2M/2M | 293 MB | **325 MB** |
| 2M/4M/2M | 404 MB | **445 MB** |
| 4M/4M/4M | 434 MB | **480 MB** |
| 4M/8M/4M | 655 MB | **725 MB** |
| 8M/8M/8M | 748 MB | **825 MB** |
| 8M/16M/8M | 1189 MB | **1310 MB** |
| 16M/16M/16M | 1375 MB | **1515 MB** |

### Threshold Safety Margins

To guarantee a specific configuration, use:

| Target Config | Guaranteed Minimum |
|---------------|-------------------|
| 2M/2M/2M | 300 MB |
| 2M/4M/2M | 570 MB |
| 4M/4M/4M | 615 MB |
| 4M/8M/4M | 925 MB |
| 8M/8M/8M | 1005 MB |
| 8M/16M/8M | 1540 MB |
| 16M/16M/16M | 1705 MB |

---

## Data Files Reference

### Primary Data
- **Full Results CSV**: `backups/logs/tmp_log_run_measurements_20251230_1715/tmp_log/measurement_results.csv`
- **Progress Log**: `backups/logs/tmp_log_run_measurements_20251230_1715/measurement_log_20251230.txt`
- **Threshold Verification**: `backups/logs/tmp_log_threshold_test/verify_thresholds.sh_up_to_2GB.log`

### Per-Test Logs
Individual solver and monitor logs for each test point in:
- `backups/logs/tmp_log_run_measurements_20251230_1715/tmp_log/solver_*MB.log`
- `backups/logs/tmp_log_run_measurements_20251230_1715/tmp_log/monitor_*MB.log`
- `backups/logs/tmp_log_run_measurements_20251230_1715/tmp_log/rss_*MB.csv`

### Theoretical Predictions
- `backups/logs/tmp_log_run_measurements_20251230_1715/tmp_log/theoretical_predictions_corrected.csv`

---

## Next Steps

1. **Detailed Log Analysis**
   - Extract actual load factors from solver logs
   - Analyze RSS time-series for spike patterns
   - Measure exact node counts vs bucket capacities

2. **Theoretical Model Revision**
   - Implement fixed overhead correction
   - Test revised spike reserve formulas
   - Validate against all 47 measurement points

3. **Documentation**
   - Create theoretical alignment document
   - Update user guide with production minimums
   - Add configuration selection flowchart

4. **Extended Testing**
   - Test edge cases (very small: 200 MB, very large: 3000 MB)
   - Validate across different hardware
   - Test with different cube scrambles/positions

---

*Generated: 2025-12-30*  
*Data Source: 47-point comprehensive measurement suite*  
*Analysis Scripts: Available in src/xxcrossTrainer/*
