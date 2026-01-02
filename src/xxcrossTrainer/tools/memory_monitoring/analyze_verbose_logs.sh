#!/bin/bash
# Memory Spike Analysis from VERBOSE Logs
# Extracts RSS checkpoints and identifies peak memory usage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XXCROSS_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SOLVER="$XXCROSS_DIR/solver_dev"
RESULTS_DIR="$XXCROSS_DIR/results/spike_analysis_verbose/run_$(date +%Y%m%d_%H%M%S)"

mkdir -p "$RESULTS_DIR"

echo "=========================================="
echo "Memory Spike Analysis from VERBOSE Logs"
echo "=========================================="
echo "Solver: $SOLVER"
echo "Method: RSS checkpoints from VERBOSE=1 output"
echo "Results: $RESULTS_DIR"
echo ""

test_model() {
    local MODEL=$1
    local NAME=$2
    
    echo "=========================================="
    echo "Testing: $MODEL ($NAME)"
    echo "=========================================="
    
    local SOLVER_LOG="$RESULTS_DIR/${NAME}_verbose.log"
    local RSS_EXTRACT="$RESULTS_DIR/${NAME}_rss_checkpoints.txt"
    local ANALYSIS="$RESULTS_DIR/${NAME}_analysis.txt"
    
    # Run solver with VERBOSE=1
    cd "$XXCROSS_DIR"
    echo "Running solver with VERBOSE=1..."
    BUCKET_MODEL=$MODEL ENABLE_CUSTOM_BUCKETS=1 VERBOSE=1 timeout 300 "$SOLVER" > "$SOLVER_LOG" 2>&1
    EXITCODE=$?
    
    echo "Exit code: $EXITCODE"
    
    if [ $EXITCODE -ne 0 ] && [ $EXITCODE -ne 124 ]; then
        echo "⚠️  Solver exited with error code $EXITCODE"
        tail -20 "$SOLVER_LOG"
        return 1
    else
        echo "✅ Solver completed successfully"
    fi
    
    # Extract all RSS measurements
    echo ""
    echo "Extracting RSS checkpoints..."
    grep -E "RSS|Peak" "$SOLVER_LOG" | grep -E "MB|KB" > "$RSS_EXTRACT" || true
    
    # Analyze
    cat > "$ANALYSIS" << 'EOF_ANALYSIS'
=== Memory Analysis from VERBOSE Checkpoints ===

EOF_ANALYSIS
    
    # Find peak RSS
    PEAK_RSS=$(grep -oP "RSS.*?: \K[0-9.]+(?= MB)" "$RSS_EXTRACT" | sort -n | tail -1)
    FINAL_RSS=$(grep "RSS (after search" "$RSS_EXTRACT" | tail -1 | grep -oP "\K[0-9.]+(?= MB)" || echo "N/A")
    
    echo "Model: $MODEL" >> "$ANALYSIS"
    echo "Peak RSS: ${PEAK_RSS:-N/A} MB" >> "$ANALYSIS"
    echo "Final RSS: ${FINAL_RSS:-N/A} MB" >> "$ANALYSIS"
    echo "" >> "$ANALYSIS"
    
    echo "Phase-by-Phase RSS Progression:" >> "$ANALYSIS"
    grep -E "(Phase [1-4]|RSS (before|after|at))" "$RSS_EXTRACT" | head -40 >> "$ANALYSIS"
    
    cat "$ANALYSIS"
    
    echo ""
    echo "📊 Peak RSS: ${PEAK_RSS:-N/A} MB"
    echo "📊 Final RSS: ${FINAL_RSS:-N/A} MB"
    echo ""
}

# Test all three models
test_model "2M/2M/2M" "2m"
test_model "4M/4M/4M" "4m"
test_model "8M/8M/8M" "8m"

echo "=========================================="
echo "All Tests Complete"
echo "=========================================="
echo "Results: $RESULTS_DIR"
echo ""
echo "Files generated:"
ls -lh "$RESULTS_DIR"
echo ""
echo "Peak RSS Summary:"
for model in 2m 4m 8m; do
    if [ -f "$RESULTS_DIR/${model}_analysis.txt" ]; then
        echo -n "  $model: "
        grep "Peak RSS:" "$RESULTS_DIR/${model}_analysis.txt" | head -1
    fi
done
