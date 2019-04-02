"""PredatorPrey-model plot function for phase space"""

from typing import Union

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
             helper_defaults=dict(
                 set_labels=dict(x="Predator density",
                                y="Prey density"),
                 set_limits=dict(x=(0.0, 0.1),
                                 y=(0.0, 0.1)))
             )
def phase_space(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper, 
                cmap: str=None, show_grid: bool=False, 
                **plot_kwargs):
    """plots the frequency of one species against the frequency of the other

    Args:
    dm (DataManager): The data manager from which to retrieve the data
    uni (UniverseGroup): The universe from which to plot the data
    cmap (str): The cmap which is used to color-code the time development
    show_grid (bool): Show a grid 
    save_kwargs (dict, optional): kwargs to the plt.savefig function
    **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']

    # Extract the population data ...
    prey = grp['prey']
    predator = grp['predator']

    # Get the prey and predator frequencies per time step by summing them up
    # and dividing them by the total number of cells per time step
    # NOTE This only works if time is the first dimension
    f_prey = prey.sum(dim=[d for d in prey.dims if d!='time']) \
                / np.prod(prey.shape[1:])
    f_predator = predator.sum(dim=[d for d in prey.dims if d!='time']) \
                / np.prod(predator.shape[1:])

    # Plot the phase space, either color coding the time or not
    if cmap:
        hlpr.ax.scatter(f_predator, f_prey, c=range(f_prey.size), 
                        cmap=cmap, **plot_kwargs)
    else:
        hlpr.ax.scatter(f_predator, f_prey, **plot_kwargs)

    # Add a grid in the background if desired
    hlpr.ax.grid(b=show_grid, which='both')

