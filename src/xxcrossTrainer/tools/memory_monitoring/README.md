# Memory Monitoring Tools

**Location**: `src/xxcrossTrainer/tools/memory_monitoring/`  
**Purpose**: High-frequency (10ms) RSS monitoring for spike detection  
**Version**: 2026-01-02

---

## Tools

### 1. monitor_memory.py

**High-frequency memory monitor** using `/proc/self/status`

**Usage**:
```bash
python3 monitor_memory.py <PID> [output.csv]
```

**Features**:
- 10ms sampling intervals (100 samples/second)
- Minimal overhead (<0.1% CPU)
- Real-time peak RSS reporting
- CSV output: `time_s, vmrss_kb, vmsize_kb`

**Example**:
```bash
# Terminal 1: Start solver
# From workspace root
cd src/xxcrossTrainer
BUCKET_MODEL=8M/8M/8M ENABLE_CUSTOM_BUCKETS=1 ./solver_dev &
SOLVER_PID=$!

# Terminal 2: Monitor
python3 tools/memory_monitoring/monitor_memory.py $SOLVER_PID results/8m_rss.csv
```

---

### 2. analyze_spikes.py

**Spike detection and analysis tool**

**Usage**:
```bash
python3 analyze_spikes.py <csv_file>
```

**Features**:
- Detects allocation bursts (>10 MB/s threshold)
- Reports peak RSS and timeline segments
- Top spikes ranking by rate and magnitude

**Example**:
```bash
python3 tools/memory_monitoring/analyze_spikes.py results/8m_rss.csv
```

**Output**:
- Peak RSS and final RSS
- Peak/Final ratio
- Spike detection (>10 MB/s)
- Timeline segments (quartiles)

---

### 3. run_spike_detection.sh

**Automated test suite for all bucket models**

**Usage**:
```bash
# From workspace root
cd src/xxcrossTrainer
./tools/memory_monitoring/run_spike_detection.sh
```

**Features**:
- Tests 2M/2M/2M, 4M/4M/4M, 8M/8M/8M automatically
- Timestamped output directory
- Generates solver logs, RSS CSVs, analysis reports

**Output**: `results/spike_detection/run_YYYYMMDD_HHMMSS/`
- `{2m,4m,8m}_solver.log` - Solver output
- `{2m,4m,8m}_rss.csv` - Time-series RSS data
- `{2m,4m,8m}_analysis.txt` - Spike detection reports

---

## Methodology

Based on [MEMORY_MONITORING.md](../../docs/developer/MEMORY_MONITORING.md)

**Sampling**:
- Frequency: 10ms (100 Hz)
- Data source: `/proc/self/status` VmRSS
- Duration: Full database construction (~30-150s)

**Analysis**:
- Allocation rate: `dRSS/dt` between samples
- Spike threshold: 10 MB/s (configurable)
- Timeline: Quartile-based segmentation

**Use Cases**:
1. Detect transient spikes between checkpoints
2. Validate checkpoint-based measurements
3. Measure peak RSS with high precision
4. Identify allocation burst patterns

---

## Results Location

**Directory**: `src/xxcrossTrainer/results/spike_detection/`

**Structure**:
```
results/spike_detection/
├── run_20260102_153000/
│   ├── 2m_rss.csv
│   ├── 2m_analysis.txt
│   ├── 2m_solver.log
│   ├── 4m_rss.csv
│   ├── 4m_analysis.txt
│   ├── 4m_solver.log
│   ├── 8m_rss.csv
│   ├── 8m_analysis.txt
│   └── 8m_solver.log
└── run_20260102_154500/
    └── ...
```

**Retention**: Results are preserved for documentation and analysis. Do not store in `/tmp/` or `temp/`.

---

## Integration with Documentation

**After running measurements**:

1. Review analysis reports in `results/spike_detection/run_*/`
2. Compare transient peaks vs checkpoint peaks
3. Update [peak_rss_optimization.md](../../docs/developer/Experiences/peak_rss_optimization.md) spike investigation section
4. Update [IMPLEMENTATION_PROGRESS.md](../../docs/developer/IMPLEMENTATION_PROGRESS.md)

**Documentation workflow**:
- If no significant transient spikes: Validate checkpoint methodology
- If transient spikes detected: Update peak RSS values and safety margins

---

## Reference

- **Methodology**: [MEMORY_MONITORING.md](../../docs/developer/MEMORY_MONITORING.md)
- **Spike investigation results**: [peak_rss_optimization.md](../../docs/developer/Experiences/peak_rss_optimization.md#spike-investigation-results-2026-01-02)
- **Implementation progress**: [IMPLEMENTATION_PROGRESS.md](../../docs/developer/IMPLEMENTATION_PROGRESS.md)
