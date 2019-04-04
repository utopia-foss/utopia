"""PredatorPrey-model plot function for time-series"""

from typing import Union

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="Iteration step", y="Density"),
                set_limits=dict(y=(0., 1.)),
                set_legend=dict(use_legend=True, loc='best'))
              )
def densities(dm: DataManager, *, hlpr: PlotHelper, uni: UniverseGroup, 
              prey: bool=True, predator: bool=True, **plot_kwargs):
    """Calculates the density of a given Population and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper
        uni (UniverseGroup): The universe from which to plot the data
        prey (bool, optional): If True the prey density is plotted
        predator (bool, optional): If True the predator density is plotted
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']

    def calculate_density(species: str):
        # Extract the species data ...
        species = grp[species]

        # Get the species density per time step by summing them up
        # and dividing them by the total number of cells per time step
        # NOTE This only works if time is the first dimension
        return species.mean(dim=[d for d in species.dims if d != 'time'])
        
    if prey:
        hlpr.ax.plot(calculate_density("prey"),
                     label="Prey", **plot_kwargs)

    if predator:
        hlpr.ax.plot(calculate_density("predator"),
                     label="Predator", **plot_kwargs)
