"""ContDisease-model specific plot function for spatial figures"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close, get_times, colorline

# -----------------------------------------------------------------------------

def tree_density(dm: DataManager, *, 
                 out_path: str, 
                 uni: UniverseGroup, 
                 fmt: str=None, 
                 save_kwargs: dict=None, 
                 **plot_kwargs):
    """Calculates the the density of trees and perfoms a lineplot
    
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

    # Extract the data for the tree states and convert it into a 3d-array
    data = grp["densities/tree"]

    # Get the time steps
    times = get_times(uni)

    # Assemble the arguments
    args = [times, data]
    if fmt:
        args.append(fmt)

    plt.figure()

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    plt.xlabel("time steps")
    plt.ylabel("tree density")

    save_and_close(out_path, save_kwargs=save_kwargs)


def densities(dm: DataManager, *, 
              out_path: str, 
              uni: UniverseGroup, 
              save_kwargs: dict=None, 
              **plot_kwargs):
    """Extracts the densities and perfoms the lineplots
    
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
    
    # Get the time steps
    times = get_times(uni)
    
    # Expand the stone and source dataset to have the same value for each time step
    d_stone = np.full(len(times), d_stone) 
    d_source = np.full(len(times), d_source)

    # Create the figure and get the axes
    fig = plt.figure()
    ax = fig.gca()

    # Call the plot function
    ax.plot(times, d_empty, color='black', label='empty', **plot_kwargs)
    ax.plot(times, d_tree, color='green', label='tree', **plot_kwargs)
    ax.plot(times, d_infected, color='red', label='infected', **plot_kwargs)
    ax.plot(times, d_source, color='orange', label='source', **plot_kwargs)
    ax.plot(times, d_stone, color='gray', label='stone', **plot_kwargs)

    plt.xlabel("time steps")
    plt.ylabel("densities")

    plt.legend(loc='best')

    save_and_close(out_path, save_kwargs=save_kwargs)


def phase_diagram(dm: DataManager, *, 
                  out_path: str, 
                  uni: UniverseGroup, 
                  x: str,
                  y: str,
                  xlabel: str=None,
                  ylabel: str=None,
                  fmt: str=None, 
                  save_kwargs: dict=None, 
                  **plot_kwargs):
    """Calculates the the phase diagram of trees and infected trees
    
    Args:
        dm (DataManager): The data manager
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe data to use
        fmt (str, optional): the plt.plot format argument
        x (str): What to plot on the x_axis
        y (str): What to plot on the y_axis
        xlabel (str): The x-axis label
        ylabel (str): The y-axis label
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the tree states and convert it into a 3d-array
    d_tree = grp[x]
    d_infected = grp[y]

    # Call the plot function
    colorline(d_tree, d_infected)

    # Set the x label
    if xlabel is not None:
        plt.xlabel("{}".format(xlabel))
    else:
        plt.xlabel(x)

    # Set the y label
    if ylabel is not None:
        plt.ylabel("{}".format(ylabel))
    else:
        plt.ylabel(y)
        
    save_and_close(out_path, save_kwargs=save_kwargs)
