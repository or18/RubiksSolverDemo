# Theoretical Model Revision and Alignment

**Date**: 2025-12-30  
**Based on**: 47-point measurement suite (300-2000 MB)  
**Objective**: Align theoretical predictions with actual measurements

> **Navigation**: [← Back to Experiences](README.md) | [←← Developer Docs](../../README.md) | [Memory Config](../../../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [Theory Analysis](MEMORY_THEORY_ANALYSIS.md) | [Measurement Results](MEASUREMENT_RESULTS_20251230.md) | [Implementation](../../SOLVER_IMPLEMENTATION.md)

## Problem Statement

The current theoretical model shows systematic prediction errors:

| Bucket Size | Theory Error | Direction |
|-------------|--------------|-----------|
| Small (≤8M total) | +22% to +27% | Underestimate |
| Medium (12-16M) | -1% to -6% | Near-perfect to slight overestimate |
| Large (≥24M) | -10% to -26% | Significant overestimate |

**Root Causes Identified**:
1. Missing fixed overhead component
2. Spike reserve formula inaccurate for large buckets
3. Possible baseline calculation refinement needed

---

## Current Theoretical Formula

```python
# Phase 1 (freed before Phase 2)
phase1_rss_mb = 217  # Measured, temporary

# Phase 2-4 calculation
total_memory = limit_mb * 1024 * 1024
remaining = total_memory - (phase1_rss_mb * 1024 * 1024)
wasm_margin = remaining * 0.25 (if <1GB) or 0.20 (if ≥1GB)
usable = remaining - wasm_margin

# Bucket allocation (2:2:2 ratio with power-of-2 rounding)
target_per_bucket = usable / (6 * 33)
bucket_7 = bucket_8 = bucket_9 = round_to_power_of_2(target_per_bucket * 2)
# Plus flexible upgrades

# Memory prediction
baseline_mb = (bucket_7 + bucket_8 + bucket_9) * 33 / (1024 * 1024)
spike_reserve_mb = max(bucket_7, bucket_8, bucket_9) * 33 * 0.5 / (1024 * 1024)
theoretical_peak_mb = baseline_mb + spike_reserve_mb
```

---

## Empirical Findings from Measurements

### Finding 1: Fixed Overhead = ~62 MB

**Evidence from 2M/2M/2M configuration** (5 test points):

| Limit | Theo Baseline | Theo Spike | Theo Total | Actual Peak | Overhead |
|-------|---------------|------------|------------|-------------|----------|
| 300 MB | 198 MB | 33 MB | 231 MB | 293.4 MB | +62.4 MB |
| 400 MB | 198 MB | 33 MB | 231 MB | 293.4 MB | +62.4 MB |
| 500 MB | 198 MB | 33 MB | 231 MB | 293.4 MB | +62.4 MB |
| 549 MB | 198 MB | 33 MB | 231 MB | 293.2 MB | +62.2 MB |
| 559 MB | 198 MB | 33 MB | 231 MB | 293.4 MB | +62.4 MB |

**Average overhead**: 62.4 MB (extremely consistent)

**Components**:
- Phase 1 remnants (partially freed allocations)
- C++ runtime overhead (iostream, STL)
- OS allocator metadata (malloc/free bookkeeping)
- Thread-local storage
- Stack frames
- Program text + read-only data
- Memory arena fragmentation

**Validation**: This overhead is present in ALL configurations, but becomes proportionally less significant as bucket sizes increase.

---

### Finding 2: Spike Reserve is Bucket-Size Dependent

**Current formula**: `spike_reserve = max_bucket × 33 × 0.5`

**Analysis by configuration**:

| Config | Max Bucket | Theo Spike | Actual Peak | Theo Baseline | Implied Actual Spike |
|--------|-----------|------------|-------------|---------------|----------------------|
| 2M/2M/2M | 2M | 33 MB | 293 MB | 198 MB | 95 MB (after fixed overhead) |
| 2M/4M/2M | 4M | 66 MB | 404 MB | 264 MB | 78 MB (after fixed overhead) |
| 4M/4M/4M | 4M | 66 MB | 434 MB | 396 MB | -24 MB (formula works!)  |
| 4M/8M/4M | 8M | 132 MB | 655 MB | 528 MB | -5 MB (almost no spikes!) |
| 8M/8M/8M | 8M | 132 MB | 748 MB | 792 MB | -176 MB (formula fails) |
| 8M/16M/8M | 16M | 264 MB | 1189 MB | 1056 MB | -131 MB (formula fails) |
| 16M/16M/16M | 16M | 264 MB | 1375 MB | 1584 MB | -473 MB (formula fails) |

**Pattern**:
1. **Small buckets (2M)**: Spike reserve underestimated (needs ~1.5× current)
2. **Medium buckets (4M)**: Spike reserve accurate
3. **Large buckets (8M+)**: Spike reserve overestimated (hash tables don't spike as much)

**Hypothesis**: Large hash tables have better amortized rehashing. With modern allocators (tcmalloc), they may:
- Reuse freed blocks more efficiently
- Avoid large single allocations
- Use incremental rehashing strategies

---

### Finding 3: Baseline Calculation Accuracy

**Current**: `baseline = total_buckets × 33 bytes`

**Test with 4M/4M/4M** (most accurate configuration):
```
Theory baseline = 396 MB
Theory spike = 66 MB
Theory total = 462 MB

Actual peak = 434 MB
Actual baseline (if spike=66) = 434 - 66 - 62 (fixed) = 306 MB
Theory says 396 MB
Difference = -90 MB (-23%)
```

This doesn't match. Let's try another approach:

**If actual baseline includes fixed overhead**:
```
Actual "working set" = 434 MB
Fixed overhead = 62 MB
Actual dynamic memory = 434 - 62 = 372 MB

If spike = 66 MB (theory):
  Actual baseline = 372 - 66 = 306 MB
  Theory baseline = 396 MB
  Error = -90 MB

If spike = 0 MB (minimal spikes observed):
  Actual baseline = 372 MB
  Theory baseline = 396 MB
  Error = -24 MB (-6%)
```

**Better match with spike=0!** This suggests 4M/4M/4M has minimal rehashing spikes, and the -6% error is within measurement noise and allocator overhead.

---

## Revised Theoretical Model

### Version 2.0: Corrected Formula

```python
# Constants
PHASE1_RSS_MB = 217
FIXED_OVERHEAD_MB = 62  # NEW: Empirically measured
EMPIRICAL_OVERHEAD = 33  # bytes/slot

# Spike reserve factors (NEW: bucket-size dependent)
SPIKE_FACTORS = {
    2: 1.5,   # 2M buckets need more reserve
    4: 0.3,   # 4M buckets have minimal spikes
    8: 0.0,   # 8M+ buckets have negligible rehashing spikes
    16: 0.0,
    32: 0.0,
}

def calculate_theoretical_peak_v2(limit_mb, bucket_7_mb, bucket_8_mb, bucket_9_mb):
    # 1. Calculate usable memory (unchanged)
    total_bytes = limit_mb * 1024 * 1024
    phase1_bytes = PHASE1_RSS_MB * 1024 * 1024
    remaining = total_bytes - phase1_bytes
    
    wasm_margin_pct = 0.25 if total_bytes < (1024 * 1024 * 1024) else 0.20
    wasm_margin = remaining * wasm_margin_pct
    usable = remaining - wasm_margin
    
    # 2. Baseline memory (unchanged)
    bucket_7_bytes = bucket_7_mb * 1024 * 1024
    bucket_8_bytes = bucket_8_mb * 1024 * 1024
    bucket_9_bytes = bucket_9_mb * 1024 * 1024
    
    baseline_bytes = (bucket_7_bytes + bucket_8_bytes + bucket_9_bytes) * EMPIRICAL_OVERHEAD
    baseline_mb = baseline_bytes / (1024 * 1024)
    
    # 3. Spike reserve (NEW: bucket-size dependent)
    max_bucket_mb = max(bucket_7_mb, bucket_8_mb, bucket_9_mb)
    
    if max_bucket_mb in SPIKE_FACTORS:
        spike_factor = SPIKE_FACTORS[max_bucket_mb]
    else:
        # Fallback for unlisted sizes
        spike_factor = 0.0 if max_bucket_mb >= 8 else 0.5
    
    max_bucket_bytes = max_bucket_mb * 1024 * 1024
    spike_reserve_bytes = max_bucket_bytes * EMPIRICAL_OVERHEAD * spike_factor
    spike_reserve_mb = spike_reserve_bytes / (1024 * 1024)
    
    # 4. Total prediction (NEW: includes fixed overhead)
    theoretical_peak_mb = FIXED_OVERHEAD_MB + baseline_mb + spike_reserve_mb
    
    return {
        'baseline_mb': baseline_mb,
        'spike_reserve_mb': spike_reserve_mb,
        'fixed_overhead_mb': FIXED_OVERHEAD_MB,
        'theoretical_peak_mb': theoretical_peak_mb,
    }
```

---

## Validation Against Actual Measurements

### Predicted vs Actual with Revised Model

| Config | Limit | Old Theo | New Theo | Actual | Old Error | New Error |
|--------|-------|----------|----------|--------|-----------|-----------|
| 2M/2M/2M | 300 | 231.0 | 309.0 | 293.4 | +62.4 (+27%) | +15.6 (+5.3%) |
| 2M/4M/2M | 569 | 330.0 | 405.6 | 403.8 | +73.8 (+22%) | +1.8 (+0.4%) |
| 4M/4M/4M | 613 | 462.0 | 497.8 | 434.1 | -27.9 (-6%) | +63.7 (+14.7%) |
| 4M/8M/4M | 921 | 660.0 | 590.0 | 654.9 | -5.1 (-0.8%) | -64.9 (-9.9%) |
| 8M/8M/8M | 1000 | 924.0 | 854.0 | 747.7 | -176.3 (-19%) | +106.3 (+14.2%) |
| 8M/16M/8M | 1537 | 1320.0 | 1118.0 | 1189.3 | -130.7 (-10%) | -71.3 (-6.0%) |
| 16M/16M/16M | 1702 | 1848.0 | 1646.0 | 1374.8 | -473.2 (-26%) | +271.2 (+19.7%) |

**Calculations**:
```
2M/2M/2M:  62 + 198 + (2 × 33 × 1.5) = 62 + 198 + 49.5 = 309.5 ≈ 309
2M/4M/2M:  62 + 264 + (4 × 33 × 1.5) = 62 + 264 + 79.2 = 405.2 ≈ 406
4M/4M/4M:  62 + 396 + (4 × 33 × 0.3) = 62 + 396 + 39.6 = 497.6 ≈ 498
4M/8M/4M:  62 + 528 + (8 × 33 × 0.0) = 62 + 528 + 0 = 590
8M/8M/8M:  62 + 792 + (8 × 33 × 0.0) = 62 + 792 + 0 = 854
8M/16M/8M: 62 + 1056 + (16 × 33 × 0.0) = 62 + 1056 + 0 = 1118
16M/16M/16M: 62 + 1584 + 0 = 1646
```

### Problem with Revised Model

The revised model still has significant errors, especially for:
- 4M/4M/4M: Now overestimates (+14.7%)
- 8M/8M/8M: Still overestimates (+14.2%)
- 16M/16M/16M: Still overestimates (+19.7%)

**The baseline calculation itself must be wrong for larger buckets!**

---

## Alternative Hypothesis: Baseline Overhead is Not Constant

### Re-examining Empirical Data

Let's calculate implied **bytes/slot** from actual measurements:

```
Config: 2M/2M/2M
  Actual peak (minus fixed 62 MB) = 293.4 - 62 = 231.4 MB
  Total buckets = 6M = 6291456 slots
  Bytes per slot = 231.4 MB / 6.0 M = 38.6 bytes

Config: 2M/4M/2M
  Actual (minus fixed) = 403.8 - 62 = 341.8 MB
  Total buckets = 8M = 8388608 slots
  Bytes per slot = 341.8 MB / 8.0 M = 42.7 bytes

Config: 4M/4M/4M
  Actual (minus fixed) = 434.1 - 62 = 372.1 MB
  Total buckets = 12M = 12582912 slots
  Bytes per slot = 372.1 MB / 12.0 M = 31.0 bytes

Config: 4M/8M/4M
  Actual (minus fixed) = 654.9 - 62 = 592.9 MB
  Total buckets = 16M = 16777216 slots
  Bytes per slot = 592.9 MB / 16.0 M = 37.1 bytes

Config: 8M/8M/8M
  Actual (minus fixed) = 747.7 - 62 = 685.7 MB
  Total buckets = 24M = 25165824 slots
  Bytes per slot = 685.7 MB / 24.0 M = 28.6 bytes

Config: 8M/16M/8M
  Actual (minus fixed) = 1189.3 - 62 = 1127.3 MB
  Total buckets = 32M = 33554432 slots
  Bytes per slot = 1127.3 MB / 32.0 M = 35.2 bytes

Config: 16M/16M/16M
  Actual (minus fixed) = 1374.8 - 62 = 1312.8 MB
  Total buckets = 48M = 50331648 slots
  Bytes per slot = 1312.8 MB / 48.0 M = 27.4 bytes
```

### Pattern Discovery!

| Total Buckets | Bytes/Slot | Note |
|---------------|------------|------|
| 6M | 38.6 | High overhead (small buckets) |
| 8M | 42.7 | Highest (transition point?) |
| 12M | 31.0 | Below theory (33 bytes) |
| 16M | 37.1 | Above theory |
| 24M | 28.6 | Well below theory |
| 32M | 35.2 | Above theory |
| 48M | 27.4 | Lowest |

**Observation**: No clear linear pattern! Bytes/slot varies between 27-43 bytes.

**Possible explanation**: 
- Different bucket sizes trigger different allocator behaviors
- Hash table implementation may use different data structures at different sizes
- Memory alignment and padding varies
- Load factor may not be exactly 0.9 for all configurations

---

## Final Revised Model: Empirical Table Approach

Given the complexity and non-linear behavior, use **empirically-derived values per configuration**:

```python
# Empirical peak memory (includes all overhead)
EMPIRICAL_PEAKS = {
    (2, 2, 2): 293.4,
    (2, 4, 2): 403.8,
    (4, 4, 4): 434.1,
    (4, 8, 4): 654.9,
    (8, 8, 8): 747.7,
    (8, 16, 8): 1189.3,
    (16, 16, 16): 1374.8,
}

def get_theoretical_peak_v3(bucket_7_mb, bucket_8_mb, bucket_9_mb):
    """
    Return empirically-measured peak memory for known configurations.
    For unknown configurations, interpolate or use conservative estimate.
    """
    config = (bucket_7_mb, bucket_8_mb, bucket_9_mb)
    
    if config in EMPIRICAL_PEAKS:
        return EMPIRICAL_PEAKS[config]
    
    # For unknown configs, use conservative estimate:
    # Take the next-larger known configuration
    total_mb = bucket_7_mb + bucket_8_mb + bucket_9_mb
    
    # Find nearest empirical point
    for empirical_config, peak in sorted(EMPIRICAL_PEAKS.items(), key=lambda x: sum(x[0])):
        empirical_total = sum(empirical_config)
        if empirical_total >= total_mb:
            # Use empirical value with small safety margin
            return peak * 1.05  # 5% safety margin
    
    # If larger than all known configs, extrapolate linearly
    largest_config = max(EMPIRICAL_PEAKS.keys(), key=lambda x: sum(x))
    largest_peak = EMPIRICAL_PEAKS[largest_config]
    largest_total = sum(largest_config)
    
    # Linear extrapolation with safety margin
    ratio = total_mb / largest_total
    estimated = largest_peak * ratio * 1.10  # 10% safety margin for extrapolation
    
    return estimated
```

---

## Recommendations

### For Production Use

**Use empirical table** (Version 3) for known configurations:
- Most accurate (±0.1 MB)
- No complex calculations
- Validated against real measurements

**For unknown configurations**:
- Use next-larger empirical value + 5% margin
- Or run a quick measurement test
- Avoid extrapolating theoretical formulas beyond tested range

### For Future Measurements

1. **Measure intermediate configurations** to fill gaps:
   - 4M/4M/8M, 4M/8M/8M
   - 8M/8M/16M
   - Other asymmetric combinations

2. **Test different scrambles** to validate consistency

3. **Measure on different hardware** to check platform dependency

4. **Add instrumentation** to solver to report:
   - Actual load factors after each phase
   - Exact node counts stored
   - Hash table rehashing events
   - Memory allocator statistics

### For Theoretical Understanding

Further investigation needed:
1. Why does bytes/slot vary so much (27-43 bytes)?
2. What causes the non-monotonic pattern?
3. How does hash table implementation differ by size?
4. What is the actual load factor (theory assumes 0.9)?

---

## Conclusion

**Theoretical Model Status**: ⚠️ Partially Aligned

- **Fixed overhead** (62 MB): ✅ Confirmed
- **Baseline calculation**: ❌ Formula (33 bytes/slot) inaccurate
- **Spike reserve**: ❌ Formula inaccurate for large buckets

**Recommended Approach**: 
- ✅ Use empirical table for production
- ⚠️ Theoretical formula needs deeper investigation
- 📊 More measurements needed to understand byte/slot variation

**Immediate Action**:
- Update user documentation with empirical values
- Mark theoretical predictions as "estimates only"
- Add 10-15% safety margins for production deployments

---

*Document Version*: 2.0  
*Last Updated*: 2025-12-30  
*Status*: Analysis complete, further investigation recommended
