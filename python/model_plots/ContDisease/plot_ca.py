"""ContDisease-model specific plot function for spatial figures"""

import os

import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def plot_ca_anim(dm: DataManager, *, out_path: str, uni: UniverseGroup, save_kwargs: dict=None, **plot_kwargs):
    """Plots the state for each time step of the 2d celluar automaton
    
    Args:
        dm (DataManager): The data manager
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe data to use
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Get some info about the data
    uni_cfg = uni['cfg']
    write_every = uni_cfg.get('write_every', 1)
    num_steps = uni_cfg['num_steps'] // write_every
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Extract the data for the tree states and convert it into a 3d-array
    data = np.reshape(grp["state"], (num_steps+1, grid_size[1], grid_size[0]))

    # Define colormap
    cmap = mpl.colors.ListedColormap(['#996633', '#006600', '#cc0000',
                                      '#ff6600', '#595959'])
    boundary = [0, 0.5, 1.5, 2.5, 3.5, 4.5]
    norm = mpl.colors.BoundaryNorm(boundary, cmap.N)

    # Retrieve the base path and extension
    out_path_base, out_path_ext = os.path.splitext(out_path)

    # Determine number of digits and create a path format string
    num_digs = len(str(num_steps))
    fname_fstr = "{idx:0{digs:}d}{ext:}"

    # Create a folder for the data
    os.makedirs(out_path_base, exist_ok=True)

    # Create a figure and write the initial state
    fig = plt.figure()
    img = plt.imshow(data[0], cmap, norm=norm, origin='lower', **plot_kwargs)

    plt.title("time step: 0")
    plt.savefig(os.path.join(out_path_base,
                             fname_fstr.format(idx=0, digs=num_digs,
                                               ext=out_path_ext)),
                **(save_kwargs if save_kwargs else {}))

    # Now plot all the other states
    for i in range(1, num_steps):
        img.set_data(data[i])

        plt.title("time step: {}".format(i * write_every))
        plt.savefig(os.path.join(out_path_base,
                                 fname_fstr.format(idx=i, digs=num_digs,
                                                   ext=out_path_ext)),
                    **(save_kwargs if save_kwargs else {}))

    plt.close(fig)
