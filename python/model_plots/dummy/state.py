"""Dummy-model specific plot function for the state"""

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
    grp = dm['uni'][str(uni)]['data/dummy']

    # Extract the x and y data
    # NOTE need this complicated approach due to the way the data is saved
    data = [(int(name[5:]), np.mean(dset)) for name, dset in grp.items()]
    data.sort()
    x_data = [p[0] for p in data]
    y_data = [p[1] for p in data]

    # Assemble the arguments
    args = [x_data, y_data]
    if fmt:
        args.append(fmt)

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
