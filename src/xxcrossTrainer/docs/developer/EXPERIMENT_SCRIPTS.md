# Experiment Scripts Reference

**Version**: stable-20251230  
**Last Updated**: 2025-12-30

> **Navigation**: [← Back to Developer Docs](../README.md) | [User Guide](../USER_GUIDE.md) | [Memory Config](../MEMORY_CONFIGURATION_GUIDE.md)
>
> **Related**: [Memory Monitoring](MEMORY_MONITORING.md) | [WASM Scripts](WASM_EXPERIMENT_SCRIPTS.md) | [Measurement Results](Experiences/MEASUREMENT_RESULTS_20251230.md)

---

## Purpose

This document provides **complete implementation examples** of shell scripts used for memory testing and threshold verification. These scripts were used in the comprehensive 47-point measurement campaign.

**Note**: Script files (`.sh`) are archived and may be removed from the repository. Use the implementations below as reference.

---

## Script 1: Comprehensive Memory Measurement

### Purpose
Run solver with multiple memory limits and collect RSS data for analysis.

### Implementation

```bash
#!/bin/bash
#
# run_measurements.sh
# Comprehensive Memory Measurement Script
# Runs solver with various memory limits and measures actual memory usage
#

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOLVER_BINARY="${SCRIPT_DIR}/solver_dev_test"
MONITOR_SCRIPT="${SCRIPT_DIR}/monitor_memory.py"
LOG_DIR="${SCRIPT_DIR}/tmp_log"
RESULTS_FILE="${LOG_DIR}/measurement_results.csv"

# Ensure directories exist
mkdir -p "${LOG_DIR}"

# Check if solver exists
if [ ! -f "${SOLVER_BINARY}" ]; then
    echo "Error: Solver binary not found: ${SOLVER_BINARY}"
    echo "Please compile: g++ -O3 -std=c++17 -o solver_dev_test solver_dev.cpp"
    exit 1
fi

# Show solver info
echo "Solver binary info:"
ls -lh "${SOLVER_BINARY}"
echo "MD5: $(md5sum ${SOLVER_BINARY} | awk '{print $1}')"
echo ""

# Load test points from theoretical predictions
if [ ! -f "${LOG_DIR}/theoretical_predictions_corrected.csv" ]; then
    echo "Error: Theoretical predictions not found"
    echo "Please run: python3 generate_predictions_corrected.py"
    exit 1
fi

# Extract memory limits (skip header)
MEMORY_LIMITS=($(tail -n +2 "${LOG_DIR}/theoretical_predictions_corrected.csv" | cut -d',' -f1 | sort -n))

echo "======================================================================"
echo "Comprehensive Memory Measurement"
echo "======================================================================"
echo "Solver: ${SOLVER_BINARY}"
echo "Test points: ${#MEMORY_LIMITS[@]}"
echo ""

# Initialize results file
echo "memory_limit_mb,actual_peak_mb,actual_vmsize_mb,duration_s,exit_code" > "${RESULTS_FILE}"

# Function to run single measurement
run_measurement() {
    local memory_mb=$1
    local solver_log="${LOG_DIR}/solver_${memory_mb}MB.log"
    local monitor_log="${LOG_DIR}/monitor_${memory_mb}MB.log"
    local rss_csv="${LOG_DIR}/rss_${memory_mb}MB.csv"
    
    echo "----------------------------------------"
    echo "[$(date +%H:%M:%S)] Testing: ${memory_mb} MB"
    echo "----------------------------------------"
    
    # Start solver in background
    "${SOLVER_BINARY}" "${memory_mb}" > "${solver_log}" 2>&1 &
    local solver_pid=$!
    
    # Monitor with timeout
    if timeout 180 python3 "${MONITOR_SCRIPT}" "${solver_pid}" "${rss_csv}" > "${monitor_log}" 2>&1; then
        local exit_code=0
        echo "✓ Completed successfully"
    else
        local exit_code=$?
        echo "⚠ Exited with code: ${exit_code}"
    fi
    
    # Wait for solver
    wait "${solver_pid}" 2>/dev/null || true
    
    # Extract metrics
    local peak_mb="N/A"
    local vmsize_mb="N/A"
    local duration="N/A"
    
    if [ -f "${monitor_log}" ]; then
        peak_mb=$(grep "Peak VmRSS:" "${monitor_log}" 2>/dev/null | tail -n 1 | awk '{print $3}' || echo "N/A")
        vmsize_mb=$(grep "Peak VmSize:" "${monitor_log}" 2>/dev/null | tail -n 1 | awk '{print $3}' || echo "N/A")
        duration=$(grep "Duration:" "${monitor_log}" 2>/dev/null | tail -n 1 | awk '{print $2}' || echo "N/A")
    fi
    
    # Append to results
    echo "${memory_mb},${peak_mb},${vmsize_mb},${duration},${exit_code}" >> "${RESULTS_FILE}"
    
    echo ""
}

# Main loop
tested=0
total=${#MEMORY_LIMITS[@]}

for memory_mb in "${MEMORY_LIMITS[@]}"; do
    run_measurement "${memory_mb}"
    tested=$((tested + 1))  # Safe arithmetic
    echo "Progress: ${tested}/${total} tests complete"
done

echo "======================================================================"
echo "Measurement Complete"
echo "======================================================================"
echo "Results saved to: ${RESULTS_FILE}"
echo "Logs saved to: ${LOG_DIR}"
echo ""
echo "Summary:"
echo "  Total tests: ${total}"
echo "  Successful: $(grep -c ",0$" "${RESULTS_FILE}" || echo "0")"
echo "  Failed: $(grep -c -v ",0$" "${RESULTS_FILE}" | tail -n +2 || echo "0")"
```

### Usage

```bash
# Generate test points first
python3 generate_predictions_corrected.py

# Run measurements (takes ~2 hours for 47 points)
nohup ./run_measurements.sh > measurement_log.txt 2>&1 &

# Monitor progress
tail -f measurement_log.txt

# Check results
cat tmp_log/measurement_results.csv
```

---

## Script 2: Threshold Verification

### Purpose
Verify that bucket configuration transitions occur at exact thresholds.

### Implementation

```bash
#!/bin/bash
#
# verify_thresholds.sh
# Verify bucket configuration thresholds with ±1 MB precision
#

set -euo pipefail

SOLVER="./solver_dev"

# All known thresholds
THRESHOLDS=(569 613 921 1000 1537 1702)

echo "======================================================================"
echo "Threshold Verification"
echo "======================================================================"
echo ""

for THRESHOLD in "${THRESHOLDS[@]}"; do
    BELOW=$((THRESHOLD - 1))
    AT=$THRESHOLD
    ABOVE=$((THRESHOLD + 1))
    
    echo "Threshold: ${THRESHOLD} MB"
    echo "----------------------------------------"
    
    # Test below threshold
    CONFIG_BELOW=$($SOLVER $BELOW 2>&1 | grep "Bucket sizes" | head -n 1)
    echo "  ${BELOW} MB: ${CONFIG_BELOW}"
    
    # Test at threshold
    CONFIG_AT=$($SOLVER $AT 2>&1 | grep "Bucket sizes" | head -n 1)
    echo "  ${AT} MB: ${CONFIG_AT}"
    
    # Test above threshold
    CONFIG_ABOVE=$($SOLVER $ABOVE 2>&1 | grep "Bucket sizes" | head -n 1)
    echo "  ${ABOVE} MB: ${CONFIG_ABOVE}"
    
    # Verify transition
    if [ "$CONFIG_BELOW" != "$CONFIG_AT" ]; then
        echo "  ✓ Transition confirmed at ${THRESHOLD} MB"
    elif [ "$CONFIG_AT" != "$CONFIG_ABOVE" ]; then
        echo "  ⚠ Transition occurs between ${AT} and ${ABOVE} MB"
    else
        echo "  ✗ No transition detected"
    fi
    
    echo ""
done

echo "======================================================================"
echo "Verification Complete"
echo "======================================================================"
```

### Expected Output

```
Threshold: 569 MB
----------------------------------------
  568 MB: Bucket sizes: 7=2MB (2097152), 8=2MB (2097152), 9=2MB (2097152)
  569 MB: Bucket sizes: 7=2MB (2097152), 8=4MB (4194304), 9=2MB (2097152)
  570 MB: Bucket sizes: 7=2MB (2097152), 8=4MB (4194304), 9=2MB (2097152)
  ✓ Transition confirmed at 569 MB

Threshold: 1000 MB
----------------------------------------
  999 MB: Bucket sizes: 7=4MB (4194304), 8=8MB (8388608), 9=4MB (4194304)
  1000 MB: Bucket sizes: 7=8MB (8388608), 8=8MB (8388608), 9=8MB (8388608)
  1001 MB: Bucket sizes: 7=8MB (8388608), 8=8MB (8388608), 9=8MB (8388608)
  ✓ Transition confirmed at 1000 MB
```

---

## Script 3: Quick Single Test

### Purpose
Run single test with memory monitoring and save all outputs.

### Implementation

```bash
#!/bin/bash
#
# run_quick_test.sh
# Quick single memory test with monitoring
#

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <memory_mb> [output_prefix]"
    echo "Example: $0 1005 test_8M"
    exit 1
fi

MEMORY_MB=$1
PREFIX=${2:-test_${MEMORY_MB}mb}
SOLVER="./solver_dev"

echo "Running solver with ${MEMORY_MB} MB limit"
echo "Output prefix: ${PREFIX}"
echo ""

# Start solver
$SOLVER $MEMORY_MB > ${PREFIX}_solver.log 2>&1 &
SOLVER_PID=$!

echo "Solver PID: $SOLVER_PID"

# Monitor memory
python3 monitor_memory.py $SOLVER_PID ${PREFIX}_rss.csv > ${PREFIX}_monitor.log 2>&1

# Wait for solver
wait $SOLVER_PID
EXIT_CODE=$?

echo ""
echo "======================================================================"
echo "Test Complete"
echo "======================================================================"
echo "Exit code: $EXIT_CODE"
echo ""
echo "Output files:"
echo "  Solver log: ${PREFIX}_solver.log"
echo "  Monitor log: ${PREFIX}_monitor.log"
echo "  RSS data: ${PREFIX}_rss.csv"
echo ""

# Show peak memory
PEAK=$(grep "Peak VmRSS:" ${PREFIX}_monitor.log | tail -n 1 | awk '{print $3}')
echo "Peak memory: ${PEAK} MB"
```

### Usage

```bash
# Quick test
./run_quick_test.sh 1005 my_test

# Output files created:
# - my_test_solver.log
# - my_test_monitor.log
# - my_test_rss.csv
```

---

## Script 4: Batch Configuration Test

### Purpose
Test all recommended configurations quickly.

### Implementation

```bash
#!/bin/bash
#
# test_recommended_configs.sh
# Test all recommended memory configurations
#

set -euo pipefail

SOLVER="./solver_dev"
CONFIGS=(323 478 823 1005 1308 1513)

echo "======================================================================"
echo "Testing Recommended Configurations"
echo "======================================================================"
echo ""

for CONFIG in "${CONFIGS[@]}"; do
    echo "[$(date +%H:%M:%S)] Testing ${CONFIG} MB..."
    
    LOG_FILE="test_${CONFIG}MB_$(date +%Y%m%d_%H%M%S).log"
    
    if $SOLVER $CONFIG > $LOG_FILE 2>&1; then
        PEAK=$(grep "Peak RSS" $LOG_FILE | tail -n 1 | awk '{print $3}')
        echo "  ✓ Success - Peak: ${PEAK} MB"
    else
        echo "  ✗ Failed (see $LOG_FILE)"
    fi
    
    echo ""
done

echo "All tests complete"
```

---

## Script 5: Spike Analysis

### Purpose
Analyze RSS CSV files to detect memory allocation spikes.

### Implementation

```bash
#!/bin/bash
#
# analyze_spikes.sh
# Analyze RSS CSV files for memory spikes
#

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <rss_csv_file> [threshold_mb_s]"
    exit 1
fi

CSV_FILE=$1
THRESHOLD=${2:-1.0}  # MB/s

# Python inline script for spike detection
python3 << EOF
import csv
import sys

csv_file = '${CSV_FILE}'
threshold = float('${THRESHOLD}')

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
                if rate > threshold:
                    spikes.append({
                        'time': time_s,
                        'rss': rss_mb,
                        'rate': rate
                    })
        
        prev_time = time_s
        prev_rss = rss_mb

print(f"Spike Analysis: {csv_file}")
print(f"Threshold: {threshold} MB/s")
print(f"Spikes detected: {len(spikes)}")
print("")

if spikes:
    print("Top 5 spikes:")
    spikes.sort(key=lambda x: x['rate'], reverse=True)
    for spike in spikes[:5]:
        print(f"  t={spike['time']:6.2f}s: {spike['rss']:7.1f} MB ({spike['rate']:6.1f} MB/s)")
EOF
```

### Usage

```bash
# Analyze with default threshold (1.0 MB/s)
./analyze_spikes.sh rss_1005MB.csv

# Use custom threshold
./analyze_spikes.sh rss_1005MB.csv 2.0
```

---

## Script 6: Multi-Run Variance Test

### Purpose
Run same configuration multiple times to measure variance.

### Implementation

```bash
#!/bin/bash
#
# test_variance.sh
# Run multiple tests to measure memory variance
#

set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <memory_mb> <num_runs>"
    echo "Example: $0 1005 5"
    exit 1
fi

MEMORY_MB=$1
NUM_RUNS=$2
SOLVER="./solver_dev"

echo "======================================================================"
echo "Variance Test"
echo "======================================================================"
echo "Configuration: ${MEMORY_MB} MB"
echo "Runs: ${NUM_RUNS}"
echo ""

PEAKS=()

for run in $(seq 1 $NUM_RUNS); do
    echo "[Run $run/$NUM_RUNS] Testing..."
    
    PREFIX="variance_${MEMORY_MB}MB_run${run}"
    
    # Run test
    $SOLVER $MEMORY_MB > ${PREFIX}_solver.log 2>&1 &
    SOLVER_PID=$!
    
    python3 monitor_memory.py $SOLVER_PID ${PREFIX}_rss.csv > ${PREFIX}_monitor.log 2>&1
    
    wait $SOLVER_PID
    
    # Extract peak
    PEAK=$(grep "Peak VmRSS:" ${PREFIX}_monitor.log | tail -n 1 | awk '{print $3}')
    PEAKS+=($PEAK)
    
    echo "  Peak: ${PEAK} MB"
    echo ""
done

# Calculate statistics
python3 << EOF
peaks = [${PEAKS[@]}]

mean = sum(peaks) / len(peaks)
variance = sum((p - mean)**2 for p in peaks) / len(peaks)
std_dev = variance ** 0.5
cv = (std_dev / mean) * 100

print("======================================================================"
print("Variance Analysis")
print("======================================================================")
print(f"Runs: {len(peaks)}")
print(f"Mean peak: {mean:.2f} MB")
print(f"Std dev: {std_dev:.3f} MB")
print(f"CV: {cv:.3f}%")
print(f"Range: {min(peaks):.2f} - {max(peaks):.2f} MB")
EOF
```

### Usage

```bash
# Run 5 tests
./test_variance.sh 1005 5

# Expected output:
# Variance Analysis
# ======================================================================
# Runs: 5
# Mean peak: 748.12 MB
# Std dev: 0.087 MB
# CV: 0.012%
# Range: 748.02 - 748.24 MB
```

---

## Best Practices

### 1. Always Use Absolute Paths

```bash
# Good
SOLVER="${SCRIPT_DIR}/solver_dev"

# Bad
SOLVER="./solver_dev"  # Breaks if PWD changes
```

### 2. Handle Process Termination Safely

```bash
# Start background process
./solver_dev 1005 &
SOLVER_PID=$!

# Safe wait
wait $SOLVER_PID 2>/dev/null || true
```

### 3. Arithmetic in Bash

```bash
# Safe (always succeeds)
tested=$((tested + 1))

# Unsafe with set -e (fails when tested=0)
((tested++))
```

### 4. Check File Existence

```bash
if [ ! -f "${SOLVER_BINARY}" ]; then
    echo "Error: Solver not found"
    exit 1
fi
```

### 5. Use Timeouts

```bash
# Prevent hanging
timeout 180 ./solver_dev 1005
```

---

## Related Documentation

- [MEMORY_MONITORING.md](MEMORY_MONITORING.md) - Memory monitoring system
- [Experiences/MEASUREMENT_RESULTS_20251230.md](Experiences/MEASUREMENT_RESULTS_20251230.md) - Campaign results
- [Experiences/MEASUREMENT_SUMMARY.md](Experiences/MEASUREMENT_SUMMARY.md) - Executive summary

---

**Document Version**: 1.0  
**Status**: Production-ready  
**Scripts Archived**: Original .sh files backed up, may be removed from repository
