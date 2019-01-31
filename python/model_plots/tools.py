"""Plotting tools that can be used from all model-specific plot functions"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
from matplotlib.colors import ListedColormap, BoundaryNorm

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


def get_times(uni: UniverseGroup, *, include_t0: bool=True):
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

    # Calculate the times array which either includes the initial write or not
    # and return it.
    if include_t0:
        return np.linspace(0, num_steps, num_steps//write_every + 1)
    else:
        return np.linspace(write_every, num_steps, num_steps//write_every + 1)


def colorline(x, y, *, 
              ax=None, 
              z=None, 
              cmap='Blues', 
              norm=None, 
              linewidth=3,
              alpha=1.0):
    """Plot a (gradient) colorline with the coordinates x and y.
    
    Args:
        x (list): The x data
        y (list): The y data
        ax: The axis on which to plot.
        z: An array that specifies the colors
        cmap (str): The colormap
        norm : The norm
        linewidth (float): The linewidth
        alpha (float): The transparency alpha
    
    Returns:
        matplotlib.LineCollection: The line collection
    """

    # -- Resolve defaults
    ax = ax if ax is not None else plt.gca()
    norm = norm if norm is not None else plt.Normalize(0.0, 1.0)

    # Convert y and y into np.arrays
    x = np.array(x)
    y = np.array(y)

    if x.ndim != 1 or y.ndim != 1:
        raise ValueError()

    if z is None:
        z = np.linspace(0., 1., x.size)
    elif not hasattr(z, '__iter__'):
        # Presumably a single number; allow it being handled in the same way
        z = np.array([z])
    else:
        # Make sure it is an array and of correct size
        z = np.array(z)
        if z.ndim != 1:
            raise ValueError()

    # Get the colormap
    cmap = plt.get_cmap(cmap)

    # -- Done with argument checks now.
     
    # Calculate the segments
    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)

    # Create the line collection
    lc = LineCollection(segments, array=z, cmap=cmap, norm=norm, 
                        linewidth=linewidth, alpha=alpha)
    
    # Register it with the axis and trigger autoscaling
    ax.add_collection(lc)
    ax.autoscale()

    # Pass back the line collection if other things need to be done with it
    return lc
