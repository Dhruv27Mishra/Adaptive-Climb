import os
import re
import csv

RESULTS_DIR = os.path.join(os.path.dirname(__file__), 'results_ilru')
SUMMARY_CSV = os.path.join(os.path.dirname(__file__), 'ilru_summary.csv')

summary = []

for fname in os.listdir(RESULTS_DIR):
    if not fname.endswith('.cachesim'):
        continue
    # Use regex to extract trace name and cache size
    mfile = re.match(r'(.+)_ilru_(\d+)\.cachesim', fname)
    if not mfile:
        continue
    trace = mfile.group(1)
    size = int(mfile.group(2))
    with open(os.path.join(RESULTS_DIR, fname)) as f:
        for line in f:
            # Example: ilru cache size   10485760,           666667 req, miss ratio 0.1234, throughput 25.67 MQPS
            m = re.search(r'ilru cache size\s+(\d+),\s+\d+ req, miss ratio ([0-9.]+), throughput ([0-9.]+) MQPS', line)
            if m:
                cache_size = int(m.group(1))
                miss_ratio = float(m.group(2))
                throughput = float(m.group(3))
                summary.append([trace, cache_size, miss_ratio, throughput])
                break

# Write summary CSV
with open(SUMMARY_CSV, 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['Trace', 'Cache Size (bytes)', 'Miss Ratio', 'Throughput (MQPS)'])
    for row in summary:
        writer.writerow(row)

# Print summary
print('Trace,Cache Size (bytes),Miss Ratio,Throughput (MQPS)')
for row in summary:
    print(','.join(map(str, row))) 