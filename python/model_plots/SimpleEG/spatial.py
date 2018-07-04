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
                    properties: Union[list, str], 
                    cmaps: Union[list, str], 
                    rngs: Union[list, str], 
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
    # Get the group that all datasets are in
    grp = dm['uni'][str(uni)]['data/SimpleEG']

    # Get the shape of the 2D grid to later reshape the data
    cfg = dm['uni'][str(uni)]['cfg']
    grid_size = cfg['SimpleEG']['grid_size']
    steps = cfg['num_steps']
    new_shape = (steps+1, grid_size[0], grid_size[1])

    # create animation writer
    writer = manimation.writers['ffmpeg'](fps=4, metadata={'title' : 'Grid Animation for {}'.format("_".join(properties)), 
                                                           'artist' : 'Utopia',
                                                           'comment' : 'This is a comment'})
    
    # Get the number of properties to plot
    num_properties = len(properties)

    # Set plot parameters
    rcParams.update({'font.size': 20})
    rcParams['figure.figsize'] = (6.0*num_properties, 5.0)

    # Create figure
    fig, axs = plt.subplots(1,num_properties)

    # Need this line so that the loop can be run even if there is only one properties
    if num_properties == 1 : axs = [axs] 

    # Extract the data of the strategies in the CA
    data_1d = {}
    for p in properties:
        data_1d[p] = grp[p]
    
    data = {}
    for key, value in data_1d.items():
        data[key] = np.reshape(value, new_shape)

    with writer.saving(fig, out_path, dpi=dpi):

        for key, d in data.items():

            for t in range(steps):
                cbs = []
                # iterate over all properties
                for j in range(num_properties):
                    axs[j].cla()
                    
                    if num_properties == 1:
                        im = axs[j].imshow(d[t], cmap=cmaps[j], animated=True, origin='lower', vmin=rngs[j][0], vmax=rngs[j][1])
                    else:
                        im = axs[j].imshow(d[t], cmap=cmaps[j], animated=True, origin='lower', vmin=rngs[j][0], vmax=rngs[j][1])

                    # fraction and pad get magic values that makes the plots look nice
                    cbs.append(fig.colorbar(im ,ax=axs[j], fraction=0.046, pad=0.04))
                    axs[j].axis('off')

                writer.grab_frame()
                
                # clean up the colorbars (needed to prevent the colorbars from piling up from frame to frame)
                for j in range(num_properties) :
                    cbs[j].remove()