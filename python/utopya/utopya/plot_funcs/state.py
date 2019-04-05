"""This module provides plotting functions to visualize multiverse data"""

from ._setup import *

import xarray as xr
from typing import Union

from utopya import DataManager
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator

# -----------------------------------------------------------------------------

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
                 mean_of: Union[str, list]=None,
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
    # Calculate the mean over all dimensions that are mentioned within mean_of
    mv_data.mean(dim=[d for d in mv_data.dims if d in mean_of])

    # Case: Create multiple lines from a given dimension
    if lines_from:
        for line in mv_data[lines_from]:
            # Select the correct data that results in one line
            data_sel = mv_data.sel(**{line.name:line})

            # Get the mean and std data
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

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=MultiversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(y="Density"),
                set_legend=dict(use_legend=True),
                save_figure=dict(bbox_inches="tight")
                )
              )
def mean_and_std_snapshot(dm: DataManager, *,
                          mv_data: xr.Dataset,
                          hlpr: PlotHelper,
                          sweep_over: str,
                          snapshot_at: dict=dict(dim='time', idx=-1),
                          mean_of: str=None,
                          **plot_kwargs):
    """Plot a snapshot of the mean and standard deviation of a 
    
    Args:
        dm (DataManager): The data manager
        mv_data (xr.Dataset): The extracted multidimensional data
        hlpr (PlotHelper): The plot helper
        sweep_over (str): The data that is plotted on the x data resulted
            from a sweep over this dimension.
        snapshot_at (dict): The plot shows a snapshot over the 'dim'ension
            specified in this dictionary either at an index `idx` or at a
            specific 'value'. If nothing is specified the snapshot is done
            over the final time.
        mean_of (str, optional): Take the mean over this dimension
        lines_from (str, optional): Plot multiple lines from this dimension
        plot_kwargs (dict, optional): Kwargs that are passed to plt.errorbar
    """
    # Calculate the mean over all dimensions except for the time
    mv_data.mean(dim=[d for d in mv_data.dims if d in mean_of])

    # Extract the data of a snapshot in a provided dimension,
    if 'idx' in snapshot_at and 'value' in snapshot_at:
        raise KeyError("You are not allowed to specify the parameters 'idx' "
            "and 'value' at the same time! If you want to access a dimension "
            "via index, provide the desired 'idx'. If you want to specify "
            "a specific value at which to access the data, specify the 'value'!")

    elif 'idx' in snapshot_at:
        mv_data = mv_data.isel(**{snapshot_at['dim'] : snapshot_at['idx']})

    elif 'value' in snapshot_at:
        mv_data = mv_data.sel(**{snapshot_at['dim'] : snapshot_at['value']})

    else:
        raise KeyError("You need to specify either an index 'idx' or a 'value' "
            "parameter that specifies which data to access in the desired "
            "dimension!")

    # Get the mean and std data
    mean = mv_data['mean']
    if 'std' in mv_data:
        std = mv_data['std']
    else:
        std = None

    # Plot the data
    hlpr.ax.errorbar(mean[sweep_over], mean, yerr=std, **plot_kwargs)