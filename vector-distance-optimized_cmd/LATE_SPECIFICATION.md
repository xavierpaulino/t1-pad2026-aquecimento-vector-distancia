# Late specification update

Evaluator-facing invocation:

```bash
./build/vector_distance T
```

Example:

```bash
./build/vector_distance 1000
```

The only standard-output line is:

```text
xavier, 1000, <time_ms>
```

`T` is the vector size. `N` remains fixed at 16384. The reported value is the time, in milliseconds, of one execution of the optimized v4 distance kernel after five unmeasured warm-up executions. Memory allocation, initialization, random data generation, checksum calculation, file I/O and formatting are outside the measured region.

The legacy experimental interface (`--n`, `--t`, `--variant`, CSV output, self-test and timer probe) is preserved for reproducibility and for the v3/v4 comparison scripts.
