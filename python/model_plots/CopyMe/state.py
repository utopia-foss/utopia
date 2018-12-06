"""CopyMe-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def state_mean( dm: DataManager, *, 
                uni: UniverseGroup, 
                out_path: str, 
                fmt: str=None, 
                save_kwargs: dict=None, 
                **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        out_path (str): Where to store the plot to
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/CopyMe']

    # Extract the y_data which is 'some_state' averaged over all grid cells 
    # for every time step
    data = grp['some_state']
    y_data = [np.mean(d) for d in data]

    # Assemble the arguments
    args = [y_data]
    if fmt:
        args.append(fmt)

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
