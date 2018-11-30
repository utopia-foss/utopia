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

def frequency(dm: DataManager, *, out_path: str, uni: UniverseGroup, 
              Population: Union[str, list] = ['prey', 'predator'], 
              save_kwargs: dict=None, **plot_kwargs):
    """Calculates the frequency of a given Population and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe from which to plot the data
        Population (Union[str, list], optional): The population to plot
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']

    # Extract the data of the frequency
    population_data = grp['population'] 
    num_cells = len(population_data[0])
    frequencies = [np.bincount(p, minlength=4)[[1, 2]] / num_cells 
                   for p in population_data] 
    

    # Get the frequencies of the desired Population and plot it
    # Single population
    if isinstance(Population, str):
        y_data = [f[np.where(np.asarray(['prey', 'predator']) == Population)] 
                  for f in frequencies]

        # Create the plot
        plt.plot(y_data, label=Population, **plot_kwargs)
        
    
    # Multiple populations
    elif isinstance(Population, list):
        for p in Population:
            y_data = [f[np.where(np.asarray(['prey', 'predator']) == p)] 
                      for f in frequencies]

            # Create the plot
            plt.plot(y_data, label=p, **plot_kwargs)

    else:
        raise TypeError("Invalid argument 'population' of type {} and value "
                        "'{}'!".format(type(Population), Population))

    # Set general plot options
    plt.ylim(-0.05, 1.05)
    plt.xlabel('Iteration step')
    plt.ylabel('Frequency')
    plt.legend(loc='best')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
