#!/usr/bin/env python3

import json
import os
import subprocess
from pathlib import Path

# Cache sizes to test (in MB)
CACHE_SIZES = [10, 50, 100, 500, 1000, 5000, 8000]

# Traces to test
TRACES = [
    ("trace.txt", "txt"),
    ("trace.csv", "csv"),
    ("trace.vscsi", "vscsi"),
    ("trace.oracleGeneral.bin", "oracleGeneralBin"),
    ("twitter_cluster52.csv", "csv"),
]

def run_simulation(trace_path, trace_type, cache_size, output_file):
    cmd = [
        "build/bin/cachesim",
        trace_path,
        trace_type,
        "LHD",
        f"{cache_size}MB",
        "-o", output_file
    ]
    
    # Add ignore-obj-size for txt traces
    if trace_type == "txt":
        cmd.append("--ignore-obj-size=true")
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running simulation for {trace_path} with {cache_size}MB cache:")
        print(f"STDOUT: {e.stdout}")
        print(f"STDERR: {e.stderr}")
        return False

def main():
    # Create results directory if it doesn't exist
    results_dir = Path("results")
    results_dir.mkdir(exist_ok=True)
    
    # Initialize or load existing results
    results_file = results_dir / "lhd_results.json"
    if results_file.exists():
        with open(results_file, 'r') as f:
            results = json.load(f)
    else:
        results = {}
    
    # Run simulations for each trace and cache size
    for trace_file, trace_type in TRACES:
        trace_path = f"../../data/{trace_file}"
        trace_name = Path(trace_file).stem
        
        if trace_name not in results:
            results[trace_name] = {}
        
        for cache_size in CACHE_SIZES:
            if str(cache_size) not in results[trace_name]:
                print(f"\nRunning LHD on {trace_name} with {cache_size}MB cache...")
                output_file = f"results/lhd_{trace_name}_{cache_size}MB.json"
                
                if run_simulation(trace_path, trace_type, cache_size, output_file):
                    # Read the results
                    try:
                        with open(output_file, 'r') as f:
                            result = f.read().strip()
                        results[trace_name][str(cache_size)] = result
                    except Exception as e:
                        print(f"Error reading results for {trace_name} with {cache_size}MB cache:")
                        print(f"Error: {e}")
                
                # Save results after each run
                with open(results_file, 'w') as f:
                    json.dump(results, f, indent=2)
                
                # Clean up temporary output file
                try:
                    os.remove(output_file)
                except:
                    pass

if __name__ == "__main__":
    main() 