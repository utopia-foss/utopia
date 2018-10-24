"""Dummy-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def single_state(dm: DataManager, *, out_path: str, uni: UniverseGroup, step: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Plots the state for a single time step
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The selected universe data
        step (int): The time step to plot
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the state dataset
    state = uni['data/dummy/state']

    # Select the time step
    state_at_step = state[step, :]

    # Assemble arguments
    args = [state_at_step]
    if fmt:
        args.append(fmt)

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)


def state_mean(dm: DataManager, *, out_path: str, uni: UniverseGroup, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Plots the state mean over time.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The selected universe data
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the state dataset
    state = uni['data/dummy/state']

    # Calculate the mean by averaging over the columns
    y_data = np.mean(state, axis=1)

    # The x data is just the time steps
    x_data = list(range(state.shape[0]))

    # Assemble the arguments
    args = [x_data, y_data]
    if fmt:
        args.append(fmt)

    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
