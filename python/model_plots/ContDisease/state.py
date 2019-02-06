"""ContDisease-model specific plot function for spatial figures"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close, colorline

# -----------------------------------------------------------------------------

def tree_density(dm: DataManager, *, 
                 out_path: str, 
                 uni: UniverseGroup, 
                 fmt: str=None, 
                 save_kwargs: dict=None, 
                 **plot_kwargs):
    """Plots the density of trees and perfoms a lineplot
    
    Args:
        dm (DataManager): The data manager
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe data to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the tree states
    data = grp["densities/tree"]

    # Get the time steps array for this universe
    times = uni.get_times_array()

    # Call the plot function
    plt.plot(times, data, **plot_kwargs)

    plt.title("Tree density")
    plt.xlabel("Time [Steps]")
    plt.ylabel("Density")

    plt.xlim(0, max(times))
    plt.ylim(0, 1)

    save_and_close(out_path, save_kwargs=save_kwargs)


def densities(dm: DataManager, *, 
              out_path: str, 
              uni: UniverseGroup, 
              save_kwargs: dict=None, 
              **plot_kwargs):
    """Plots the densities of all cell states
    
    Args:
        dm (DataManager): The data manager
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe data to use
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the tree states and convert it into a 3d-array
    d_empty = grp["densities/empty"]
    d_tree = grp["densities/tree"]
    d_infected = grp["densities/infected"]
    d_source = grp["densities/source"]
    d_stone = grp["densities/stone"]
    
    # Get the time steps array for the given universe
    times = uni.get_times_array()
    
    # Expand the stone and source arrays (both constant) to be of valid length
    d_stone = np.full(len(times), d_stone) 
    d_source = np.full(len(times), d_source)

    # Call the plot function
    plt.plot(times, d_empty, color='black', label='empty', **plot_kwargs)
    plt.plot(times, d_tree, color='green', label='tree', **plot_kwargs)
    plt.plot(times, d_infected, color='red', label='infected', **plot_kwargs)
    plt.plot(times, d_source, color='orange', label='source', **plot_kwargs)
    plt.plot(times, d_stone, color='gray', label='stone', **plot_kwargs)

    plt.title("State Densities")
    plt.xlabel("Time [Steps]")
    plt.ylabel("Density")

    plt.xlim(0, max(times))
    plt.ylim(0, 1)

    plt.legend(loc='best')

    save_and_close(out_path, save_kwargs=save_kwargs)


def phase_diagram(dm: DataManager, *, 
                  out_path: str, 
                  uni: UniverseGroup, 
                  x: str='tree',
                  y: str='infected',
                  xlims: tuple=None,
                  ylims: tuple=None,
                  xlabel: str=None,
                  ylabel: str=None,
                  fmt: str=None, 
                  save_kwargs: dict=None, 
                  **lc_kwargs):
    """Plots the the phase diagram of tree and infected tree densities
    
    Args:
        dm (DataManager): The data manager
        out_path (str): Where to save the plot to
        uni (UniverseGroup): The universe data to use
        x (str, optional): What to plot on the x-axis
        y (str, optional): What to plot on the y-axis
        xlims (tuple, optional): The limits of the x-axis
        ylims (tuple, optional): The limits of the y-axis
        xlabel (str, optional): The x-axis label
        ylabel (str, optional): The y-axis label
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **lc_kwargs: Passed on to LineCollection.__init__
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the x and y coordinates
    d_x = grp[x]
    d_y = grp[y]

    # Call the plot function
    colorline(d_x, d_y, **lc_kwargs)

    # Set title and labels
    plt.title("Phase Diagram")
    plt.xlabel("{}".format(xlabel) if xlabel is not None else x)
    plt.ylabel("{}".format(ylabel) if ylabel is not None else y)

    # Set limits, if given
    if xlims:
        plt.xlim(*xlims)
    
    if ylims:
        plt.ylim(*ylims)

    save_and_close(out_path, save_kwargs=save_kwargs)
