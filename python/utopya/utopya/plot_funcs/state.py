"""This module provides plotting functions to visualize multiverse data"""

from ._setup import *

import xarray as xr

from utopya import DataManager
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator

@is_plot_func(creator_type=MultiversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time", y="Density"),
                set_legend=dict(use_legend=True),
                save_figure=dict(bbox_inches="tight")
                )
              )
def mean_and_std(dm: DataManager, *,
                 mv_data: xr.Dataset,
                 hlpr: PlotHelper,
                 mean_of: str=None,
                 lines_from: str=None,
                 **plot_kwargs):
    """Plot the mean and standard deviation of multidimensional data
    
    Args:
        dm (DataManager): The data manager
        mv_data (xr.Dataset): The extracted multidimensional data
        hlpr (PlotHelper): The plot helper
        mean_of (str, optional): Take the mean of this dimension
        lines_from (str, optional): Plot multiple lines from this dimension
        plot_kwargs (dict, optional): Kwargs that are passed to plt.errorbar
    """
    # Calculate the mean over all dimensions except for the time
    mv_data.mean(dim=[d for d in mv_data.dims if d in mean_of])

    # Case: Create multiple lines from a given dimension
    if lines_from:
        for line in mv_data[lines_from]:
            # Select the correct data that results in one line
            data_sel = mv_data.sel(**{line.name:line})

            # Get the averaged data
            mean = data_sel['mean']
            if 'std' in data_sel:
                std = data_sel['std']
            else:
                std = None

            # Get the values of the dimension from which to create the lines
            plot_kwargs['label'] = '${}$'.format(line.values)

            # Plot the data
            hlpr.ax.errorbar(mean['time'], mean, yerr=std, **plot_kwargs)

            # As default use the dimension name from which the lines originate
            hlpr.provide_defaults('set_legend', title='{}'.format(line.name))

    # Case: Create a single line
    else:
        # Get the averaged data
        mean = mv_data['mean']
        if 'std' in mv_data:
            std = mv_data['std']
        else:
            std = None

        # Plot the data
        hlpr.ax.errorbar(mean['time'], mean, yerr=std, **plot_kwargs)