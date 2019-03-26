"""PredatorPrey-model plot function for time-series"""
    
import logging
from typing import Union

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def phase_space(dm: DataManager, *, out_path: str, uni: UniverseGroup, plot_par: dict,
                Population: Union[str, list] = ['prey', 'predator'],
                save_kwargs: dict=None, **plot_kwargs):
    """plots the frequency of one species against the frequency of the other

    Args:
    dm (DataManager): The data manager from which to retrieve the data
    out_path (str): Where to store the plot to
    uni (UniverseGroup): The universe from which to plot the data
    plot_par: specify properties of the plot
    Population (Union[str, list], optional): The population to plot
    save_kwargs (dict, optional): kwargs to the plt.savefig function
    **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']
    
    # Get the gridsize
    grid_size = uni['cfg']['PredatorPrey']['cell_manager']['grid']['resolution']
    
    #if the plot is colorcoded set the colormap
    if(plot_par['color_code']):
        cmap=plot_par['cmap']
        if isinstance(cmap, str):
            colormap = cmap
        else:
            raise TypeError("Argument cmap needs to be a string with "
                            "name of the colormap."
                            "Was: {} with value: '{}'"
                            "".format(type(cmap), cmap))

    # Extract the data of the frequency
    population_data = grp['population']
    num_cells = grid_size * grid_size
    frequencies = [np.bincount(p.flatten(), minlength=4)[[1, 2]] / num_cells
                   for p in population_data]

    # rearrange data for plotting - one array each with population densities
    # and index to store the time step
    prey=[]
    pred=[]
    index=[]
    i=0
    for a in frequencies:
        prey.append(a[0])
        pred.append(a[1])
        index.append(i)
        i+=1
            
    # Create the plot

    # limit the plot range if demanded
    if(plot_par['specify_range']):
      Axes=plt.gca()
      Axes.set_xlim(left=plot_par['xrange'][0],right=plot_par['xrange'][1])
      Axes.set_ylim(bottom=plot_par['yrange'][0],top=plot_par['yrange'][1])

    # plot the phase space, either color coding the time or not
    if(plot_par['color_code']):
        plt.scatter(prey,pred, c=index, s=0.2, cmap=colormap)
    else:
        plt.scatter(prey,pred,s=0.2)

    # add a grid in the background if desired
    plt.grid(b=plot_par['show_grid'],which='both')

    # add labels to the axis
    plt.xlabel('Prey density')
    plt.ylabel('Predator density')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)


