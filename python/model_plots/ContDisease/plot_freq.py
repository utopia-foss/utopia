"""ContDisease-model specific plot function for spatial figures"""


import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def plot_frequency(dm: DataManager, *, out_path: str, file_format: str='png', uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Calculates the the density of trees and perfoms a lineplot

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = dm['uni'][str(uni)]['data/ContDisease']

    # Get the shape of the data
    uni_cfg = dm['uni'][str(uni)]['cfg']
    num_steps = uni_cfg['num_steps']
    grid_size = uni_cfg['ContDisease']['grid_size']
    num_cells = grid_size[0] * grid_size[1]
    # Extract the data for the tree states and convert it into a 3d-array

    data_ = grp["state"]

    # Calculates for each time step the ratio of trees to the grid size

    ratio_tree = []
    timesteps = list(range(num_steps))
    for i in range(num_steps):
        ratio_tree.append( np.sum(data_[i] == 1) / num_cells )

    plt.figure()
    plt.title("tree density")

    plt.plot(timesteps,ratio_tree,  color = 'green', label = 'tree', **plot_kwargs)

    plt.xlabel("timesteps")
    plt.legend()
    save_and_close(out_path, save_kwargs=save_kwargs)
