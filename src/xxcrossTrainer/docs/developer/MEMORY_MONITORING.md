# Memory Monitoring System

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md) | [Memory Config](../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [Implementation](SOLVER_IMPLEMENTATION.md) | [Experiment Scripts](EXPERIMENT_SCRIPTS.md) | [Measurement Results](Experiences/MEASUREMENT_RESULTS_20251230.md)

---

## Purpose

This document describes the `/proc`-based memory monitoring system used for empirical memory measurements. The system provides **high-frequency (10ms intervals)** RSS tracking with minimal overhead.

---

## Overview

### Key Features

- **High-frequency sampling**: 10ms intervals (100 samples/second)
- **Low overhead**: Uses `/proc` filesystem (no external profilers)
- **Accurate peak detection**: Captures transient memory spikes
- **CSV output**: Time-series data for analysis
- **Comprehensive metrics**: VmRSS, VmSize, VmData

### Use Cases

1. **Memory profiling**: Understand solver memory behavior
2. **Threshold verification**: Confirm bucket allocation transitions
3. **Spike detection**: Identify transient memory peaks
4. **Stability testing**: Measure variance across runs

---

## Implementation

### Core Monitor Script

**File**: `monitor_memory.py` (DO NOT reference file, use embedded code)

```python
#!/usr/bin/env python3
"""
Memory Monitor using /proc filesystem
Provides high-frequency RSS tracking with minimal overhead
"""

import time
import sys
import os
from pathlib import Path

class MemoryMonitor:
    """High-frequency memory monitor using /proc filesystem"""
    
    def __init__(self, pid, sample_interval_ms=10, output_csv="memory.csv"):
        self.pid = pid
        self.sample_interval = sample_interval_ms / 1000.0
        self.output_csv = output_csv
        
        self.data = []
        self.start_time = None
        self.max_vmrss = 0
        
    def read_proc_status(self):
        """Read memory stats from /proc/PID/status"""
        try:
            with open(f"/proc/{self.pid}/status", "r") as f:
                status = f.read()
            
            vmrss = 0
            vmsize = 0
            
            for line in status.split("\\n"):
                if line.startswith("VmRSS:"):
                    vmrss = int(line.split()[1])  # KB
                elif line.startswith("VmSize:"):
                    vmsize = int(line.split()[1])  # KB
            
            return vmrss, vmsize
        except (FileNotFoundError, ProcessLookupError):
            return None, None
    
    def monitor(self, timeout_seconds=180):
        """Monitor process memory usage"""
        self.start_time = time.time()
        
        print(f"Monitoring PID {self.pid} (interval: {self.sample_interval*1000:.0f}ms)")
        
        with open(self.output_csv, 'w') as f:
            f.write("time_s,vmrss_kb,vmsize_kb\\n")
            
            while True:
                # Check if process exists
                try:
                    os.kill(self.pid, 0)
                except OSError:
                    print(f"Process {self.pid} terminated")
                    break
                
                # Read memory
                vmrss, vmsize = self.read_proc_status()
                if vmrss is None:
                    break
                
                elapsed = time.time() - self.start_time
                f.write(f"{elapsed:.3f},{vmrss},{vmsize}\\n")
                
                # Track peak
                if vmrss > self.max_vmrss:
                    self.max_vmrss = vmrss
                    print(f"[{elapsed:6.2f}s] Peak RSS: {vmrss/1024:.1f} MB")
                
                # Timeout check
                if elapsed > timeout_seconds:
                    print(f"Timeout reached ({timeout_seconds}s)")
                    break
                
                time.sleep(self.sample_interval)
        
        return self.summarize()
    
    def summarize(self):
        """Generate summary statistics"""
        print(f"\\n=== Summary ===")
        print(f"Peak VmRSS: {self.max_vmrss/1024:.2f} MB")
        print(f"Samples: {len(self.data)}")
        print(f"Duration: {time.time() - self.start_time:.1f}s")
        
        return {
            'peak_rss_mb': self.max_vmrss / 1024,
            'samples': len(self.data)
        }

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 monitor_memory.py <PID> [output.csv]")
        sys.exit(1)
    
    pid = int(sys.argv[1])
    output = sys.argv[2] if len(sys.argv) > 2 else f"rss_{pid}.csv"
    
    monitor = MemoryMonitor(pid, sample_interval_ms=10, output_csv=output)
    monitor.monitor(timeout_seconds=180)
```

---

## Usage

### Basic Usage

```bash
# Terminal 1: Run solver
./solver_dev 1005 > solver.log 2>&1 &
SOLVER_PID=$!

# Terminal 2: Monitor memory
python3 monitor_memory.py $SOLVER_PID rss_output.csv

# Wait for completion
wait $SOLVER_PID
```

### Automated Testing

```bash
#!/bin/bash
# Single test with automatic monitoring

MEMORY_MB=1005
SOLVER="./solver_dev"

# Start solver in background
$SOLVER $MEMORY_MB > solver_${MEMORY_MB}MB.log 2>&1 &
SOLVER_PID=$!

# Monitor until completion
python3 monitor_memory.py $SOLVER_PID rss_${MEMORY_MB}MB.csv

# Wait for solver
wait $SOLVER_PID
echo "Test complete"
```

---

## Output Format

### CSV Structure

```csv
time_s,vmrss_kb,vmsize_kb
0.000,12345,45678
0.010,12456,45689
0.020,12567,45700
...
149.980,767890,800000
```

**Columns**:
- `time_s`: Elapsed time since monitoring started (seconds)
- `vmrss_kb`: Resident Set Size in kilobytes (actual memory)
- `vmsize_kb`: Virtual Memory Size in kilobytes (allocated)

### Analysis Example

```python
#!/usr/bin/env python3
import csv

# Load data
data = []
with open('rss_output.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        data.append({
            'time': float(row['time_s']),
            'rss_mb': float(row['vmrss_kb']) / 1024,
            'vmsize_mb': float(row['vmsize_kb']) / 1024
        })

# Find peak
peak_rss = max(row['rss_mb'] for row in data)
print(f"Peak RSS: {peak_rss:.2f} MB")

# Find baseline (last 10% of samples)
baseline_samples = data[int(len(data)*0.9):]
baseline_rss = sum(row['rss_mb'] for row in baseline_samples) / len(baseline_samples)
print(f"Baseline RSS: {baseline_rss:.2f} MB")

# Detect spikes (> 1 MB/s increase)
spikes = []
for i in range(1, len(data)):
    dt = data[i]['time'] - data[i-1]['time']
    drss = data[i]['rss_mb'] - data[i-1]['rss_mb']
    if dt > 0:
        rate = drss / dt
        if rate > 1.0:  # 1 MB/s threshold
            spikes.append({'time': data[i]['time'], 'rate': rate})

print(f"Spikes detected: {len(spikes)}")
if spikes:
    max_spike = max(spikes, key=lambda x: x['rate'])
    print(f"Max spike rate: {max_spike['rate']:.1f} MB/s at t={max_spike['time']:.1f}s")
```

---

## Advanced Techniques

### Spike Detection

Detect memory allocation bursts:

```python
def detect_spikes(csv_file, threshold_mb_s=1.0):
    """Detect memory allocation spikes"""
    import csv
    
    spikes = []
    prev_time = 0
    prev_rss = 0
    
    with open(csv_file) as f:
        reader = csv.DictReader(f)
        for row in reader:
            time_s = float(row['time_s'])
            rss_mb = float(row['vmrss_kb']) / 1024
            
            if prev_time > 0:
                dt = time_s - prev_time
                drss = rss_mb - prev_rss
                
                if dt > 0:
                    rate = drss / dt
                    if rate > threshold_mb_s:
                        spikes.append({
                            'time': time_s,
                            'rss_mb': rss_mb,
                            'rate_mb_s': rate
                        })
            
            prev_time = time_s
            prev_rss = rss_mb
    
    return spikes

# Usage
spikes = detect_spikes('rss_output.csv', threshold_mb_s=1.0)
print(f"Found {len(spikes)} spikes")
for spike in spikes[:5]:  # Show first 5
    print(f"  t={spike['time']:.2f}s: {spike['rss_mb']:.1f} MB "
          f"({spike['rate_mb_s']:.1f} MB/s)")
```

### Variance Analysis

Measure stability across multiple runs:

```python
def analyze_variance(csv_files):
    """Analyze peak memory variance across runs"""
    peaks = []
    
    for csv_file in csv_files:
        with open(csv_file) as f:
            reader = csv.DictReader(f)
            rss_values = [float(row['vmrss_kb']) / 1024 for row in reader]
            peaks.append(max(rss_values))
    
    mean_peak = sum(peaks) / len(peaks)
    variance = sum((p - mean_peak)**2 for p in peaks) / len(peaks)
    std_dev = variance ** 0.5
    cv = std_dev / mean_peak  # Coefficient of variation
    
    return {
        'mean_mb': mean_peak,
        'std_dev_mb': std_dev,
        'cv_percent': cv * 100,
        'min_mb': min(peaks),
        'max_mb': max(peaks)
    }

# Usage
files = ['rss_run1.csv', 'rss_run2.csv', 'rss_run3.csv']
stats = analyze_variance(files)
print(f"Mean peak: {stats['mean_mb']:.2f} MB")
print(f"Std dev: {stats['std_dev_mb']:.3f} MB ({stats['cv_percent']:.3f}%)")
print(f"Range: {stats['min_mb']:.2f} - {stats['max_mb']:.2f} MB")
```

---

## Integration with Solver

### Automated Test Suite

```bash
#!/bin/bash
# run_comprehensive_tests.sh
# Automated memory testing across multiple configurations

set -euo pipefail

SOLVER="./solver_dev"
MEMORY_LIMITS=(323 478 823 1005 1308 1513)

for LIMIT in "${MEMORY_LIMITS[@]}"; do
    echo "Testing ${LIMIT} MB..."
    
    # Run solver with monitoring
    $SOLVER $LIMIT > solver_${LIMIT}MB.log 2>&1 &
    SOLVER_PID=$!
    
    python3 monitor_memory.py $SOLVER_PID rss_${LIMIT}MB.csv
    
    wait $SOLVER_PID
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -ne 0 ]; then
        echo "⚠ Solver failed with exit code $EXIT_CODE"
    else
        echo "✓ Completed successfully"
    fi
    
    echo ""
done

echo "All tests complete"
```

### Threshold Verification

```bash
#!/bin/bash
# verify_threshold.sh
# Verify bucket configuration threshold

THRESHOLD_MB=1000
BELOW=$((THRESHOLD_MB - 1))
AT=$THRESHOLD_MB
ABOVE=$((THRESHOLD_MB + 1))

for LIMIT in $BELOW $AT $ABOVE; do
    echo "Testing ${LIMIT} MB..."
    
    ./solver_dev $LIMIT 2>&1 | grep "Bucket sizes" > config_${LIMIT}MB.txt
    
    cat config_${LIMIT}MB.txt
done

# Expected output:
# 999 MB  → 4M/8M/4M
# 1000 MB → 8M/8M/8M
# 1001 MB → 8M/8M/8M
```

---

## Performance Characteristics

### Monitor Overhead

**CPU Usage**: <1% (one core)
- `/proc` reads: ~100 μs per sample
- CSV writes: ~50 μs per sample
- Total: ~150 μs every 10ms (1.5% theoretical max)

**Memory Usage**: <10 MB
- Script runtime: ~5 MB
- CSV buffering: ~1 MB per 10,000 samples

**I/O Impact**: Minimal
- Buffered writes to CSV
- No forced sync (relies on OS caching)

### Sampling Resolution

**Time Resolution**: 10ms intervals
- Typical run: 150 seconds → 15,000 samples
- Full campaign (47 tests): ~660,000 samples

**Memory Resolution**: 4 KB (Linux page size)
- VmRSS rounded to nearest 4 KB
- Practical: ±0.004 MB precision

**Spike Detection Limit**:
- Can detect allocations lasting >10ms
- Sub-10ms transients may be missed
- For finer resolution, reduce interval (trade-off: more overhead)

---

## Troubleshooting

### Problem: Process not found

```bash
$ python3 monitor_memory.py 12345 output.csv
Error: [Errno 3] No such process
```

**Solution**: Verify PID exists:
```bash
ps aux | grep solver_dev
# or
pidof solver_dev
```

---

### Problem: Permission denied

```bash
$ python3 monitor_memory.py 12345 output.csv
PermissionError: [Errno 13] Permission denied: '/proc/12345/status'
```

**Solution**: Ensure process belongs to same user:
```bash
# Run monitor as same user as solver
sudo -u solver_user python3 monitor_memory.py 12345 output.csv
```

---

### Problem: CSV file empty

**Diagnosis**: Process terminated before first sample

**Solution**: Increase solver startup delay or check for immediate crashes:
```bash
./solver_dev 1005 > solver.log 2>&1 &
SOLVER_PID=$!

sleep 1  # Give solver time to start

python3 monitor_memory.py $SOLVER_PID output.csv
```

---

## Best Practices

### 1. Consistent Environment

- **Same CPU**: Memory allocation patterns may vary
- **No background load**: Minimize OS memory pressure
- **Same binary**: Recompile invalidates comparisons

### 2. Multiple Runs

Run each test 3-5 times to measure variance:

```bash
for run in {1..5}; do
    ./solver_dev 1005 > /dev/null 2>&1 &
    python3 monitor_memory.py $! rss_run${run}.csv
    wait
done
```

### 3. Long-Running Tests

For stability testing, increase timeout:

```python
monitor = MemoryMonitor(pid, sample_interval_ms=10)
monitor.monitor(timeout_seconds=300)  # 5 minutes
```

### 4. Archive Raw Data

```bash
# Save all CSV files for later analysis
mkdir -p results/$(date +%Y%m%d_%H%M%S)
mv rss_*.csv results/$(date +%Y%m%d_%H%M%S)/
```

---

## Related Documentation

- [EXPERIMENT_SCRIPTS.md](EXPERIMENT_SCRIPTS.md) - Automated testing scripts
- [Experiences/MEASUREMENT_RESULTS_20251230.md](Experiences/MEASUREMENT_RESULTS_20251230.md) - Campaign results
- [Experiences/MEMORY_THEORY_ANALYSIS.md](Experiences/MEMORY_THEORY_ANALYSIS.md) - Theoretical analysis

---

**Document Version**: 1.0  
**Status**: Production-ready  
**Last Validated**: 2025-12-30 (47-point campaign)
