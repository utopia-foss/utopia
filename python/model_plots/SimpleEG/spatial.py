"""SimpleEG-model specific plot functions for spatial figures"""
from typing import Union

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation as manimation
from matplotlib import rcParams

from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def grid_animation( dm: DataManager, 
                    *, 
                    out_path: str, 
                    uni: int, 
                    to_plot: dict,
                    # properties: Union[list, str], 
                    # cmaps: Union[list, str], 
                    # rngs: Union[list, str], 
                    fps: int=1, 
                    step_size: int=1,
                    dpi: int=100):
    """
    datafile -- model data output file
    properties -- list of strings, each string indicating a property of the model that has been written to a dataset in each time step
    cmap -- list of colormaps, one for each property
    rngs -- list color ranges, one for each property
    fps -- frames per second
    step -- number of time steps between frames (e.g. if step=5, only each fifth 
            dataset will be used for the animation)
    """    
    def plot_property(name, *, initial_data, ax, cmap, limits, title=None):
        # Create imshow
        im = ax.imshow(initial_data, cmap=cmap, animated=True, origin='lower', vmin=limits[0], vmax=limits[1])

        if title is not None:
            ax.set_title(title)

        # Create colorbars
        fig = plt.gcf()
        fig.colorbar(im ,ax=ax, fraction=0.046, pad=0.04)
        ax.axis('off')
        return im

    # Get the group that all datasets are in
    grp = dm['uni'][str(uni)]['data/SimpleEG']

    # Get the shape of the 2D grid to later reshape the data
    cfg = dm['uni'][str(uni)]['cfg']
    grid_size = cfg['SimpleEG']['grid_size']
    steps = cfg['num_steps']
    new_shape = (steps+1, grid_size[0], grid_size[1])

    # Extract the data of the strategies in the CA    
    data_1d = {p: grp[p] for p in to_plot.keys()}
    data = {k: np.reshape(v, new_shape) for k,v in data_1d.items()}
        
    # create animation writer
    writer = manimation.writers['ffmpeg'](fps=4, metadata={'title' : 'Grid Animation for {}'.format("_".join(to_plot.keys())), 
                                                           'artist' : 'Utopia'})

    # Set plot parameters
    # rcParams.update({'font.size': 20})
    rcParams['figure.figsize'] = (6.0*len(to_plot), 5.0)

    # Create figure
    fig, axs = plt.subplots(1,len(to_plot))

    # Assert that the axes are stored in a list even if it is only one axis.
    if len(to_plot) == 1 :
         axs = [axs] 
    
    # store 
    ims = []
    
    with writer.saving(fig, out_path, dpi=dpi):
        
        for t in range(steps):

            for i, (ax, (key, props)) in enumerate(zip(axs, to_plot.items())):

                    if t == 0:
                        ims.append(plot_property(key, initial_data=data[key][t], ax=ax, **props))
                        continue

                    # update imshow data
                    ims[i].set_data(data[key][t])

                    # colorbar update data???                        

            writer.grab_frame()