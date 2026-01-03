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

1. Open a trainer page (e.g., `xcross_trainer.html`)
2. Select memory tier based on your device:
   - **Mobile phones**: Mobile LOW/MIDDLE (618-896 MB)
   - **Desktop/Laptop**: Desktop STD (1512 MB) ✅ Recommended
3. Start solving!

**WASM tiers** are pre-configured and automatically manage memory.

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

### Web Hosting (WASM)

**Recommended deployment method** for most users:

```bash
# Build production WASM module
source ~/emsdk/emsdk_env.sh
em++ solver_dev.cpp -o solver_dev.js -std=c++17 -O3 -I../.. \
  -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MAXIMUM_MEMORY=4GB \
  -s EXPORTED_RUNTIME_METHODS='["cwrap"]' -s MODULARIZE=1 \
  -s EXPORT_NAME="createModule" --bind -s INITIAL_MEMORY=64MB

# Deploy to web server
cp solver_dev.{js,wasm} /var/www/html/trainer/
cp test_wasm_browser.html /var/www/html/trainer/
cp worker_test_wasm_browser.js /var/www/html/trainer/

# Note: build_wasm_unified.sh builds solver_heap_measurement.{js,wasm}
# for legacy heap measurement workflows (archived in backups/)
```

**CDN deployment**:
```html
<!-- In your HTML -->
<script src="https://cdn.example.com/solver_dev.js"></script>
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

### Database Construction Time

Typical construction time (on modern CPU):
- **Phase 1** (BFS depth 0-6): ~30 seconds
- **Phase 2** (Depth 7 expansion): ~10 seconds
- **Phase 3** (Depth 8 expansion): ~40 seconds
- **Phase 4** (Depth 9 expansion): ~70 seconds
- **Phase 5** (Depth 10 random sampling): ~15-20 seconds
- **Total**: ~165-180 seconds

### Memory vs Performance

| Tier | Bucket Config | WASM Heap | Total Nodes | Construction Time |
|------|---------------|-----------|-------------|--------------------|
| Mobile LOW | 1M/1M/2M/4M | 309 MB | 12.1M | ~120s |
| Desktop STD | 8M/8M/8M/8M | 756 MB | 34.7M | ~165s |
| Desktop ULTRA | 16M/16M/16M/16M | 1442 MB | 65.0M | ~180s |

**Insight**: Larger buckets store more nodes (better depth 10 coverage) but construction time increases slightly.

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
