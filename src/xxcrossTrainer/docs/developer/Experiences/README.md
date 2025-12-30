# Experimental Findings Archive

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: [← Back to Developer Docs](../../README.md) | [User Guide](../../USER_GUIDE.md) | [Memory Config](../../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [Implementation](../SOLVER_IMPLEMENTATION.md) | [Memory Monitoring](../MEMORY_MONITORING.md) | [Experiment Scripts](../EXPERIMENT_SCRIPTS.md)

---

## Purpose

This directory archives **all experimental findings** from memory behavior investigations and measurement campaigns. These documents provide deep insights into solver memory characteristics and the evolution of our understanding.

---

## Key Documents

### 📊 Comprehensive Measurement Campaign (2025-12-30)

**[MEASUREMENT_RESULTS_20251230.md](MEASUREMENT_RESULTS_20251230.md)** - Primary reference
- 47-point comprehensive measurement results (300-2000 MB)
- Configuration-by-configuration analysis
- Theory vs empirical comparison
- Spike detection and analysis
- **Most detailed technical analysis**

**[MEASUREMENT_SUMMARY.md](MEASUREMENT_SUMMARY.md)** - Executive summary
- Key discoveries and recommendations
- Production configuration table
- Data files reference
- Quick-reference format

**[MEMORY_THEORY_ANALYSIS.md](MEMORY_THEORY_ANALYSIS.md)** - Theoretical deep-dive
- Load factor investigation (0.6-1.0 variation)
- Bytes-per-slot analysis (27-43 bytes)
- Spike reserve analysis
- Alternative model proposals
- **Best for understanding "why"**

**[THEORETICAL_MODEL_REVISION.md](THEORETICAL_MODEL_REVISION.md)** - Model evolution
- Formula history and limitations
- Empirical vs theoretical comparison
- Fixed overhead discovery (62 MB)
- Conclusion: Use empirical table

---

### 📈 Incremental Investigation (2025-12-29)

**[INVESTIGATION_20251229.md](INVESTIGATION_20251229.md)** - Daily investigation log
- Threshold discovery process
- Bug fixes and corrections
- Experimental methodology

**[INVESTIGATION_SUMMARY.md](INVESTIGATION_SUMMARY.md)** - Summary of findings
- Key insights from investigation
- Actionable recommendations

---

### 🔬 Specialized Analyses

**[SOLVER_VS_THEORY_COMPARISON.md](SOLVER_VS_THEORY_COMPARISON.md)**
- Direct comparison of solver output vs theoretical predictions
- Error analysis by configuration

**[MEMORY_ANALYSIS_DETAILED.md](MEMORY_ANALYSIS_DETAILED.md)**
- Detailed memory component breakdown
- RSS monitoring methodology

**[REHASH_PROTECTION.md](REHASH_PROTECTION.md)**
- Hash table rehashing behavior
- Robin-set implementation details

**[MEMORY_OPTIMIZATION_HISTORY.md](MEMORY_OPTIMIZATION_HISTORY.md)**
- Historical optimizations attempted
- Performance trade-offs documented

---

### 🧪 Initial Testing

**[INITIAL_TEST_RESULTS.md](INITIAL_TEST_RESULTS.md)**
- First measurement results
- Baseline configurations

**[THEORETICAL_PREDICTIONS.md](THEORETICAL_PREDICTIONS.md)** (Original)
- Initial theoretical model (33 bytes/slot)
- 32-point predictions (300-1500 MB)

**[THEORETICAL_PREDICTIONS_CORRECTED.md](THEORETICAL_PREDICTIONS_CORRECTED.md)**
- Extended to 47 points (300-2000 MB)
- All 6 thresholds discovered

---

### 📝 Source Code Documentation

**[SOURCE_CODE_VERSIONS.md](SOURCE_CODE_VERSIONS.md)**
- Binary version tracking
- MD5 checksums for validation
- Code change history

---

## Document Relationships

```
Production Deployment
├─ ../../MEMORY_CONFIGURATION_GUIDE.md (Use this for deployment)
│
Understanding Memory Behavior
├─ MEASUREMENT_RESULTS_20251230.md (Empirical data, most comprehensive)
├─ MEMORY_THEORY_ANALYSIS.md (Theoretical understanding)
└─ THEORETICAL_MODEL_REVISION.md (Model limitations)
    
Historical Context
├─ INVESTIGATION_20251229.md (How we got here)
├─ MEASUREMENT_SUMMARY.md (Quick reference)
└─ MEMORY_OPTIMIZATION_HISTORY.md (What we tried)

Technical Deep-Dives
├─ SOLVER_VS_THEORY_COMPARISON.md (Accuracy analysis)
├─ MEMORY_ANALYSIS_DETAILED.md (Component breakdown)
└─ REHASH_PROTECTION.md (Implementation details)
```

---

## Reading Guide

### For Production Deployment
1. **[../../MEMORY_CONFIGURATION_GUIDE.md](../../MEMORY_CONFIGURATION_GUIDE.md)** - Complete production guide
2. **[MEASUREMENT_SUMMARY.md](MEASUREMENT_SUMMARY.md)** - Quick reference

### For Understanding Memory Behavior
1. **[MEMORY_THEORY_ANALYSIS.md](MEMORY_THEORY_ANALYSIS.md)** - Start here for theory
2. **[MEASUREMENT_RESULTS_20251230.md](MEASUREMENT_RESULTS_20251230.md)** - Empirical validation
3. **[THEORETICAL_MODEL_REVISION.md](THEORETICAL_MODEL_REVISION.md)** - Model limitations

### For Debugging/Troubleshooting
1. **[SOLVER_VS_THEORY_COMPARISON.md](SOLVER_VS_THEORY_COMPARISON.md)** - Compare predictions
2. **[MEMORY_ANALYSIS_DETAILED.md](MEMORY_ANALYSIS_DETAILED.md)** - Component-level analysis
3. **[REHASH_PROTECTION.md](REHASH_PROTECTION.md)** - Hash table behavior

### For Historical Context
1. **[INVESTIGATION_20251229.md](INVESTIGATION_20251229.md)** - Investigation process
2. **[MEMORY_OPTIMIZATION_HISTORY.md](MEMORY_OPTIMIZATION_HISTORY.md)** - Optimization attempts
3. **[SOURCE_CODE_VERSIONS.md](SOURCE_CODE_VERSIONS.md)** - Code evolution

---

## Key Findings Summary

### Empirical Measurements (stable-20251230)

**7 Bucket Configurations**:
- 2M/2M/2M: 293 MB peak (CV: 0.027%)
- 2M/4M/2M: 404 MB peak (CV: 0.012%)
- 4M/4M/4M: 434 MB peak (CV: 0.008%)
- 4M/8M/4M: 655 MB peak (CV: 0.000%)
- 8M/8M/8M: 748 MB peak (CV: 0.008%) ← **Recommended**
- 8M/16M/8M: 1189 MB peak (CV: 0.006%)
- 16M/16M/16M: 1375 MB peak (CV: 0.010%)

**Thresholds** (±1 MB precision):
- 569, 613, 921, 1000, 1537, 1702 MB

**Memory Stability**:
- Peak variation: <0.5 MB within configuration
- CV < 0.03% across all configs
- 660,000+ samples analyzed

### Theoretical Insights

**Fixed Overhead**: 62 MB (constant across all configs)
- C++ runtime: ~15 MB
- Phase 1 remnants: ~20 MB
- OS allocator: ~10 MB
- Misc: ~17 MB

**Bytes per Slot**: 27-43 bytes (not constant 33!)
- Small buckets (2M-4M): 38-43 bytes/slot
- Medium buckets (4M-8M): 31-37 bytes/slot
- Large buckets (8M+): 27-29 bytes/slot

**Load Factor**: Varies by bucket size
- Small buckets: 0.9-1.0 (maximize space)
- Large buckets: 0.6-0.7 (maximize speed)

**Spike Reserve**: Not needed for large buckets
- Allocator efficiently reuses memory
- No measurable transient peaks for 8M+ buckets

---

## Measurement Campaign Stats

**Date**: 2025-12-30  
**Duration**: ~2 hours  
**Test Points**: 47 (300-2000 MB range)  
**Success Rate**: 100% (47/47)  
**Total Samples**: ~660,000 RSS measurements  
**Sampling Rate**: 10ms intervals  

**Binary**: solver_dev_test  
**MD5**: 5066d1321c2a02310b4c85ff98dbbfd3  
**Status**: Declared stable-20251230  

---

## Production Recommendations

### Safe Configuration (Recommended)

```bash
./solver_dev 1005  # 8M/8M/8M with 10% safety margin
```

**Characteristics**:
- Peak: 748 MB
- Limit: 1005 MB
- Buffer: 257 MB (34%)
- **Best balance** of performance and safety

### Alternative Margins

| Margin | Limit | Use Case |
|--------|-------|----------|
| +2% (More Aggressive) | 823 MB | Exact binary match, known platform |
| +5% (Aggressive) | 861 MB | Controlled environments |
| **+10% (Safe)** | **1005 MB** | **Production deployments** ✅ |
| +15% (Conservative) | 1092 MB | Critical systems, unknowns |

---

## Related Documentation

### In Parent Directory
- **[../SOLVER_IMPLEMENTATION.md](../SOLVER_IMPLEMENTATION.md)** - Code implementation
- **[../MEMORY_MONITORING.md](../MEMORY_MONITORING.md)** - Monitoring techniques
- **[../EXPERIMENT_SCRIPTS.md](../EXPERIMENT_SCRIPTS.md)** - Testing scripts
- **[../WASM_BUILD_GUIDE.md](../WASM_BUILD_GUIDE.md)** - WASM compilation

### In Root docs/
- **[../../README.md](../../README.md)** - Developer entry point
- **[../../USER_GUIDE.md](../../USER_GUIDE.md)** - End-user guide
- **[../../MEMORY_CONFIGURATION_GUIDE.md](../../MEMORY_CONFIGURATION_GUIDE.md)** - Production deployment

---

## Data Files

Original measurement data archived locally in:
```
RubiksSolverDemo/src/xxcrossTrainer/backups/logs/
└─ tmp_log_run_measurements_20251230_1715/
   ├─ measurement_results.csv (47 test summaries)
   ├─ measurement_log_20251230.txt (502 lines)
   ├─ solver_*MB.log (47 files)
   ├─ monitor_*MB.log (47 files)
   └─ rss_*MB.csv (47 files, ~14K samples each)
```

**Note**: Log files are excluded from git repository (see .gitignore). They remain available locally for reference but are not pushed to remote.

---

**Document Purpose**: This README serves as a navigation guide to all experimental findings. Use it to understand the relationships between documents and find the information you need quickly.

**Status**: Archive complete, all findings documented.
