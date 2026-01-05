# XXCross Solver - User Guide

**Version**: stable_20260103  
**Last Updated**: 2026-01-03

> **Navigation**: User Guide (You are here) | [Developer Documentation](README.md)

---

## Overview

The XXCross Solver is a high-performance Rubik's Cube F2L (First Two Layers) solver that finds optimal solutions for XXCross (double cross) configurations. This guide helps you choose the right memory configuration and use the solver effectively.

---

## Quick Start

### Web Browser (WASM - Recommended)

For most users, the WASM version running in a web browser is the easiest option:

1. Open `xxcross_trainer.html` in your web browser
2. Wait for solver initialization (~10-12 seconds)
3. Start solving!

**Production Configuration**:
- **Default Model**: MOBILE_LOW (automatically selected)
- **Memory Usage**: ~600MB (dual-solver for adjacent/opposite pairs)
- **Initialization**: One-time ~10-12s per session
- **Scramble Generation**: ~12-14ms for depth 10 (imperceptible latency)

**Note**: The trainer uses MOBILE_LOW configuration by default. This provides excellent performance while ensuring broad device compatibility. No configuration selection is needed.

### Command Line (Native C++)

```bash
# Download or compile the solver
g++ -std=c++17 -O3 -march=native solver_dev.cpp -o solver_dev

# Run with custom bucket configuration (developer mode)
export ENABLE_CUSTOM_BUCKETS=1
export BUCKET_D7=8 BUCKET_D8=8 BUCKET_D9=8 BUCKET_D10=8
./solver_dev
```

The solver will:
1. Build the search database (~165-180 seconds, 5 phases up to depth 10)
2. Wait for input or provide a scramble
3. Find the optimal XXCross solution

**Note**: For custom bucket sizes and native builds, see [Developer Documentation](README.md).

---

## Memory Configuration Guide

### How WASM Tiers Work

The solver uses **pre-configured WASM memory tiers** with specific bucket sizes for each tier. Each tier is optimized for different device capabilities:

- **Bucket sizes** (d7/d8/d9/d10) determine memory usage and node capacity
- **Dual-heap values** account for two solvers (adjacent + opposite F2L slots)
- Choose a tier based on your **target device's available memory**

### WASM Memory Tiers (Recommended)

| Tier | Dual Heap | Bucket Config (d7/d8/d9/d10) | Total Nodes | Recommended For |
|------|-----------|------------------------------|-------------|------------------|
| **Mobile LOW** | **618 MB** | 1M/1M/2M/4M | 12.1M | Budget phones (2GB RAM) |
| **Mobile MIDDLE** | **896 MB** | 2M/4M/4M/4M | 17.8M | Mid-range phones (4GB RAM) |
| **Mobile HIGH** | **1070 MB** | 4M/4M/4M/4M | 19.7M | Flagship phones (6GB+ RAM) |
| **Desktop STD** | **1512 MB** | 8M/8M/8M/8M | 34.7M | ✅ **Standard desktops** |
| **Desktop HIGH** | **2786 MB** | 8M/16M/16M/16M | 57.4M | High-memory PC (16GB+ RAM) |
| **Desktop ULTRA** | **2884 MB** | 16M/16M/16M/16M | 65.0M | Max performance (4GB browser limit) |

**Note**: Dual heap values account for two solvers (adjacent + opposite F2L slots)

---

## Deployment Examples

### Production Deployment (Current)

**xxcross_trainer.html** is production-ready and deployed:

```bash
# Production files (deploy to web server)
cp production/solver_prod.{js,wasm} /var/www/html/trainer/
cp production/worker_prod.js /var/www/html/trainer/
cp xxcross_trainer.html /var/www/html/trainer/
```

**Configuration**:
- **Default**: MOBILE_LOW (hardcoded in xxcross_trainer.html)
- **Memory**: ~600MB dual-heap (adjacent + opposite solvers)
- **Build**: -O3 -msimd128 -flto (95KB JS, 302KB WASM)
- **Browser Tested**: Desktop and mobile verified

**Build from source** (if needed):
```bash
cd src/xxcrossTrainer/production
./build_production.sh  # Creates solver_prod.{js,wasm}
```

**CDN deployment**:
```html
<!-- In your HTML -->
<script src="https://cdn.example.com/solver_prod.js"></script>
```

### Docker (for development/testing)

```dockerfile
FROM nginx:alpine

# Copy WASM files and trainer HTML
COPY solver_heap_measurement.* /usr/share/nginx/html/
COPY *_trainer.html /usr/share/nginx/html/

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
```

**Build and run**:
```bash
docker build -t xxcross-trainer .
docker run -p 8080:80 xxcross-trainer
# Access at http://localhost:8080
```

For advanced deployment and build instructions, see [Developer Documentation](README.md).

---

## Understanding Memory Behavior

> **For deeper technical understanding**, see [Developer Documentation](README.md):
> - [Implementation Details](README.md#developer-documentation-developer) - How bucket allocation works
> - [Memory Theory Analysis](README.md#experimental-findings-developerexperiences) - Why memory behaves this way

### What Determines Memory Usage?

Memory usage depends **entirely** on the bucket configuration (not a memory limit):

```
Bucket Config (d7/d8/d9/d10) → Memory Usage
    1M/1M/2M/4M             →   309 MB (single solver)
    8M/8M/8M/8M             →   756 MB (single solver)
   16M/16M/16M/16M          →  1442 MB (single solver)
```

**Key Insight**: WASM tiers explicitly specify bucket sizes, and memory usage is measured empirically for each configuration. There is no "automatic selection" - you choose the tier, and the solver uses those exact bucket sizes.

### Memory Composition

For 8M/8M/8M/8M configuration (Desktop STD tier):

```
WASM Heap: 756 MB (measured)
├─ Fixed Overhead: ~70 MB
│  ├─ WASM runtime: ~20 MB
│  ├─ JavaScript engine: ~15 MB
│  ├─ Phase 1-4 data: ~25 MB
│  └─ Misc: ~10 MB
└─ Hash Tables (d7-d10): ~686 MB
   ├─ Depth 7 bucket: 8M slots
   ├─ Depth 8 bucket: 8M slots
   ├─ Depth 9 bucket: 8M slots
   └─ Depth 10 bucket: 8M slots (Phase 5 random sampling)
```

**Total nodes**: 34.7M across all depths (0-10)

### Memory Stability

The solver exhibits **extremely stable** memory usage:
- **Variation**: <0.5 MB within same configuration
- **Coefficient of Variation**: <0.03%
- **No transient spikes** for large buckets (8M+)

Based on 660,000+ measurements across 47 test points.

---

## Performance Characteristics

**Based on comprehensive testing** (January 2026, 6,000 trials across 12 configurations)

### Database Construction Time (Initialization)

Production WASM module initialization times (measured in browser):

| Tier | Adjacent | Opposite | Average | Notes |
|------|----------|----------|---------|-------|
| **Mobile LOW** | 12.0s | 10.2s | **11.1s** | Best for quick startup |
| **Mobile MIDDLE** | 18.2s | 16.6s | **17.4s** | Balanced mobile option |
| **Mobile HIGH** | 19.2s | 18.7s | **19.0s** | Premium mobile experience |
| **Desktop STD** | 33.5s | 36.8s | **35.2s** | ✅ Recommended standard |
| **Desktop HIGH** | 73.4s | 75.0s | **74.2s** | High-memory systems |
| **Desktop ULTRA** | 81.7s | 96.1s | **88.9s** | Maximum quality |

**Key Insight**: Initialization is a one-time cost. Users wait once per session, then enjoy fast scramble generation.

### Scramble Generation Performance

**Depth 10 scrambles** (most demanding, 100-trial average):

| Tier | Adjacent | Opposite | Average | Best For |
|------|----------|----------|---------|----------|
| **Mobile LOW** | 11.7ms | 14.2ms | **12.9ms** | 💡 Budget phones |
| **Mobile MIDDLE** | 11.9ms | 13.8ms | **12.9ms** | Mid-range phones |
| **Mobile HIGH** | 10.5ms | 16.2ms | **13.3ms** | Flagship phones |
| **Desktop STD** | 11.6ms | 12.5ms | **12.1ms** | ⭐ Best balance |
| **Desktop HIGH** | 14.6ms | 13.4ms | **14.0ms** | High-spec desktops |
| **Desktop ULTRA** | 12.8ms | 18.0ms | **15.4ms** | Maximum node count |

**All configurations meet production requirements (<20ms)**

### Depth Guarantee Algorithm Performance

Each scramble is guaranteed to have exact optimal depth through retry logic:

| Tier | Avg Retries | Generation Time | Notes |
|------|-------------|-----------------|-------|
| **Mobile LOW** | 11.4x | 12.9ms | Sparse coverage requires more retries |
| **Mobile MIDDLE** | 7.7x | 12.9ms | ⭐ Most efficient retry ratio |
| **Mobile HIGH** | 11.3x | 13.3ms | Similar to LOW despite larger buckets |
| **Desktop STD** | 11.2x | 12.1ms | Best overall performance |
| **Desktop HIGH** | 8.4x | 14.0ms | Improved coverage reduces retries |
| **Desktop ULTRA** | 11.8x | 15.4ms | Opposite configuration less efficient |

**Insight**: More nodes ≠ always fewer retries. Database structure and randomness affect retry behavior.

---

## Scramble Generation Quality

### Guaranteed Exact Depth (2026-01-04)

The solver uses a **depth guarantee algorithm** to ensure scrambles have exact optimal solution at requested depth:

**How it works**:
1. Random node selected from `index_pairs[depth]`
2. Verify actual optimal depth by testing depths 1→target
3. Accept only if actual depth matches requested depth
4. Retry with different node if mismatch (up to 100 attempts)

**Why this is necessary**:
- Database stores nodes **discovered** at depth N, not necessarily with **optimal solution** at depth N
- Sparse coverage at depths 7+ (0.008-0.16% of state space)
- Example: Depth 10 has ~50B nodes, we store ~4M (0.008%)
- High probability random node has shorter optimal solution

**Production Performance** (Comprehensive testing, Jan 2026):
- **Depth 6-8**: <1ms average (<2 retries typically)
- **Depth 9**: ~2ms average (3-5 retries)
- **Depth 10**: 12-15ms average (8-12 retries, depending on model)
- **100% success rate** across 6,000 trials (all models, all depths)

**Result**: Every scramble is guaranteed to have optimal solution at exact requested depth, even on minimal memory configuration (Mobile LOW: 1M/1M/2M/4M).

### Model Selection Guidelines

**For mobile devices (phones/tablets)**:
- **Budget devices (2-3GB RAM)**: Mobile LOW (11s init, 13ms depth 10)
- **Mid-range devices (4-6GB RAM)**: Mobile MIDDLE (17s init, 13ms depth 10) ⭐ Best mobile balance
- **Flagship devices (8GB+ RAM)**: Mobile HIGH (19s init, 13ms depth 10)

**For desktop/laptop browsers**:
- **Standard use**: Desktop STD (35s init, 12ms depth 10) ✅ Recommended for most users
- **High-memory systems**: Desktop HIGH (74s init, 14ms depth 10)
- **Maximum quality**: Desktop ULTRA (89s init, 15ms depth 10)

**Key tradeoffs**:
- **Initialization time**: One-time cost per session (users typically solve 10-50+ scrambles)
- **Generation speed**: All models <20ms (imperceptible to users)
- **Memory usage**: Choose based on target device RAM availability

---

## Common Use Cases

### Web Application (WASM)

**Recommended**: Use pre-configured WASM tiers

```javascript
// Tier selection based on device
const tier = detectDeviceTier();  // Returns 'MOBILE_LOW', 'DESKTOP_STD', etc.

// Load solver with tier configuration
const solver = await loadSolverWithTier(tier);
```

**Tier selection guidelines**:
- **Mobile phones (2-4GB RAM)**: Mobile LOW or MIDDLE
- **Desktop browsers**: Desktop STD (best balance)
- **High-memory systems**: Desktop HIGH or ULTRA

**For WASM deployment**, see [Developer Documentation](README.md) for build instructions.

### Mobile Application

Recommended: **Mobile MIDDLE tier** (2M/4M/4M/4M, 448 MB single / 896 MB dual)
- Safe for modern phones (4+ GB RAM)
- Good balance of memory and node coverage

### Desktop Application

Recommended: **Desktop STD tier** (8M/8M/8M/8M, 756 MB single / 1512 MB dual)
- Optimal for desktops with 4+ GB RAM
- 34.7M total nodes, excellent depth 10 coverage

### Server / Cloud

Recommended: **Desktop HIGH or ULTRA tier**
- HIGH: 8M/16M/16M/16M (1393 MB single / 2786 MB dual)
- ULTRA: 16M/16M/16M/16M (1442 MB single / 2884 MB dual)
- Best node coverage, ideal for server environments

---

## Troubleshooting

### Problem: Browser/WASM runs out of memory

**Symptoms**: Browser tab crashes or "out of memory" error

**Solution**: Switch to a smaller tier:
1. Try **Mobile LOW** (618 MB) - works on most devices
2. Close other browser tabs to free memory
3. Use a 64-bit browser if available

---

### Problem: Slow database construction

**Symptoms**: Construction takes >200 seconds

**Diagnosis**:
1. Check CPU performance (requires modern processor)
2. Verify browser is using hardware acceleration
3. Check if other processes are consuming CPU

**Solution**:
- Close unnecessary applications
- Try Desktop STD tier (best balance of speed and memory)
- For native builds: Compile with `-O3 -march=native`

---

### Problem: Different memory usage than documented

**Symptoms**: Browser shows different heap size

**Diagnosis**: Browser overhead, extensions, or platform differences

**Solution**: This is normal! Variations of ±50 MB are expected:
- Different browsers have different JS engine overhead
- Browser extensions consume additional memory
- Use documented tier as guideline, adjust if needed

---

## Monitoring Memory Usage

### Browser (WASM)

Most modern browsers provide memory profiling tools:

**Chrome/Edge**:
1. Open DevTools (F12)
2. Go to "Performance" or "Memory" tab
3. Take heap snapshot or record performance profile

**Firefox**:
1. Open DevTools (F12)
2. Go to "Memory" tab
3. Take snapshot

### Native C++ (Linux)

For developers working with native builds:

```bash
# Real-time monitoring
watch -n 1 'grep VmRSS /proc/$(pidof solver_dev)/status'
```

For detailed monitoring techniques, see [Developer Documentation](README.md).

---

## Frequently Asked Questions

### Q: Why does the solver need so much memory?

**A**: The solver builds a complete search database of **all reachable XXCross states** (up to depth 10), storing tens of millions of nodes using a 5-Phase BFS construction. This enables:
- Optimal solution finding
- Fast lookups during search
- Scramble generation for training

### Q: Which WASM tier should I choose?

**A**: Recommended tiers by device:
- **Mobile phones (2-4GB RAM)**: Mobile LOW or MIDDLE (618-896 MB)
- **High-end phones (6GB+ RAM)**: Mobile HIGH (1070 MB)
- **Desktop/Laptop**: Desktop STD (1512 MB) ✅ Best balance
- **High-memory systems**: Desktop HIGH or ULTRA (2786-2884 MB)

### Q: Does higher memory improve performance?

**A**: No. Performance depends on bucket sizes, not memory limits. Desktop STD (8M/8M/8M/8M) is already very fast and recommended for most users.

### Q: Can I use this in a web browser?

**A**: Yes! The solver compiles to WebAssembly. Use the appropriate WASM tier for your target devices:
- Mobile browsers: Use Mobile tiers (LOW/MIDDLE/HIGH)
- Desktop browsers: Use Desktop STD (1512 MB dual-heap)
- See [Developer Documentation](README.md) for integration guide

### Q: What if my browser runs out of memory?

**A**: Switch to a smaller tier:
- Mobile LOW (618 MB) works on most devices
- Each tier is carefully measured for stability
- Contact issues if problems persist with Mobile LOW

---

## How It Works (Brief)

For users who want to understand the basics:

### 5-Phase Construction

The solver builds the search database in 5 phases:

1. **Phase 1** (Depth 0-6): Full BFS expansion of all reachable states
2. **Phase 2** (Depth 7): Local BFS expansion using hash table
3. **Phase 3** (Depth 8): Continued local expansion
4. **Phase 4** (Depth 9): Further expansion
5. **Phase 5** (Depth 10): Random sampling for coverage

### WASM Memory Tiers

Each tier specifies hash table sizes for depths 7, 8, 9, 10:
- **Mobile LOW**: 1M/1M/2M/4M slots (smallest, most compatible)
- **Desktop STD**: 8M/8M/8M/8M slots (recommended)
- **Desktop ULTRA**: 16M/16M/16M/16M slots (maximum performance)

Larger hash tables = more nodes stored = better solution coverage.

### Why Multiple Tiers?

Different devices have different memory limits:
- Mobile browsers: typically 512 MB - 2 GB available
- Desktop browsers: up to 4 GB (browser limit)
- Each tier is optimized for specific hardware constraints

---

## Getting Help

### For Usage Questions

1. Check this guide's troubleshooting section
2. Review the WASM Memory Tiers table for tier selection
3. Try a smaller tier if you encounter memory issues

### For Development/Technical Questions

See [Developer Documentation](README.md) for:
- Advanced configuration and custom bucket sizes
- WASM build and integration guides
- Memory measurement methodology
- Implementation details and API reference

---

## Version History

### stable_20260103 (Current)

- ✅ Depth 10 support via random sampling (Phase 5)
- ✅ Memory spike elimination (pre-reserve pattern)
- ✅ 6-tier WASM bucket model (618-2884 MB dual-solver)
- ✅ Complete WASM heap measurement (13 configurations)
- ✅ Comprehensive API and implementation documentation
- ✅ Malloc trim management (ENV configurable)

### stable-20251230 (Previous)

- ✅ Empirically validated memory configurations (47-point measurement campaign)
- ✅ Documented all 7 bucket configurations with safety margins
- ✅ Comprehensive memory behavior analysis
- ✅ Production-ready with <0.03% memory variation

### Previous Versions

For detailed version history, see [Developer Documentation](README.md).

---

## Summary

**Recommended Configuration**:
- **Web/WASM users**: Desktop STD tier (1512 MB dual-heap)
- **Native C++ users**: Custom bucket sizes via environment variables
- **Mobile users**: Mobile LOW/MIDDLE tier (618-896 MB)

**Key Points**:
- Choose WASM tier based on target device memory
- All tiers provide optimal solutions
- Larger tiers = better depth 10 coverage
- See [Developer Documentation](README.md) for advanced usage

**Happy Solving!** 🎲
