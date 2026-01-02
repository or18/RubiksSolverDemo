#!/usr/bin/env python3
"""
High-Frequency Memory Monitor using /proc filesystem
Detects transient memory spikes during hash table operations
Based on stable-20251230 monitoring system
"""

import time
import sys
import os
import csv
from pathlib import Path

class MemoryMonitor:
    """High-frequency memory monitor using /proc filesystem"""
    
    def __init__(self, pid, sample_interval_ms=10, output_csv="memory.csv"):
        self.pid = pid
        self.sample_interval = sample_interval_ms / 1000.0  # Convert to seconds
        self.output_csv = output_csv
        
        self.data = []
        self.start_time = None
        self.max_vmrss = 0
        self.max_vmsize = 0
        self.max_vmdata = 0
        
    def read_proc_status(self):
        """Read memory stats from /proc/PID/status"""
        try:
            with open(f"/proc/{self.pid}/status", "r") as f:
                status = f.read()
            
            vmrss = 0
            vmsize = 0
            vmdata = 0
            
            for line in status.split("\n"):
                if line.startswith("VmRSS:"):
                    vmrss = int(line.split()[1])  # KB
                elif line.startswith("VmSize:"):
                    vmsize = int(line.split()[1])  # KB
                elif line.startswith("VmData:"):
                    vmdata = int(line.split()[1])  # KB
            
            return vmrss, vmsize, vmdata
        except (FileNotFoundError, ProcessLookupError):
            return None, None, None
    
    def monitor(self, timeout_seconds=180):
        """Monitor process memory usage"""
        self.start_time = time.time()
        
        print(f"Monitoring PID {self.pid} (interval: {self.sample_interval*1000:.0f}ms)", flush=True)
        print(f"Timeout: {timeout_seconds}s", flush=True)
        print("", flush=True)
        
        # Open CSV file for writing
        with open(self.output_csv, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['time_s', 'vmrss_kb', 'vmsize_kb', 'vmdata_kb'])
            
            while True:
                # Check if process exists
                try:
                    os.kill(self.pid, 0)  # Signal 0 checks existence without killing
                except OSError:
                    print(f"Process {self.pid} terminated", flush=True)
                    break
                
                # Read memory
                vmrss, vmsize, vmdata = self.read_proc_status()
                if vmrss is None:
                    break
                
                elapsed = time.time() - self.start_time
                
                # Write to CSV immediately (for real-time analysis)
                writer.writerow([f'{elapsed:.3f}', vmrss, vmsize, vmdata])
                f.flush()  # Ensure data is written
                
                # Store in memory for later analysis
                self.data.append({
                    'time_s': elapsed,
                    'vmrss_kb': vmrss,
                    'vmsize_kb': vmsize,
                    'vmdata_kb': vmdata
                })
                
                # Track peaks and report
                if vmrss > self.max_vmrss:
                    self.max_vmrss = vmrss
                    print(f"[{elapsed:6.2f}s] Peak VmRSS: {vmrss/1024:.1f} MB "
                          f"(VmSize: {vmsize/1024:.0f} MB, VmData: {vmdata/1024:.0f} MB)", flush=True)
                
                if vmsize > self.max_vmsize:
                    self.max_vmsize = vmsize
                
                if vmdata > self.max_vmdata:
                    self.max_vmdata = vmdata
                
                # Timeout check
                if elapsed > timeout_seconds:
                    print(f"Timeout reached ({timeout_seconds}s)", flush=True)
                    break
                
                time.sleep(self.sample_interval)
        
        return self.summarize()
    
    def summarize(self):
        """Generate summary statistics"""
        print(f"\n=== Summary ===", flush=True)
        print(f"Peak VmRSS: {self.max_vmrss/1024:.2f} MB", flush=True)
        print(f"Peak VmSize: {self.max_vmsize/1024:.2f} MB", flush=True)
        print(f"Peak VmData: {self.max_vmdata/1024:.2f} MB", flush=True)
        print(f"Samples: {len(self.data)}", flush=True)
        print(f"Duration: {time.time() - self.start_time:.1f}s", flush=True)
        
        return {
            'peak_vmrss_mb': self.max_vmrss / 1024,
            'peak_vmsize_mb': self.max_vmsize / 1024,
            'peak_vmdata_mb': self.max_vmdata / 1024,
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
