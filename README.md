<img width="300" src="doc/diagram/logo.svg" alt="sieve Logo" />

<hr/>

# ADAP-repo: Adaptive Climb and Dynamic Adaptive Climb Cache Algorithms

This repository provides implementations of the **Adaptive Climb** and **Dynamic Adaptive Climb** cache eviction algorithms, integrated into the libCacheSim simulation framework. These algorithms are designed for efficient cache management and are placed alongside other state-of-the-art eviction algorithms in the simulator.

## Key Features
- **Adaptive Climb**: An adaptive eviction algorithm that dynamically adjusts to workload patterns.
- **Dynamic Adaptive Climb**: An enhanced version with further dynamic tuning for improved performance.
- Both algorithms are implemented in C and can be used with the libCacheSim simulator for benchmarking and research.

## Repo Structure
- `libCacheSim/`: Contains the simulator and all eviction algorithms, including Adaptive Climb and Dynamic Adaptive Climb in `libCacheSim/libCacheSim/cache/eviction/`.
- `mydata/`, `data/`: Example traces and data for simulation.
- `result/`, `results/`: Output directories for simulation results.
- `doc/`: Documentation and diagrams.
- `scripts/`: Helper scripts for running simulations and plotting results.

## Usage

### Install libCacheSim
```bash
cd libCacheSim/scripts && bash install_dependency.sh && bash install_libcachesim.sh;
```

After building and installing libCacheSim, `cachesim` should be in the `_build/bin/` directory. You can use `cachesim` to run cache simulations with Adaptive Climb and Dynamic Adaptive Climb:
```bash
# ./cachesim DATAPATH TRACE_FORMAT EVICTION_ALGO CACHE_SIZE [OPTION...]
# Example: Run Adaptive Climb
./_build/bin/cachesim data/trace.oracleGeneral.bin oracleGeneral adaptiveclimb 1gb
# Example: Run Dynamic Adaptive Climb
./_build/bin/cachesim data/trace.oracleGeneral.bin oracleGeneral dynamicadaptiveclimb 1gb
```
Results are recorded at `result/TRACE_NAME`.

### Quick Tryout with Zipfian Data
You can test the miss ratio for varied algorithms, including Adaptive Climb and Dynamic Adaptive Climb:
```bash
python3 libCacheSim/scripts/plot_mrc_size.py --tracepath mydata/zipf/zipf_1.0 --trace-format txt --algos=fifo,lru,adaptiveclimb,dynamicadaptiveclimb
```

## Algorithms Location
- **Adaptive Climb**: `libCacheSim/libCacheSim/cache/eviction/AdaptiveClimb.c`
- **Dynamic Adaptive Climb**: `libCacheSim/libCacheSim/cache/eviction/DynamicAdaptiveClimb.c`

## References
If you use these algorithms in your research, please cite the relevant papers (add your own references here).

---

For more details on extending libCacheSim or adding new algorithms, see `libCacheSim/doc/advanced_lib_extend.md`.