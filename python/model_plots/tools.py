"""Plotting tools that can be used from all model-specific plot functions"""

import matplotlib.pyplot as plt
import numpy as np

from utopya import UniverseGroup

# -----------------------------------------------------------------------------

def save_and_close(out_path: str, *, save_kwargs: dict=None) -> None:
    """Save and close the figure, passing the kwargs
    
    Args:
        out_path (str): The output path to save the figure to
        save_kwargs (dict, optional): The additional save_kwargs
    """
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()


def get_times(uni: UniverseGroup, include_t0: bool=True):
    """Get the time steps from the configuration of the universe
    
    Args:
        uni (UniverseGroup): The universe group
        include_t0 (bool): Include the initial time t=0 in the 

    Returns:
        np.array: The times array
    """
    # Retrive the necessary parameters from the configuration
    cfg = uni['cfg']
    num_steps = cfg['num_steps']
    write_every = cfg['write_every']

    # Calculate the times array which either includes the initial write or not.
    if include_t0:
        times = np.linspace(0, num_steps, num_steps//write_every + 1)
    else:
        times = np.linspace(write_every, num_steps, num_steps//write_every + 1)

    # Return a np.array object containing the discretized time steps
    return times