import os
import subprocess

# Path to cachesim binary
CACHESIM_BIN = os.path.abspath(os.path.join(os.path.dirname(__file__), 'build/bin/cachesim'))

# Directories to search for traces
TRACE_DIRS = [
    os.path.abspath(os.path.join(os.path.dirname(__file__), 'data')),
    os.path.abspath(os.path.join(os.path.dirname(__file__), 'Traces')),
]

# Output directory for results
RESULTS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), 'results_ilru'))
os.makedirs(RESULTS_DIR, exist_ok=True)

# Standard cache sizes (in bytes)
CACHE_SIZES = [10*1024*1024, 50*1024*1024, 100*1024*1024, 200*1024*1024, 500*1024*1024]

# Find all trace files
trace_files = []
for trace_dir in TRACE_DIRS:
    if not os.path.isdir(trace_dir):
        continue
    for fname in os.listdir(trace_dir):
        if fname.endswith('.csv') or fname.endswith('.oracleGeneral') or fname.endswith('.bin') or fname.endswith('.txt'):
            trace_files.append(os.path.join(trace_dir, fname))

# Run iLRU on each trace file and cache size
for trace in trace_files:
    trace_name = os.path.basename(trace)
    file_type = trace_name.split('.')[-1]
    for size in CACHE_SIZES:
        out_file = os.path.join(RESULTS_DIR, f'{trace_name}_ilru_{size}.cachesim')
        cmd = [
            CACHESIM_BIN,
            trace,
            file_type,
            'ilru',
            str(size),
            '--num-thread=1',
            '-t', 'time-col=1,obj-id-col=2,obj-size-col=3,has-header=false,obj-id-is-num=true'
        ]
        print(f'Running: {" ".join(cmd)}')
        with open(out_file, 'w') as fout:
            subprocess.run(cmd, stdout=fout, stderr=subprocess.STDOUT) 