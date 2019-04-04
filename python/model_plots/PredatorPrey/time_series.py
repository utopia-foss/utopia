"""PredatorPrey-model plot function for time-series"""

from typing import Union

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="Time [Iteration Steps]",
                                y="Density"),
                set_limits=dict(y=(0., 1.)),
                set_legend=dict(use_legend=True, loc='best'))
              )
def densities(dm: DataManager, *, hlpr: PlotHelper, uni: UniverseGroup, 
              lines_from: list, **plot_kwargs):
    """Calculates the density of a given Population and performs a lineplot
    from that.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper
        uni (UniverseGroup): The universe from which to plot the data
        lines_from (list): Which datasets to create lines from. Should be paths
            within the 'data/PredatorPrey' group. Their capitalized names are
            used for the line label.
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the times array
    times = uni.get_times_array()

    # Go over all names that lines are to be created from
    for dset_name in lines_from:
        # Get the raw data
        data = uni['data']['PredatorPrey'][dset_name]

        # Plot the data, taking the mean over all but the time dimension
        hlpr.ax.plot(times,
                     data.mean(dim=[d for d in data.dims if d != 'time']),
                     label=dset_name.capitalize(), **plot_kwargs)
