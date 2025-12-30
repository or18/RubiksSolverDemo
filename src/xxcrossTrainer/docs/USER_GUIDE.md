# XXCross Solver - User Guide

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: User Guide (You are here) | [Memory Configuration](MEMORY_CONFIGURATION_GUIDE.md) | [Developer Documentation](README.md)

---

## Overview

The XXCross Solver is a high-performance Rubik's Cube F2L (First Two Layers) solver that finds optimal solutions for XXCross (double cross) configurations. This guide helps you choose the right memory configuration and use the solver effectively.

---

## Quick Start

### Basic Usage (Command Line)

```bash
# Download or compile the solver
g++ -std=c++17 -O3 -march=native solver_dev.cpp -o solver_dev

# Run with recommended memory limit (1005 MB)
./solver_dev 1005
```

The solver will:
1. Build the search database (~150 seconds)
2. Wait for input or provide a scramble
3. Find the optimal XXCross solution

### Recommended Configuration

**For most users**: `1005 MB` memory limit
- Configuration: 8M/8M/8M buckets
- Peak memory: ~748 MB
- Safety margin: +257 MB (34%)
- **Best balance** of speed and memory efficiency

---

## Memory Configuration Guide

### How Memory Limits Work

The solver **automatically selects** the best bucket configuration based on your specified memory limit. Higher limits allow larger hash tables, but memory usage is **deterministic** regardless of the limit value within the same configuration range.

### Configuration Table

| Memory Limit | Bucket Config | Actual Peak | Buffer | Recommended For |
|--------------|---------------|-------------|--------|-----------------|
| 300-568 MB | 2M/2M/2M | 293 MB | 7-275 MB | **Embedded systems** |
| 569-612 MB | 2M/4M/2M | 404 MB | 165-208 MB | Mobile devices |
| 613-920 MB | 4M/4M/4M | 434 MB | 179-486 MB | Standard servers |
| 921-999 MB | 4M/8M/4M | 655 MB | 266-344 MB | - |
| **1000-1536 MB** | **8M/8M/8M** | **748 MB** | **252-788 MB** | **Production (recommended)** ✅ |
| 1537-1701 MB | 8M/16M/8M | 1189 MB | 348-512 MB | High-memory systems |
| 1702+ MB | 16M/16M/16M | 1375 MB | 327+ MB | Performance-critical |

### Choosing Your Configuration

**Conservative (Recommended)**: Use `1005 MB` or `1513 MB` limits
- +10% or +15% safety margin
- Protects against platform variations
- Ideal for production deployments

**Aggressive**: Use `748 MB` or `823 MB` limits
- +0% or +5% safety margin
- Requires exact binary match and platform
- Best for controlled environments

**More Aggressive**: Exact peak values (e.g., `748 MB`)
- No safety margin
- Only for testing/research
- Risk of OOM on different platforms

---

## Deployment Examples

### Docker

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y g++

# Copy solver
COPY solver_dev.cpp /app/
WORKDIR /app

# Build
RUN g++ -std=c++17 -O3 -march=native solver_dev.cpp -o solver_dev

# Run with 1005 MB limit (8M/8M/8M config)
CMD ["./solver_dev", "1005"]
```

**Memory limit in Docker**:
```bash
docker run -m 1200M my-solver  # Container limit > solver limit
```

### Kubernetes

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: xxcross-solver
spec:
  containers:
  - name: solver
    image: xxcross-solver:stable-20251230
    args: ["1005"]  # Solver memory limit
    resources:
      requests:
        memory: "1000Mi"  # Kubernetes request
      limits:
        memory: "1200Mi"  # Kubernetes hard limit (add 200 MB buffer)
```

**Why the extra buffer?**
- Kubernetes measures total pod memory (includes OS, sidecar containers)
- Solver peak is 748 MB, but pod may use ~900-1000 MB total
- 1200 MB limit provides safe headroom

### Systemd Service

```ini
[Unit]
Description=XXCross Solver Service
After=network.target

[Service]
Type=simple
User=solver
WorkingDirectory=/opt/xxcross
ExecStart=/opt/xxcross/solver_dev 1005
Restart=on-failure

# Memory limit (1.2 GB)
MemoryMax=1200M
MemoryHigh=1100M

[Install]
WantedBy=multi-user.target
```

### Command Line (Direct)

```bash
# Standard usage
./solver_dev 1005

# Low-memory system (300 MB available)
./solver_dev 323  # 2M/2M/2M config with 10% margin

# High-performance system (2 GB available)
./solver_dev 1513  # 16M/16M/16M config with 10% margin
```

---

## Understanding Memory Behavior

> **For deeper technical understanding**, see [Developer Documentation](README.md):
> - [Implementation Details](README.md#developer-documentation-developer) - How bucket allocation works
> - [Memory Theory Analysis](README.md#experimental-findings-developerexperiences) - Why memory behaves this way

### What Determines Memory Usage?

Memory usage depends **only** on the bucket configuration, not the specified limit:

```
Memory Limit → Bucket Selection → Fixed Peak Memory
    1005 MB  →    8M/8M/8M      →      748 MB
    1200 MB  →    8M/8M/8M      →      748 MB (same!)
    1600 MB  →    8M/16M/8M     →     1189 MB (different config)
```

**Key Insight**: Within a configuration range (e.g., 1000-1536 MB), increasing the limit does **not** increase memory usage—it only increases the safety margin.

### Memory Composition

For 8M/8M/8M configuration (typical):

```
Total Peak: 748 MB
├─ Fixed Overhead: 62 MB
│  ├─ C++ runtime: ~15 MB
│  ├─ Program text/data: ~10 MB
│  ├─ OS allocator: ~10 MB
│  ├─ Phase 1 remnants: ~20 MB
│  └─ Misc: ~7 MB
└─ Bucket Memory: 686 MB
   ├─ Hash table buckets: 24M slots × 8 bytes = 192 MB
   └─ Node storage: ~20M nodes × ~25 bytes = ~494 MB
```

### Memory Stability

The solver exhibits **extremely stable** memory usage:
- **Variation**: <0.5 MB within same configuration
- **Coefficient of Variation**: <0.03%
- **No transient spikes** for large buckets (8M+)

Based on 660,000+ measurements across 47 test points.

---

## Performance Characteristics

### Solving Time

Typical scramble (on modern CPU):
- **Phase 1** (BFS depth 0-6): ~30 seconds
- **Phase 2** (Partial expansion): ~10 seconds
- **Phase 3** (Local BFS 7→8): ~40 seconds
- **Phase 4** (Local BFS 8→9): ~70 seconds
- **Total**: ~150 seconds

### Memory vs Speed Trade-off

| Configuration | Peak Memory | Relative Speed |
|---------------|-------------|----------------|
| 2M/2M/2M | 293 MB | 1.0× (baseline) |
| 4M/4M/4M | 434 MB | ~1.0× (same) |
| 8M/8M/8M | 748 MB | ~1.0× (same) |
| 16M/16M/16M | 1375 MB | ~1.0× (same) |

**Insight**: Speed is **independent** of bucket size. Choose configuration based purely on available memory.

---

## Common Use Cases

### Web Application (WASM)

```javascript
// Load solver (built with emscripten)
const solver = await createModule();

// Configure for browser (typically 256-512 MB available)
// WASM will use 4M/4M/4M or 8M/8M/8M depending on browser limits
```

See [developer/WASM_BUILD_GUIDE.md](developer/WASM_BUILD_GUIDE.md) for build instructions.

### Mobile Application

Recommended: `434 MB` (4M/4M/4M configuration)
- Safe for most modern phones (2+ GB RAM)
- Limit: `478 MB` (with 10% margin)

```bash
./solver_dev 478
```

### Desktop Application

Recommended: `748 MB` (8M/8M/8M configuration)
- Optimal for desktops with 4+ GB RAM
- Limit: `823 MB` or `1005 MB` (with margin)

```bash
./solver_dev 1005
```

### Server / Cloud

Recommended: `1189 MB` or `1375 MB` (8M/16M/8M or 16M/16M/16M)
- Best for servers with dedicated memory
- Limit: `1308 MB` or `1513 MB` (with margin)

```bash
./solver_dev 1308  # 8M/16M/8M
# or
./solver_dev 1513  # 16M/16M/16M
```

---

## Troubleshooting

### Problem: Solver crashes immediately

**Symptoms**: Process exits with error or no output

**Diagnosis**:
```bash
# Check available memory
free -h

# Check if limit is too low
./solver_dev 500  # Might crash if selects 4M/4M/4M (needs 434 MB)
```

**Solution**: Increase memory limit to at least `323 MB` (minimum for 2M/2M/2M)

---

### Problem: Out of memory during execution

**Symptoms**: "Cannot allocate memory" error during phase 3 or 4

**Diagnosis**: Memory limit is between thresholds (e.g., 570 MB)
```
569 MB → Selects 2M/4M/2M (needs 404 MB) ✓
570 MB → Still 2M/4M/2M ✓
...
612 MB → Still 2M/4M/2M ✓
613 MB → Selects 4M/4M/4M (needs 434 MB) ✓
```

**Solution**: Use recommended limits with safety margins (avoid bare threshold values)

---

### Problem: WASM runs out of memory

**Symptoms**: Browser tab crashes or "out of memory" error

**Diagnosis**: WASM initial memory too low

**Solution**: Build with sufficient initial memory:
```bash
emcc -O3 solver_dev.cpp -o solver.js \
  -s INITIAL_MEMORY=268435456  # 256 MB minimum
  -s ALLOW_MEMORY_GROWTH=1
```

---

### Problem: Performance slower than expected

**Symptoms**: Solving takes >200 seconds

**Diagnosis**:
1. Check CPU (requires modern x86_64 with AVX2)
2. Check compilation flags (`-O3 -march=native`)
3. Verify not running in virtual machine with limited resources

**Solution**:
```bash
# Rebuild with proper flags
g++ -std=c++17 -O3 -march=native solver_dev.cpp -o solver_dev

# Check CPU features
lscpu | grep avx2
```

---

### Problem: Different peak memory than documented

**Symptoms**: Monitoring shows peak of 755 MB instead of 748 MB

**Diagnosis**: Platform/compiler differences
- Different libc version
- Different STL implementation
- Compiler optimizations

**Solution**: This is normal! Variations of ±5-10 MB are expected across platforms. Use safety margins:
- Documented: 748 MB
- Safe limit: 823 MB (+10%)
- Conservative limit: 861 MB (+15%)

---

## Monitoring Memory Usage

### Linux (Command Line)

```bash
# Real-time monitoring
watch -n 1 'grep VmRSS /proc/$(pidof solver_dev)/status'

# Single check
grep VmRSS /proc/$(pidof solver_dev)/status
```

### Programmatic Monitoring (Python)

```python
#!/usr/bin/env python3
import time
import sys

def get_rss_kb(pid):
    """Get RSS in KB from /proc"""
    with open(f'/proc/{pid}/status') as f:
        for line in f:
            if line.startswith('VmRSS:'):
                return int(line.split()[1])
    return None

pid = int(sys.argv[1])
while True:
    rss = get_rss_kb(pid)
    if rss is None:
        print("Process terminated")
        break
    print(f"RSS: {rss / 1024:.1f} MB")
    time.sleep(1)
```

Usage:
```bash
python3 monitor.py $(pidof solver_dev)
```

For detailed monitoring techniques, see [developer/MEMORY_MONITORING.md](developer/MEMORY_MONITORING.md).

---

## Frequently Asked Questions

### Q: Why does the solver need so much memory?

**A**: The solver builds a complete search database of **all reachable XXCross states** (up to depth 9), storing tens of millions of nodes. This enables:
- Optimal solution finding
- Fast lookups during search
- Scramble generation for training

### Q: Can I reduce memory usage?

**A**: Yes, use smaller bucket configurations:
- `323 MB`: 2M/2M/2M (minimum viable)
- `478 MB`: 4M/4M/4M (recommended minimum for production)

Lower configurations have **same speed** but limited safety margin.

### Q: Does higher memory improve performance?

**A**: No. Within a configuration (e.g., 1000-1536 MB all use 8M/8M/8M), speed is identical. Higher limits only increase the safety buffer.

### Q: How accurate are the documented peak values?

**A**: Very accurate (±0.5 MB) for the exact binary tested. Across platforms, expect ±5-10 MB variation. Always use safety margins (+10% recommended).

### Q: What if my system has limited memory?

**A**: The solver works down to 323 MB (2M/2M/2M configuration). For embedded systems or browsers, use the smallest viable configuration.

### Q: Can I run multiple solvers in parallel?

**A**: Yes, each instance uses independent memory. For 4 parallel solvers with 8M/8M/8M:
```
Total memory = 4 × 823 MB = 3292 MB (~3.3 GB with margin)
```

---

## Implementation Details (Brief)

For users who want to understand the basics:

### Two-Phase Algorithm

1. **Phase 1** (Depth 0-6): Full BFS expansion of all reachable states
2. **Phase 2** (Depth 7-9): Local BFS expansion using parent-child relationships

### Bucket System

The solver uses three hash tables (buckets) for depths 7, 8, 9:
- Automatically sized based on memory limit
- Typical sizes: 2M, 4M, 8M, or 16M slots per bucket
- Each bucket stores states at a specific depth

### Why Three Buckets?

Depths 0-6 are fully expanded and stored in a compact format. Depths 7-9 are partially expanded on-demand, requiring hash table lookups for efficient access.

---

## Getting Help

### For Usage Questions

1. Check this guide's troubleshooting section
2. Review [MEMORY_CONFIGURATION_GUIDE.md](MEMORY_CONFIGURATION_GUIDE.md) for detailed configuration info
3. Verify your memory limit matches a recommended value

### For Development/Technical Questions

1. See [README.md](README.md) for developer documentation links
2. Review [developer/SOLVER_IMPLEMENTATION.md](developer/SOLVER_IMPLEMENTATION.md) for code details
3. Check [developer/Experiences/](developer/Experiences/) for experimental findings

---

## Version History

### stable-20251230 (Current)

- ✅ Empirically validated memory configurations (47-point measurement campaign)
- ✅ Documented all 7 bucket configurations with safety margins
- ✅ Comprehensive memory behavior analysis
- ✅ Production-ready with <0.03% memory variation

### Previous Versions

See [developer/Experiences/](developer/Experiences/) for detailed history.

---

## Summary

**Recommended for most users**:
```bash
./solver_dev 1005  # 8M/8M/8M configuration, 748 MB peak, 257 MB buffer
```

**Key Takeaways**:
- Memory usage is deterministic and extremely stable
- Choose configuration based on available memory
- Use +10% safety margin for production deployments
- Speed is independent of bucket size

**Happy Solving!** 🎲
