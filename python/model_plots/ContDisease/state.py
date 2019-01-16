"""ContDisease-model specific plot function for spatial figures"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def plot_tree_density(dm: DataManager, *, 
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

    # Get the shape of the data
    uni_cfg = uni['cfg']
    num_steps = uni_cfg['num_steps']
    grid_size = uni_cfg['ContDisease']['grid_size']
    num_cells = grid_size[0] * grid_size[1]
    
    # Extract the data for the tree states and convert it into a 3d-array
    data = grp["density_tree"]

    # Assemble the arguments
    args = [data]
    if fmt:
        args.append(fmt)

    plt.figure()
    plt.title("Tree density")

    # Call the plot function
    plt.plot(*args, color='green', label='tree density', **plot_kwargs)
    plt.xlabel("time steps")
    plt.xlabel("tree density")
    plt.legend()

    save_and_close(out_path, save_kwargs=save_kwargs)
