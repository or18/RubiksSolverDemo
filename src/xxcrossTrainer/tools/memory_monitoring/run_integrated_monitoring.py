#!/usr/bin/env python3
"""
Integrated monitoring system
Starts solver and monitors memory simultaneously
Based on stable-20251230 backup scripts
"""

import subprocess
import sys
import os
import time
from pathlib import Path
from monitor_memory import MemoryMonitor

def run_with_monitoring(solver_path, memory_limit_mb, bucket_model, output_dir):
    """
    Launch solver and monitor memory usage
    
    Returns:
        dict: Summary statistics including peak RSS
    """
    
    # Prepare environment
    env = os.environ.copy()
    env['MEMORY_LIMIT_MB'] = str(memory_limit_mb)
    env['ENABLE_CUSTOM_BUCKETS'] = '1'
    env['BUCKET_MODEL'] = bucket_model
    
    # Output files
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    csv_file = output_dir / "rss_trace.csv"
    log_file = output_dir / "solver.log"
    config_file = output_dir / "config.txt"
    
    print(f"=== Integrated Memory Monitoring ===")
    print(f"Solver: {solver_path}")
    print(f"Memory limit: {memory_limit_mb} MB")
    print(f"Bucket model: {bucket_model}")
    print(f"Output: {output_dir}")
    print("")
    
    # Start solver
    print(f"Launching solver...", flush=True)
    with open(log_file, 'w') as log:
        process = subprocess.Popen(
            [solver_path],
            env=env,
            stdout=log,
            stderr=subprocess.STDOUT,
            cwd=Path(solver_path).parent
        )
    
    pid = process.pid
    print(f"Solver PID: {pid}", flush=True)
    print("", flush=True)
    
    # Start monitoring immediately
    monitor = MemoryMonitor(pid, sample_interval_ms=10, output_csv=str(csv_file))
    
    try:
        summary = monitor.monitor(timeout_seconds=180)
    except KeyboardInterrupt:
        print("\nMonitoring interrupted by user", flush=True)
        process.terminate()
        process.wait(timeout=5)
        raise
    
    # Wait for solver to finish
    exit_code = process.wait()
    print(f"\nSolver exited with code: {exit_code}", flush=True)
    
    # Save configuration
    with open(config_file, 'w') as f:
        f.write(f"Memory Limit: {memory_limit_mb} MB\n")
        f.write(f"Bucket Model: {bucket_model}\n")
        f.write(f"Solver PID: {pid}\n")
        f.write(f"Exit Code: {exit_code}\n")
        f.write(f"Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"\n=== Peak Memory ===\n")
        f.write(f"VmRSS: {summary['peak_vmrss_mb']:.2f} MB\n")
        f.write(f"VmSize: {summary['peak_vmsize_mb']:.2f} MB\n")
        f.write(f"VmData: {summary['peak_vmdata_mb']:.2f} MB\n")
        f.write(f"Samples: {summary['samples']}\n")
    
    print(f"\n✓ Results saved to: {output_dir}")
    
    return summary

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 run_integrated_monitoring.py <solver_path> <memory_limit_mb> <bucket_model> [output_dir]")
        print("Example: python3 run_integrated_monitoring.py ./solver_dev 1600 8M/8M/8M ./measurements")
        sys.exit(1)
    
    solver_path = sys.argv[1]
    memory_limit = int(sys.argv[2])
    bucket_model = sys.argv[3]
    output_dir = sys.argv[4] if len(sys.argv) > 4 else f"measurements_{time.strftime('%Y%m%d_%H%M%S')}"
    
    if not os.path.exists(solver_path):
        print(f"Error: Solver not found: {solver_path}")
        sys.exit(1)
    
    summary = run_with_monitoring(solver_path, memory_limit, bucket_model, output_dir)
    
    print(f"\n=== Final Summary ===")
    print(f"Peak VmRSS: {summary['peak_vmrss_mb']:.2f} MB")
    print(f"Peak VmSize: {summary['peak_vmsize_mb']:.2f} MB")
    print(f"Peak VmData: {summary['peak_vmdata_mb']:.2f} MB")
