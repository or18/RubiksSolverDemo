#!/bin/bash
# 10ms interval /proc-based memory monitoring
# Detects transient spikes during hash table operations
# Based on stable-20251230 measurement system

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/src/xxcrossTrainer"

# Configuration
MEMORY_LIMIT=${1:-1600}  # Default: 1600 MB
BUCKET_MODEL=${2:-8M/8M/8M}  # Default: largest model
OUTPUT_DIR="$SCRIPT_DIR/measurements_$(date +%Y%m%d_%H%M%S)"

echo -e "${BLUE}=== 10ms Interval Memory Spike Monitoring ===${NC}"
echo -e "Memory limit: ${GREEN}${MEMORY_LIMIT} MB${NC}"
echo -e "Bucket model: ${GREEN}${BUCKET_MODEL}${NC}"
echo -e "Output directory: ${YELLOW}${OUTPUT_DIR}${NC}"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if solver exists
SOLVER="$BUILD_DIR/solver_dev"
if [ ! -f "$SOLVER" ]; then
    echo -e "${RED}Error: Solver not found: $SOLVER${NC}"
    echo "Please build the solver first:"
    echo "  cd $BUILD_DIR && make solver_dev"
    exit 1
fi

# Start solver in background
echo -e "${BLUE}Starting solver...${NC}"
LOGFILE="$OUTPUT_DIR/solver.log"

(
    cd "$BUILD_DIR"
    export MEMORY_LIMIT_MB=$MEMORY_LIMIT
    export ENABLE_CUSTOM_BUCKETS=1
    export BUCKET_MODEL=$BUCKET_MODEL
    
    ./solver_dev > "$LOGFILE" 2>&1
) &

SOLVER_PID=$!
echo -e "Solver PID: ${GREEN}$SOLVER_PID${NC}"

# Start 10ms monitoring IMMEDIATELY (no delay)
echo -e "${BLUE}Starting 10ms /proc monitoring...${NC}"
CSV_FILE="$OUTPUT_DIR/rss_trace.csv"

python3 "$SCRIPT_DIR/monitor_memory.py" $SOLVER_PID "$CSV_FILE" &
MONITOR_PID=$!

# Wait for solver to complete
wait $SOLVER_PID
SOLVER_EXIT=$?

echo -e "${BLUE}Solver completed (exit code: $SOLVER_EXIT)${NC}"

# Wait a moment for monitor to finish writing
sleep 1

# Stop monitor if still running
if kill -0 $MONITOR_PID 2>/dev/null; then
    kill $MONITOR_PID 2>/dev/null || true
fi

# Analyze results
echo ""
echo -e "${BLUE}=== Analyzing memory trace ===${NC}"
python3 "$SCRIPT_DIR/analyze_spikes.py" "$CSV_FILE"

# Save configuration
cat > "$OUTPUT_DIR/config.txt" <<EOF
Memory Limit: $MEMORY_LIMIT MB
Bucket Model: $BUCKET_MODEL
Solver PID: $SOLVER_PID
Exit Code: $SOLVER_EXIT
Timestamp: $(date)
EOF

# Check for spikes in solver log
echo -e "${BLUE}=== Solver log (last 50 lines) ===${NC}"
tail -50 "$LOGFILE"

echo ""
echo -e "${GREEN}Results saved to: $OUTPUT_DIR${NC}"
echo -e "  - CSV trace: ${YELLOW}rss_trace.csv${NC}"
echo -e "  - Solver log: ${YELLOW}solver.log${NC}"
echo -e "  - Configuration: ${YELLOW}config.txt${NC}"
