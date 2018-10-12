"""CopyMe-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def state_mean(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = dm['uni'][uni]['data/CopyMe']

    # Get the shape of the data
    uni_cfg = dm['uni'][uni]['cfg']
    num_steps = uni_cfg['num_steps']
    grid_size = uni_cfg['CopyMe']['grid_size']

    # Extract the y data which is 'some_state' avaraged over all grid cells for every time step
    data = grp['some_state'].reshape(grid_size[0], grid_size[1], num_steps+1)
    y_data = [np.mean(d) for d in data]

    # Assemble the arguments
    args = [y_data]
    if fmt:
        args.append(fmt)

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
