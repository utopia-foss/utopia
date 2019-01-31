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


def colorline(x, y, *, ax=None, z=None, cmap='Blues', norm=plt.Normalize(0.0, 1.0), 
                linewidth=3, alpha=1.0):
    """Plot a (gradient) colorline with the coordinates x and y.
    
    Args:
        x (list): The x data
        y (list): The y data
        ax (optional): Defaults to None. The axis on which to plot.
        z (optional): Defaults to None. An array that specifies the colors
        cmap (str, optional): Defaults to 'viridis'. The colormap
        norm , optional): Defaults to plt.Normalize(0.0, 1.0). The norm
        linewidth (int, optional): Defaults to 3. The linewidth
        alpha (float, optional): Defaults to 1.0. The transparency alpha
    
    Returns:
        matplotlib.LineCollection: The line collection
    """
    def make_segments(x, y):
        """Create list of line segments from x and y coordinates in the correct 
        format for LineCollection.
        
        Args:
            x (array): The x data
            y (array): The y data
        
        Returns:
            np.ndarray: The data in the form: data[numlines][points per line][2d_data_array]
        """

        points = np.array([x, y]).T.reshape(-1, 1, 2)
        segments = np.concatenate([points[:-1], points[1:]], axis=1)
        
        return segments

    # Transform the arrays to list 
    x = list(x)
    y = list(y)

    # Get the colormap
    cmap = plt.get_cmap(cmap)

    # Default colors equally spaced on [0,1]:
    if z is None:
        z = np.linspace(0.0, 1.0, len(x))
           
    # If z is a single number, make sure to create an array out of it.
    if not hasattr(z, "__iter__"):  
        z = np.array([z])
        
    z = np.asarray(z)
    
    segments = make_segments(x, y)
    # Create a line collection object 
    lc = LineCollection(segments, array=z, cmap=cmap, norm=norm, 
                        linewidth=linewidth, alpha=alpha)
    
    if ax is None:
        ax = plt.gca()

    ax.add_collection(lc)
    
    # Automatically scale the limits
    ax.autoscale()

    return lc