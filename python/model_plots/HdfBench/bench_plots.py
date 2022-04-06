"""Plots that visualize the benchmarks"""

import logging

import numpy as np
import matplotlib.pyplot as plt

from utopya.eval import DataManager, UniverseGroup

from ..tools import save_and_close

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------
# Helper functions

def _plot_times(*, times, bench_names: list, out_path: str, save_kwargs: dict=None, std=None, num_seeds: int=None, **plot_kwargs):
    """Performs the actual plot of the figure
    
    Args:
        times (np.ndarray-like): The times to plot
        bench_names (list): Names of the benchmark columns
        out_path (str): The output path for the plot
        save_kwargs (dict, optional): passed to the plt.savefig function
        std (None, optional): If given, uses these as basis for errorbar plot
        num_seeds (int, optional): Information used in the title
        **plot_kwargs: passed to plt.plot
    
    """
    # Call the plot function on each benchmark
    for i, bname in enumerate(bench_names):
        if std is None:
            plt.plot(times[:,i], label=bname, **plot_kwargs)
        else:
            plt.errorbar(x=range(times.shape[0]),
                         y=times[:,i], yerr=std[:, i],
                         label=bname, **plot_kwargs)

    # Apply plot settings, labels, legend, ...
    plt.gca().set_yscale('log', nonpositive='clip')

    plt.xlabel("Time step")
    plt.ylabel("Execution time per step [s]")

    title = "Hdf5 Benchmark Results"
    if std is not None:
        title += " (mean ± std of {} runs)".format(num_seeds)

    plt.title(title)

    plt.legend()

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)


# -----------------------------------------------------------------------------

def times(dm: DataManager, *, uni: UniverseGroup, **kwargs):
    """Plots all benchmark times for the selected universe
    
    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data
    """
    # Get the times dataset for the selected universe
    times = uni['data/HdfBench/times']

    # Use helper function for actual plot
    _plot_times(times=times, bench_names=times.coords['benchmark'],
                **kwargs)


# -----------------------------------------------------------------------------

def mean_times(dm: DataManager, *, mv_data,
               allow_failure: bool=True, **kwargs):
    """Plots the benchmark times, but averages over the seed dimension of the
    data.
    
    Args:
        dm (DataManager): The data manager
        mv_data (xr.Dataset): The extracted multidimensional dataset
        out_path (str): The output path for the plot
        save_kwargs (dict, optional): passed to the plt.savefig function
        **plot_kwargs: passed to plt.plot
    """
    if 'seed' not in mv_data.dims:
        if not allow_failure:
            raise ValueError("No 'seed' dimension available to calculate the "
                             "mean over!")
        
        # Just warn ...
        log.warn("No 'seed' dimension available to calculate the mean over! "
                 "Skipping this plot ...")
        return

    # Get the times DataArray from the dataset
    times = mv_data['times']

    times_mean = times.mean('seed')
    times_std = times.std('seed')

    # Pass on to plot function
    _plot_times(times=times_mean, std=times_std,
                bench_names=times.coords['benchmark'],
                num_seeds=times['seed'].shape[0],
                **kwargs)
