# AdaptiveClimb and DynamicAdaptiveClimb: Research Code Repository

---

## Abstract
This repository provides open-source implementations of the **Adaptive Climb** and **Dynamic Adaptive Climb** cache eviction algorithms, designed for efficient and adaptive cache management. Integrated into the libCacheSim simulation framework, these algorithms are intended for reproducible research, benchmarking, and further development by the systems and caching research community.

## Motivation and Contributions
Modern web-scale systems require efficient cache eviction policies to maximize hit rates and minimize latency. Adaptive Climb and Dynamic Adaptive Climb are novel algorithms that dynamically adjust to workload patterns, outperforming traditional policies in skewed and dynamic environments. This repository enables:
- Reproducible evaluation of Adaptive Climb and Dynamic Adaptive Climb.
- Direct comparison with state-of-the-art eviction algorithms in libCacheSim.
- Easy integration and extension for new research.

## Repository Structure
- `libCacheSim/` — Simulator and all eviction algorithms, including Adaptive Climb (`AdaptiveClimb.c`) and Dynamic Adaptive Climb (`DynamicAdaptiveClimb.c`) in `libCacheSim/libCacheSim/cache/eviction/`.
- `mydata/`, `data/` — Example traces and data for simulation.
- `doc/` — Documentation and diagrams.
- `scripts/` — Helper scripts for running simulations and plotting results.

## Installation and Usage
### Install libCacheSim
```bash
cd libCacheSim/scripts && bash install_dependency.sh && bash install_libcachesim.sh
```

### Running Simulations
After building and installing libCacheSim, `cachesim` will be in the `_build/bin/` directory. Example usage:
```bash
# Run Adaptive Climb
./_build/bin/cachesim data/trace.oracleGeneral.bin oracleGeneral adaptiveclimb 1gb
# Run Dynamic Adaptive Climb
./_build/bin/cachesim data/trace.oracleGeneral.bin oracleGeneral dynamicadaptiveclimb 1gb
```
Results are recorded at `result/TRACE_NAME`.

### Quick Tryout with Zipfian Data
```bash
python3 libCacheSim/scripts/plot_mrc_size.py --tracepath mydata/zipf/zipf_1.0 --trace-format txt --algos=fifo,lru,adaptiveclimb,dynamicadaptiveclimb
```

## Datasets and Traces
The following traces are recommended for benchmarking (as used in recent cache research):

| Trace         | Year | Type     | Original Release | OracleGeneral Format |
|---------------|------|----------|------------------|---------------------|
| Tencent Photo | 2018 | object   | [link](http://iotta.snia.org/traces/parallel?only=27476) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/tencentPhoto/) |
| Wiki CDN      | 2019 | object   | [link](https://wikitech.wikimedia.org/wiki/Analytics/Data_Lake/Traffic/Caching) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/wiki/) |
| Tencent CBS   | 2020 | block    | [link](http://iotta.snia.org/traces/parallel?only=27917) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/tencentBlock/) |
| Alibaba Block | 2020 | block    | [link](https://github.com/alibaba/block-traces) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/alibabaBlock/) |
| Twitter KV    | 2020 | key-value| [link](https://github.com/twitter/cache-traces) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/twitter/) |
| Meta KV       | 2022 | key-value| [link](https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_FB_HW_eval/#list-of-traces) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/metaKV/) |
| Meta CDN      | 2023 | object   | [link](https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_FB_HW_eval/#list-of-traces) | [link](https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/cacheDatasets/metaCDN/) |

> **Note:** The provided traces in `/data/` are small samples for testing only. Use the full traces above for evaluation.

## Prototypes and Integrations
Adaptive Climb and Dynamic Adaptive Climb can be integrated into various cache libraries. For reference, similar algorithms have been integrated into:
- [groupcache](https://github.com/cacheMon/groupcache) (Golang)
- [mnemonist](https://github.com/cacheMon/mnemonist) (Javascript)
- [lru-rs](https://github.com/cacheMon/lru-rs) (Rust)
- [lru-dict](https://github.com/cacheMon/lru-dict) (Python + C)
- [cachelib](https://github.com/Thesys-lab/cachelib-sosp23) (C++)

## Extending and Contributing
- To add new algorithms or trace types, see `libCacheSim/doc/advanced_lib_extend.md`.
- Contributions are welcome. Please open an issue or pull request for discussion.

## How to Cite
If you use Adaptive Climb or Dynamic Adaptive Climb in your research, please cite this repository and the relevant papers:

```bibtex
@misc{adaptiveclimb2024,
  title={Adaptive Climb and Dynamic Adaptive Climb: Efficient Adaptive Cache Eviction},
  author={Your Name and Collaborators},
  howpublished={\url{https://github.com/YOUR_GITHUB/ADAP-repo}},
  year={2024}
}
```


## License
See [LICENSE](LICENSE) for details.

## Contact and Questions
For questions, please open an issue or contact the maintainers via GitHub.


---
