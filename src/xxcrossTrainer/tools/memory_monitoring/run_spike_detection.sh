#!/bin/bash
# Memory Spike Detection with 10ms Sampling
# Based on MEMORY_MONITORING.md methodology

set -euo pipefail

# Paths within src/xxcrossTrainer
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XXCROSS_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SOLVER="$XXCROSS_DIR/solver_dev"
MONITOR="$SCRIPT_DIR/monitor_memory.py"
ANALYZER="$SCRIPT_DIR/analyze_spikes.py"
RESULTS_DIR="$XXCROSS_DIR/results/spike_detection/run_$(date +%Y%m%d_%H%M%S)"

mkdir -p "$RESULTS_DIR"

echo "=========================================="
echo "Memory Spike Detection Test Suite"
echo "=========================================="
echo "Solver: $SOLVER"
echo "Monitor: 10ms sampling (100 samples/sec)"
echo "Results: $RESULTS_DIR"
echo ""

test_model() {
    local MODEL=$1
    local NAME=$2
    
    echo "=========================================="
    echo "Testing: $MODEL ($NAME)"
    echo "=========================================="
    
    local SOLVER_LOG="$RESULTS_DIR/${NAME}_solver.log"
    local RSS_CSV="$RESULTS_DIR/${NAME}_rss.csv"
    local ANALYSIS_TXT="$RESULTS_DIR/${NAME}_analysis.txt"
    
    # Start solver in background
    cd "$XXCROSS_DIR"
    BUCKET_MODEL=$MODEL ENABLE_CUSTOM_BUCKETS=1 VERBOSE=1 timeout 300 "$SOLVER" > "$SOLVER_LOG" 2>&1 &
    SOLVER_PID=$!
    
    echo "Solver PID: $SOLVER_PID"
    echo "Waiting for process to initialize..."
    sleep 1.0
    
    # Verify process is running
    if ! kill -0 $SOLVER_PID 2>/dev/null; then
        echo "❌ Solver failed to start"
        cat "$SOLVER_LOG"
        return 1
    fi
    
    # Monitor with 10ms sampling
    echo "Starting 10ms memory monitoring..."
    python3 "$MONITOR" $SOLVER_PID "$RSS_CSV"
    
    # Wait for solver
    wait $SOLVER_PID
    EXITCODE=$?
    
    echo ""
    echo "Exit code: $EXITCODE"
    
    if [ $EXITCODE -ne 0 ]; then
        echo "⚠️  Solver exited with error code $EXITCODE"
        echo "Last 20 lines of solver log:"
        tail -20 "$SOLVER_LOG"
    else
        echo "✅ Solver completed successfully"
    fi
    
    # Analyze spikes
    if [ -f "$RSS_CSV" ]; then
        echo ""
        echo "Analyzing memory spikes..."
        python3 "$ANALYZER" "$RSS_CSV" > "$ANALYSIS_TXT"
        cat "$ANALYSIS_TXT"
        
        # Extract peak RSS from analysis
        PEAK_RSS=$(grep "Peak RSS:" "$ANALYSIS_TXT" | awk '{print $3, $4}')
        echo ""
        echo "📊 Peak RSS: $PEAK_RSS"
    else
        echo "❌ No CSV output found"
    fi
    
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
echo "Summary of peaks:"
grep "Peak RSS:" "$RESULTS_DIR"/*_analysis.txt || echo "No analysis files found"
echo ""
echo "To view detailed analysis:"
echo "  cat $RESULTS_DIR/2m_analysis.txt"
echo "  cat $RESULTS_DIR/4m_analysis.txt"
echo "  cat $RESULTS_DIR/8m_analysis.txt"
echo ""
echo "To plot time-series (requires gnuplot):"
echo "  gnuplot -p -e 'plot \"$RESULTS_DIR/8m_rss.csv\" using 1:(\$2/1024) with lines title \"8M RSS\"'"
