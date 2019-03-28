"""PredatorPrey-model plot function for time-series"""

from typing import Union

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
             helper_defaults=dict(
                 set_labels=dict(x="Iteration step",
                                 y="Frequency"),
                 set_limits=dict(y=(-0.05, 1.05)),
                 set_legend=dict(use_legend=True, loc='best'))
             )
def frequency(dm: DataManager, *, hlpr: PlotHelper, uni: UniverseGroup, 
              Population: Union[str, list] = ['prey', 'predator'], 
              **plot_kwargs):
    """Calculates the frequency of a given Population and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The universe from which to plot the data
        Population (Union[str, list], optional): The population to plot
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']

    # Get the gridsize 
    grid_size = uni['cfg']['PredatorPrey']['cell_manager']['grid']['resolution']

    # Extract the data of the frequency
    population_data = grp['population'] 
    num_cells = grid_size * grid_size
    frequencies = [np.bincount(p.flatten(), minlength=4)[[1, 2]] / num_cells 
                   for p in population_data] 
    

    # Get the frequencies of the desired Population and plot it
    # Single population
    if isinstance(Population, str):
        y_data = [f[np.where(np.asarray(['prey', 'predator']) == Population)] 
                  for f in frequencies]

        # Create the plot
        hlpr.ax.plot(y_data, label=Population, **plot_kwargs)
        
    
    # Multiple populations
    elif isinstance(Population, list):
        for p in Population:
            y_data = [f[np.where(np.asarray(['prey', 'predator']) == p)] 
                      for f in frequencies]

            # Create the plot
            hlpr.ax.plot(y_data, label=p, **plot_kwargs)

    else:
        raise TypeError("Invalid argument 'population' of type {} and value "
                        "'{}'!".format(type(Population), Population))
