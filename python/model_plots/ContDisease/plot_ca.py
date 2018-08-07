"""ContDisease-model specific plot function for spatial figures"""


import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------


def plot_forest(dm: DataManager, *, out_path: str, file_format: str='png', uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Plots the forest state for each time step of the two dimensional celluar automaton

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

    # Extract the data for the tree states and convert it into a 2d-array

    data_1d = grp["treestate"]
    data = np.reshape(data_1d, (num_steps+1, grid_size[0], grid_size[1]))


    # Define colormap

    cmp = mpl.colors.ListedColormap(['#996633','#006600','#cc0000', '#ff6600'])
    boundary = [0,0.5,1.5,2.5,3.5]
    norm = mpl.colors.BoundaryNorm(boundary, cmp.N)

    # Plot data and save it

    out_path_base = os.path.splitext(out_path)[0]
    out_path_ext = os.path.splitext(file_format)[1]


    for i in range(num_steps):
        if (i == 0):
            plt.figure()
            img = plt.imshow(data[i], cmp, norm = norm, origin = 'lower', **plot_kwargs)
        else:
            img.set_data(data[i])
        plt.title("timestep: {}".format(i))
        #FIXME Include save_kwargs in plt.savefig
        plt.savefig(out_path_base + "{0:0>4}".format(i) + out_path_ext)
