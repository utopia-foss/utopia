"""PredatorPrey-model plot function for phase space"""

from typing import Union

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="Predator Density $[1/A]$",
                                y="Prey Density $[1/A]$"),
                set_limits=dict(x=(0., None),
                                y=(0., None))
              ))
def phase_space(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
                cmap: str=None, **scatter_kwargs):
    """plots the frequency of one species against the frequency of the other
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The universe from which to plot the data
        hlpr (PlotHelper): The PlotHelper instance
        cmap (str, optional): The cmap which is used to color-code the time
            development. If not given, will not color-code it.
        **scatter_kwargs: Passed on to plt.scatter
    """
    # Extract the population data ...
    prey = uni['data']['PredatorPrey']['prey']
    predator = uni['data']['PredatorPrey']['predator']

    # Get the prey and predator frequencies per time step by calculating a mean
    # over all dimensions but time
    f_prey = prey.mean(dim=[d for d in prey.dims if d != 'time'])
    f_predator = predator.mean(dim=[d for d in predator.dims if d != 'time'])

    # If a colormap was given, use it to color-code the time
    cc_kwargs = ({} if cmap is None
                 else dict(c=f_prey.coords['time'], cmap=cmap))
    
    # Plot the phase space
    sc = hlpr.ax.scatter(f_predator, f_prey, **cc_kwargs, **scatter_kwargs)

    # Add a colorbar
    if cc_kwargs:
        hlpr.fig.colorbar(sc, ax=hlpr.ax, fraction=0.05, pad=0.02,
                          label="Time [Iteration Steps]")
