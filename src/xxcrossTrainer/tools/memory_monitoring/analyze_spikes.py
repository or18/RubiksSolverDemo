#!/usr/bin/env python3
"""
Analyze memory spikes from high-frequency CSV monitoring
Detects rapid memory increases (potential spikes)
Based on stable-20251230 spike detection system
"""

import csv
import sys

def detect_spikes(csv_file, threshold_mb=20, window_ms=1000):
    """
    Detect memory allocation spikes
    
    Args:
        csv_file: Path to CSV with time_s,vmrss_kb,vmsize_kb,vmdata_kb
        threshold_mb: Minimum increase to consider a spike (default: 20 MB)
        window_ms: Time window for spike detection (default: 1000 ms)
    """
    
    spikes = []
    prev_time = 0
    prev_rss = 0
    
    with open(csv_file) as f:
        reader = csv.DictReader(f)
        for row in reader:
            time_s = float(row['time_s'])
            rss_mb = float(row['vmrss_kb']) / 1024
            
            if prev_time > 0:
                time_diff_ms = (time_s - prev_time) * 1000
                rss_diff_mb = rss_mb - prev_rss
                
                # Detect spike: significant increase within time window
                if rss_diff_mb > threshold_mb and time_diff_ms < window_ms:
                    rate_mb_s = rss_diff_mb / ((time_diff_ms / 1000) if time_diff_ms > 0 else 1)
                    spikes.append({
                        'time': time_s,
                        'rss_mb': rss_mb,
                        'delta_mb': rss_diff_mb,
                        'duration_ms': time_diff_ms,
                        'rate_mb_s': rate_mb_s
                    })
            
            prev_time = time_s
            prev_rss = rss_mb
    
    return spikes

def analyze_csv(csv_file):
    """Comprehensive analysis of memory trace"""
    
    data = []
    with open(csv_file) as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append({
                'time': float(row['time_s']),
                'rss_mb': float(row['vmrss_kb']) / 1024,
                'vmsize_mb': float(row['vmsize_kb']) / 1024,
                'vmdata_mb': float(row['vmdata_kb']) / 1024
            })
    
    if not data:
        print("No data found")
        return
    
    # Basic statistics
    peak_rss = max(row['rss_mb'] for row in data)
    peak_vmsize = max(row['vmsize_mb'] for row in data)
    peak_vmdata = max(row['vmdata_mb'] for row in data)
    peak_time = [row['time'] for row in data if row['rss_mb'] == peak_rss][0]
    final_rss = data[-1]['rss_mb']
    duration = data[-1]['time']
    
    print(f"=== Memory Analysis: {csv_file} ===")
    print(f"Duration: {duration:.2f}s")
    print(f"Samples: {len(data)}")
    print(f"Peak VmRSS: {peak_rss:.2f} MB (at t={peak_time:.2f}s)")
    print(f"Peak VmSize: {peak_vmsize:.2f} MB")
    print(f"Peak VmData: {peak_vmdata:.2f} MB")
    print(f"Final RSS: {final_rss:.2f} MB")
    if final_rss > 0:
        print(f"Peak/Final ratio: {peak_rss/final_rss:.2f}x")
    else:
        print(f"Peak/Final ratio: N/A (process terminated)")
    print()
    
    # Detect spikes (>20 MB/s allocation rate)
    spikes = detect_spikes(csv_file, threshold_mb=20, window_ms=1000)
    
    print(f"Memory Spikes (>20 MB within 1s): {len(spikes)}")
    if spikes:
        print("Top 10 spikes by magnitude:")
        print(f"{'Time (s)':<12} {'RSS (MB)':<12} {'Increase (MB)':<15} {'Duration (ms)':<15} {'Rate (MB/s)':<15}")
        print("-" * 80)
        top_spikes = sorted(spikes, key=lambda x: x['delta_mb'], reverse=True)[:10]
        for spike in top_spikes:
            print(f"{spike['time']:<12.2f} {spike['rss_mb']:<12.1f} {spike['delta_mb']:<15.1f} "
                  f"{spike['duration_ms']:<15.1f} {spike['rate_mb_s']:<15.1f}")
    else:
        print("  No major spikes detected (all allocation rates < 20 MB/s)")
    print()
    
    # Timeline segments (quartiles)
    q1_idx = len(data) // 4
    q2_idx = len(data) // 2
    q3_idx = 3 * len(data) // 4
    
    print("Timeline segments (quartiles):")
    print(f"  Q1 (0-25%):   t={data[0]['time']:.1f}-{data[q1_idx]['time']:.1f}s, "
          f"RSS: {data[0]['rss_mb']:.1f} → {data[q1_idx]['rss_mb']:.1f} MB")
    print(f"  Q2 (25-50%):  t={data[q1_idx]['time']:.1f}-{data[q2_idx]['time']:.1f}s, "
          f"RSS: {data[q1_idx]['rss_mb']:.1f} → {data[q2_idx]['rss_mb']:.1f} MB")
    print(f"  Q3 (50-75%):  t={data[q2_idx]['time']:.1f}-{data[q3_idx]['time']:.1f}s, "
          f"RSS: {data[q2_idx]['rss_mb']:.1f} → {data[q3_idx]['rss_mb']:.1f} MB")
    print(f"  Q4 (75-100%): t={data[q3_idx]['time']:.1f}-{data[-1]['time']:.1f}s, "
          f"RSS: {data[q3_idx]['rss_mb']:.1f} → {data[-1]['rss_mb']:.1f} MB")
    print()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_spikes.py <csv_file>")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    analyze_csv(csv_file)
