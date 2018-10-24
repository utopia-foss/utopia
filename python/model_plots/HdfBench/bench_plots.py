"""Plots that visualize the benchmarks"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def times(dm: DataManager, *, uni: UniverseGroup, out_path: str, save_kwargs: dict=None,  **plot_kwargs):
    """Plots all benchmark times for the selected universe
    
    Args:
        dm (DataManager): The data manager that supplies the results
        uni (UniverseGroup): The universe data
        out_path (str): The output path for the plot
        save_kwargs (dict, optional): passed to the plt.savefig function
        **plot_kwargs: passed to plt.plot
    """
    # Get the times dataset for the selected universe
    times = uni['data/HdfBench/times']

    # Get the names of the coordinates and decode them to utf-8 strings
    bench_names = [n.decode('utf-8') for n in times.attrs['coords_benchmark']]

    # Call the plot function on each benchmark
    for i, bname in enumerate(bench_names):
        plt.plot(times[:,i], label=bname, **plot_kwargs)

    # Apply plot settings, labels, legend, ...
    plt.gca().set_yscale('log', nonposy='clip')

    plt.xlabel("Time step")
    plt.ylabel("Execution time per step [s]")
    plt.title("Utopia Hdf5 Benchmarking Results")

    plt.legend()

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
