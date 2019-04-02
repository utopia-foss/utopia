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
              prey: bool=True, predator: bool=True, 
              **plot_kwargs):
    """Calculates the frequency of a given Population and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The universe from which to plot the data
        prey (bool): If True the prey frequency is plotted
        predator (bool): If True the predator frequency is plotted
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']

    def calculate_frequency(species: str):
        # Extract the species data ...
        species = grp[species]

        # Get the species frequency per time step by summing them up
        # and dividing them by the total number of cells per time step
        # NOTE This only works if time is the first dimension
        return species.sum(dim=[d for d in species.dims if d!='time']) \
                    / np.prod(species.shape[1:])            
        
    if prey:
        freq = calculate_frequency("prey")
        # Create the plot
        hlpr.ax.plot(freq, label="Prey", **plot_kwargs)

    if predator:
        freq = calculate_frequency("predator")
        # Create the plot
        hlpr.ax.plot(freq, label="Predator", **plot_kwargs)
