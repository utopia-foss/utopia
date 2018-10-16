# `HdfBench` — Benchmarks for Utopia Data Writing

This "model" implements benchmarking capabilities for Utopia's `DataIO` library, focussing on Hdf5 output.
It is implemented as a regular model in order to use all the same structure and capabilities of the `Model` class and provide a benchmarking platform that is close to the real-world use case.

## Configuration
Benchmarks can be configured completely via the frontend; recompilation is not needed.
To that end, `HdfBench` supplies a set of `setup_funcs` and `bench_funcs`, which perform a setup or benchmarking operation, respectively, and then return the time it took for the relevant part of the function to execute.

Examples for the configuration of such benchmarks is shown below.  
(Note that if you are writing a `run` config, the examples below represent the content of the `parameter_space -> HdfBench` mapping.)

#### Getting started
```yaml
# List of benchmarks to carry out
benchmarks:
  - simple

# The corresponding configurations
simple:
  # Define the names of the setup and benchmark functions; mandatory!
  setup_func: setup_nd
  write_func: write_const

  # All the following arguments are available to _both_ functions
  write_shape: [100]
  const_val: 42.
```
This will result in the benchmark `simple` being carried out:
* It sets up a dataset of shape `{num_steps + 1, 100}`
* Each step, it writes vectors of length 100, filled with the value `42.`.

(Note that `num_steps` is defined not on this level of the configuration, but on the top-most level of the run configuration.)

#### Multiple benchmarks
One can also define multiple configuration and – using YAML anchors – let them share the other benchmarks' configuration:

```yaml
benchmarks:
  - simple
  - long_rows

simple: &simple
  setup_func: setup_nd
  write_func: write_const

  write_shape: [100]
  const_val: 42.

long_rows:
  <<: *simple
  write_shape: [1048576]  # == 1024^2
```

#### Advanced options


## Available setup and write functions

| Name | Description |
| ---- | ----------- |
| `setup_nd` | Sets up an $`n`$-dimensional dataset with shape `{num_steps + 1, write_shape}` |
| `write_const` | Writes `const_val` in shape `write_shape` to the dataset. |

_TODO: add more._

## Evaluation
### Data output structure
The `times` dataset holds the benchmarking times. Its rows correspond to the time step, the columns correspond to the configured benchmarks (in the same order).

For dynamic evaluation of benchmarks, the dataset attributes should be used:
* `dims`: gives names to dimensions
* `coords_benchmark`: which benchmark corresponds to which column of the dataset
* `initial_write`: whether the first benchmarked time (row 0) includes a write operation or _only_ the setup time of the dataset.

### Evaluation scripts
_TODO_
