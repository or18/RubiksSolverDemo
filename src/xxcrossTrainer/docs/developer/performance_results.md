# XXCross Trainer - Performance Results

**Last Updated**: 2026-01-04  
**Build**: Production (O3, SIMD128, LTO)  
**Test Scale**: 6,000 trials (12 configurations × 5 depths × 100 trials)

---

## Executive Summary

Performance testing for all 12 configurations (6 models × Adjacent/Opposite) has been completed. All models achieved an average generation time of under 20ms at depth 10, making them suitable for use as practical training tools.

**Key Achievements**:
- ✅ **100% depth guarantee accuracy** - All 6,000 trials generated exact depth scrambles
- ✅ **<20ms mean depth 10 generation** - All models meet production requirements
- ✅ **10-96s initialization** - Scales predictably with database size
- ✅ **Production ready** - Comprehensive testing validates WASM module stability

**Testing Methodology**: 
- 100 trials per depth (depths 6-10) for statistical significance
- Browser verification in addition to Node.js benchmarking
- Retry behavior analysis (depth guarantee algorithm performance)

**Comparison with Initial 5-Trial Test**:
The initial 5-trial test (2026-01-04 04:05 JST) provided preliminary data but lacked statistical confidence. This comprehensive 100-trial test confirms production readiness with high confidence intervals.

### Quick Reference

| Model | Init Time | Depth 10 Avg | Memory | Recommendation |
|-------|-----------|--------------|--------|----------------|
| **MOBILE_LOW** | 10-12s | 12-14ms | ~600MB | 💡 Mobile Recommended |
| **MOBILE_MIDDLE** | 17-18s | 12-14ms | ~800MB | Mobile High Performance |
| **MOBILE_HIGH** | 19s | 10-16ms | ~1GB | Mobile Highest Quality |
| **DESKTOP_STD** | 34-37s | 12-13ms | ~1.2GB | 🖥️ Desktop Recommended |
| **DESKTOP_HIGH** | 73-75s | 14-15ms | ~2GB | Desktop High Quality |
| **DESKTOP_ULTRA** | 82-96s | 13-18ms | ~2.9GB | Highest Quality (High Spec) |

---

## Initialization Performance

### Adjacent Pairs

```
MOBILE_LOW:      ████████████                         12.0s
MOBILE_MIDDLE:   ██████████████████                   18.2s
MOBILE_HIGH:     ███████████████████                  19.2s
DESKTOP_STD:     █████████████████████████████████    33.5s
DESKTOP_HIGH:    ████████████████████████████████████████████████████████████████████ 73.4s
DESKTOP_ULTRA:   ███████████████████████████████████████████████████████████████████████████ 81.7s
```

### Opposite Pairs

```
MOBILE_LOW:      ██████████                           10.2s
MOBILE_MIDDLE:   ████████████████                     16.6s
MOBILE_HIGH:     ██████████████████                   18.7s
DESKTOP_STD:     ████████████████████████████████████ 36.8s
DESKTOP_HIGH:    ███████████████████████████████████████████████████████████████████████ 75.0s
DESKTOP_ULTRA:   ████████████████████████████████████████████████████████████████████████████████████ 96.1s
```

**Key Insight**: Opposite pairs generally initialize faster than Adjacent (except DESKTOP_ULTRA)

---

## Depth 10 Generation Performance

Depth 10 is the most important practical metric (generation time increases exponentially beyond depth 11).

### Mean Generation Time (100 trials)

| Model | Adjacent | Opposite | Combined Avg |
|-------|----------|----------|--------------|
| MOBILE_LOW | 11.66ms | 14.19ms | **12.93ms** |
| MOBILE_MIDDLE | 11.87ms | 13.82ms | **12.85ms** |
| MOBILE_HIGH | 10.46ms | 16.20ms | **13.33ms** |
| DESKTOP_STD | 11.60ms | 12.54ms | **12.07ms** ⭐ |
| DESKTOP_HIGH | 14.55ms | 13.36ms | **13.96ms** |
| DESKTOP_ULTRA | 12.76ms | 18.04ms | **15.40ms** |

⭐ **Best overall**: DESKTOP_STD (12.07ms average, 35s initialization)

### Retry Behavior

The depth guarantee algorithm operates probabilistically, so multiple attempts are made until the requested depth is reached.

| Model | Adjacent Retries | Opposite Retries | Avg Retries |
|-------|------------------|------------------|-------------|
| MOBILE_LOW | 12.34x | 10.48x | 11.41x |
| MOBILE_MIDDLE | 5.62x | 9.86x | 7.74x ⭐ |
| MOBILE_HIGH | 9.90x | 12.60x | 11.25x |
| DESKTOP_STD | 13.41x | 8.97x | 11.19x |
| DESKTOP_HIGH | 10.69x | 6.04x | 8.37x |
| DESKTOP_ULTRA | 7.85x | 15.65x | 11.75x |

⭐ **Most efficient**: MOBILE_MIDDLE (7.74x average retries)

---

## Performance by Depth

### Adjacent Pairs - Mean Generation Time

| Depth | MOBILE_LOW | MOBILE_MIDDLE | MOBILE_HIGH | DESKTOP_STD | DESKTOP_HIGH | DESKTOP_ULTRA |
|-------|------------|---------------|-------------|-------------|--------------|---------------|
| **6** | 0.34ms | 0.18ms | 0.19ms | 0.19ms | 0.20ms | 0.62ms |
| **7** | 0.31ms | 0.32ms | 0.21ms | 0.16ms | 0.17ms | 0.33ms |
| **8** | 0.51ms | 0.55ms | 0.33ms | 0.34ms | 0.39ms | 0.49ms |
| **9** | 1.76ms | 2.15ms | 2.01ms | 1.55ms | 1.65ms | 2.23ms |
| **10** | 11.66ms | 11.87ms | 10.46ms | 11.60ms | 14.55ms | 12.76ms |

### Opposite Pairs - Mean Generation Time

| Depth | MOBILE_LOW | MOBILE_MIDDLE | MOBILE_HIGH | DESKTOP_STD | DESKTOP_HIGH | DESKTOP_ULTRA |
|-------|------------|---------------|-------------|-------------|--------------|---------------|
| **6** | 0.17ms | 0.25ms | 0.16ms | 0.18ms | 0.15ms | 0.47ms |
| **7** | 0.16ms | 0.20ms | 0.16ms | 0.16ms | 0.16ms | 0.34ms |
| **8** | 0.37ms | 0.43ms | 0.35ms | 0.33ms | 0.33ms | 0.52ms |
| **9** | 2.03ms | 2.94ms | 2.68ms | 1.43ms | 1.69ms | 1.98ms |
| **10** | 14.19ms | 13.82ms | 16.20ms | 12.54ms | 13.36ms | 18.04ms |

**Trend**: Generation time increases approximately 2-4 times with each depth increment (exponential growth)

---

## Device Recommendations

### Mobile Devices 📱

**Recommended: MOBILE_LOW**
- ✅ Fastest initialization (10-12s)
- ✅ Low memory (~600MB)
- ✅ Depth 10: 12-14ms (sufficiently fast)
- 💡 Use case: Smartphones, tablets

**Alternatives: MOBILE_MIDDLE / MOBILE_HIGH**
- Improved quality with larger databases
- Initialization 18-19s (acceptable)
- Memory ~1GB

### Desktop (Standard) 🖥️

**Recommended: DESKTOP_STD**
- ⭐ Best overall balance
- ✅ Initialization 35s (acceptable)
- ✅ Depth 10: **12ms average** (best among all models)
- ✅ Memory ~1.2GB
- 💡 Use case: General PCs, laptops

### Desktop (High Performance) 💻

**Recommended: DESKTOP_HIGH**
- ✅ High-quality database
- ⚠️ Initialization 73-75s (somewhat long)
- ✅ Depth 10: 13-15ms
- ⚠️ Memory ~2GB
- 💡 Use case: High-end PCs, long training sessions

**Highest Quality: DESKTOP_ULTRA**
- 🏆 Largest database
- ⚠️ Initialization 82-96s (long)
- ✅ Depth 10: 13-18ms
- ⚠️ Memory ~2.9GB (1.5GB heap usage)
- 💡 Use case: Users seeking highest quality

---

## Production Deployment Considerations

### 1. Initialization Time UX Improvements

```javascript
// Recommended: Show initialization progress
const estimatedTime = {
  'MOBILE_LOW': '10-12 seconds',
  'MOBILE_MIDDLE': '17-18 seconds',
  'MOBILE_HIGH': '19 seconds',
  'DESKTOP_STD': '35 seconds',
  'DESKTOP_HIGH': '75 seconds',
  'DESKTOP_ULTRA': '95 seconds'
};

showProgress(`Database building in progress... Estimated time: ${estimatedTime[model]}`);
```

### 2. Device Auto-Detection

```javascript
// Memory-based auto-selection
const totalMemory = navigator.deviceMemory || 4; // GB

let recommendedModel;
if (totalMemory <= 2) {
  recommendedModel = 'MOBILE_LOW';    // 2GB or less
} else if (totalMemory <= 4) {
  recommendedModel = 'MOBILE_HIGH';   // 4GB or less
} else if (totalMemory <= 8) {
  recommendedModel = 'DESKTOP_STD';   // 8GB or less
} else {
  recommendedModel = 'DESKTOP_HIGH';  // More than 8GB
}
```

### 3. Worker Initialization Pattern

```javascript
// Lazy initialization (first use only)
let workerInitialized = false;

async function initializeWorker(bucketModel) {
  if (workerInitialized) return;
  
  worker.postMessage({
    type: 'initSolver',
    adj: true,  // or false
    bucketModel: bucketModel
  });
  
  await waitForInit();
  workerInitialized = true;
}
```

### 4. Memory Management

- **DESKTOP_ULTRA**: Up to 1.5GB heap usage (safe within 2GB limit)
- **Recommended max memory setting**: `-sMAXIMUM_MEMORY=2048MB`
- **Initial memory**: `-sINITIAL_MEMORY=512MB` (gradually expanded)
---

## Testing Methodology

### Test Configuration

- **Trials**: 100 trials per depth (statistical reliability)
- **Depths**: 6, 7, 8, 9, 10 (practical range)
- **Statistics**: Mean, median, std dev, min/max, retry count
- **Total measurements**: 6,000 trials

### Why Depth 10?

- **Depth 6-8**: Fast but too easy (beginner level)
- **Depth 9-10**: Practical difficulty (intermediate level)
- **Depth 11+**: Exponentially slower (advanced level, not recommended)

Depth 10 is the most important indicator as the **upper limit of user experience**.
### Browser Verification

Manually verified all 12 configurations in the browser (`test_worker.html`):
- ✅ Worker initialization
- ✅ Scramble generation
- ✅ Worker termination and restart
- ✅ Error handling
---

## Known Limitations

### 1. Sequential Testing Issue

**Issue**: Memory shortage on the 12th configuration (DESKTOP_ULTRA Opposite) when running 12 configurations sequentially in Node.js environment

**Cause**: WASM heap is not fully released (cumulative memory usage)

**Workaround**: Run in separate processes (successful)
**Impact**: ❌ Node.js sequential testing environment only  
　　　　✅ Browser environment (Worker) no issues

### 2. Debug Build Memory Overhead

**Issue**: Debug build (`-O2 -sASSERTIONS=1`) runs out of memory on DESKTOP_ULTRA

**Cause**: Memory overhead of debug symbols and assertions

**Workaround**: Use release build (`-O3`)

**Impact**: Limited debugging features during development (no issues in release)
---

## Conclusion

### ✅ Production Ready

All 12 configurations meet the performance requirements and are ready for production deployment.

### Recommended Default Settings

- **Mobile**: MOBILE_LOW (balanced) performance/quality
- **Desktop**: DESKTOP_STD (best performance/time ratio)
- **High-Performance PC**: DESKTOP_HIGH (quality focused)

### Next Steps

1. Integrate into xxcross_trainer.html UI
2. Implement model selection interface
3. Add initialization progress display
4. Create user guide

---

## Detailed Reports

**Complete Statistics**: [production/_archive/docs/performance_results_detailed.md](../../production/_archive/docs/performance_results_detailed.md)

**Implementation Details**: [production/_archive/docs/Production_Implementation.md](../../production/_archive/docs/Production_Implementation.md)

**Raw JSON**: [production/_archive/results/performance_results_complete.json](../../production/_archive/results/performance_results_complete.json)
