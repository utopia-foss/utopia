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
                Population: Union[str, list] = ['prey', 'predator'],
                **plot_kwargs):
    """plots the frequency of one species against the frequency of the other

    Args:
    dm (DataManager): The data manager from which to retrieve the data
    uni (UniverseGroup): The universe from which to plot the data
    cmap (str): The cmap which is used to color-code the time development
    show_grid (bool): Show a grid 
    Population (Union[str, list], optional): The population to plot
    save_kwargs (dict, optional): kwargs to the plt.savefig function
    **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid population argument
    """
    # Get the group that all datasets are in
    grp = uni['data']['PredatorPrey']
    
    # Get the grid size
    grid_size = uni['cfg']['PredatorPrey']['cell_manager']['grid']['resolution']

    # Extract the data of the frequency
    population_data = grp['population']
    num_cells = grid_size * grid_size
    frequencies = [np.bincount(p.flatten(), minlength=4)[[1, 2]] / num_cells
                   for p in population_data]
    
    # rearrange data for plotting - one array each with population densities
    # and index to store the time step
    prey = [f[0] for f in frequencies]
    pred = [f[1] for f in frequencies]

    # plot the phase space, either color coding the time or not
    if cmap:
        hlpr.ax.scatter(pred, prey, c=range(len(frequencies)), s=0.2, 
                        cmap=cmap, **plot_kwargs)
    else:
        hlpr.ax.scatter(pred, prey, s=0.2, **plot_kwargs)

    # add a grid in the background if desired
    hlpr.ax.grid(b=show_grid, which='both')

