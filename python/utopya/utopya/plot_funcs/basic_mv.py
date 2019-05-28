"""Implements basic generic multiverse plot functions"""

import logging
from typing import Union, Tuple

import numpy as np
import xarray as xr
import matplotlib as mpl

from .. import DataManager
from ..plotting import is_plot_func, PlotHelper, MultiversePlotCreator
from ..dataprocessing import transform

from ._basic import _errorbar


# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=MultiversePlotCreator)
def errorbar(dm: DataManager, *,
             mv_data: xr.Dataset,
             hlpr: PlotHelper,
             to_plot: str='data',
             transform_data: Tuple[Union[dict, str]]=None,
             lines_from: str=None,
             plot_std: Union[str, bool]=False,
             transform_std: Tuple[Union[dict, str]]=None,
             transformations_log_level: int=10,
             cmap: str=None,
             **errorbar_kwargs):
    """Perform an errorbar plot from the selected multiverse data.
    
    This plot, ultimately, requires 1D data, where the remaining dimension is
    plotted on the x-axis. The ``transform_data`` or ``lines_from`` arguments
    can be used to work with higher-dimensional data.
    
    Args:
        dm (DataManager): The data manager
        mv_data (xr.Dataset): The extracted multidimensional data
        hlpr (PlotHelper): The plot helper
        to_plot (str, optional): Which data variable from ``mv_data`` to plot.
            The data needs to be 1D. If it is not 1D, you can apply the
            ``transform`` argument to make it 1D. Alternatively, the
            ``lines_from`` argument can be given, in which case 2D data is
            allowed.
        transform_data (Tuple[Union[dict, str]], optional): Apply
            transformations to the ``to_plot`` data in ``mv_data``.
            With the parameters specified here, multiple transformations can
            be applied. This can be used for dimensionality reduction of the
            data, but also for other operations, e.g. to selecting a slice.
            For available parameters, see
            :py:func:`utopya.dataprocessing.transform`
        lines_from (str, optional): A valid dimension name over which to
            iterate in order to plot multiple errorbar lines. If None, only
            a single line is plotted.
        plot_std (Union[str, bool], optional): Which entry of the ``mv_data``
            to use for plotting the standard deviation.
            If True, the mean data will be used. If False, no standard
            deviation is plotted.
        transform_std (Tuple[Union[dict, str]], optional): Like
            ``transform_data``, but applied to the data selected in
            ``plot_std``
        transformations_log_level (int, optional): With which log level to
            perform the transformations. Useful for debugging.
        cmap (str, optional): If given, the lines created from ``lines_from``
            will be colored according to this color map
        **errorbar_kwargs: Passed on to plt.errorbar
    """
    # Get the data and apply transformations, e.g. to reduce dimensionality
    data = mv_data[to_plot]
    
    if transform_data:
        data = transform(data, *transform_data, aux_data=mv_data,
                         log_level=transformations_log_level)

    # Get the standard deviation data, if given, and also apply transformations
    std = None
    if plot_std:
        std = mv_data[plot_std if isinstance(plot_std, str) else to_plot]

        if transform_std:
            std = transform(std, *transform_std, aux_data=mv_data,
                            log_level=transformations_log_level)

    # Distinguish by `lines_from`; determines whether 1D or 2D data is needed
    if not lines_from:
        # Just plot a single line
        _errorbar(hlpr=hlpr, data=data, std=std, **errorbar_kwargs)

    else:
        if lines_from not in data.coords:
            log.warning("No labelled '%s' dimension available, only %s. Did "
                        "you perhaps not perform a sweep over this dimension?",
                        lines_from, ", ".join(data.coords.keys()))
            log.warning("Skipping plot.")
            return

        # Number of coordinates
        num_coords = len(data.coords[lines_from])

        # And color
        if cmap is not None:
            cmap = mpl.cm.get_cmap(cmap)
            colors = [cmap(i/max(num_coords-1, 1)) for i in range(num_coords)]
        else:
            colors = [None] * num_coords

        # Iterate over that coordinate
        for coord, color in zip(data.coords[lines_from], colors):
            # Prepare the selection (by value) specification dict
            sel_dict = {coord.name: coord.item()}  # single value -> dim. red.
            
            # Prepare additional kwargs
            add_kwargs = dict(label='${:.3g}$'.format(coord.item()))

            if color is not None:
                add_kwargs['color'] = color

            # Pass it on to the function with common errorbar plotting code
            _errorbar(hlpr=hlpr,
                      data=data.sel(**sel_dict),
                      std=(std if std is None else std.sel(**sel_dict)),
                      **add_kwargs, **errorbar_kwargs)

        # Set the title of the legend to include `lines_from`
        hlpr.provide_defaults('set_legend', title=lines_from)


@is_plot_func(creator_type=MultiversePlotCreator)
def asymptotic_average(dm: DataManager, *,
                       mv_data: xr.Dataset,
                       hlpr: PlotHelper,
                       average_dim: str,
                       average_fraction: float,
                       data_variable: str='data',
                       plot_std: bool=True,
                       transform_data: Tuple[Union[str, dict]]=None,
                       transformations_log_level: int=10,
                       **errorbar_kwargs):
    """Plots asymptotically averaged single values.
    
    After ``transform_data``, the ``data_variable`` in ``mv_data`` should be a
    2D array where one of the dimensions is ``average_dim``. All data from
    the other dimension are plotted as single points with standard deviation.
    
    Args:
        dm (DataManager): The data manager
        mv_data (xr.Dataset): The selected multiverse data
        hlpr (PlotHelper): The PlotHelper
        average_dim (str): The name of the dimension to average over.
        average_fraction (float): Which fraction of the dimension specified in
            ``average_dim`` to use for averaging. If it is desired to make
            this absolute, choose ``1.`` and use ``transform_data`` to select
            a smaller slice of the data.
        data_variable (str, optional): Name of the data variable from
            ``mv_data`` to use for plotting.
        transform_data (Tuple[Union[str, dict]], optional): Applied to the
            data array from ``mv_data``.
        transformations_log_level (int, optional): Log level of transformation
            operations.
        **errorbar_kwargs: Passed to plt.errorbar
    
    Raises:
        ValueError: On data that is not 2D or does not contain a time dimension
    """
    # Get the data
    data = mv_data[data_variable]

    # Apply transformations
    if transform_data:
        data = transform(data, *transform_data,
                         log_level=transformations_log_level)

    # Check that dimensionality is 2
    if data.ndim != 2 or average_dim not in data.dims:
        raise ValueError("Need a 2-dimensional xr.DataArray where one "
                         "dimension is 'time', but got: {}".format(data))

    # Select the subset by index
    num_indices = max(1, int(average_fraction * len(data.coords[average_dim])))
    to_average = data.isel(**{average_dim: slice(-num_indices, None)})
    log.info("Selected '%s' data in asymptotic value interval [%s, %s] "
             "(the %d data points available in last %.1f%% of [%s, %s]) "
             "for averaging ...",
             average_dim,
             to_average.coords[average_dim].min().item(),
             to_average.coords[average_dim].max().item(),
             len(to_average.coords[average_dim]),
             average_fraction*100,
             data.coords[average_dim].min().item(),
             data.coords[average_dim].max().item())

    # Perform the averaging
    mean = to_average.mean(dim=average_dim)
    std = to_average.std(dim=average_dim)

    # Find out which dimension remains
    iter_dim_name = mean.dims[0]
    
    # For the remaining dimension, plot the mean values
    log.info("Plotting %d point(s) over remaining dimension '%s' ...",
             len(mean.coords[iter_dim_name]), iter_dim_name)

    for coord_val in mean.coords[iter_dim_name]:
        sel_dict = {coord_val.name: coord_val.item()}
        hlpr.ax.errorbar(mean.sel(**sel_dict), coord_val,
                         yerr=std.sel(**sel_dict) if plot_std else None,
                         label="{:.3g}".format(coord_val.item()),
                         **errorbar_kwargs)
    
    hlpr.provide_defaults('set_labels', y=dict(label=iter_dim_name))
