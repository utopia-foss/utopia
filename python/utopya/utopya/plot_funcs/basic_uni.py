"""Implements basic generic plot universe plot functions"""

import logging
from typing import Union, Tuple

import numpy as np
import xarray as xr

from .. import DataManager, UniverseGroup
from ..plotting import is_plot_func, PlotHelper, UniversePlotCreator
from ..dataprocessing import transform

from ._basic import _errorbar


# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator)
def lineplot(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
             model_name: str, path_to_data: str, transform_data: dict=None,
             transformations_log_level: int=10,
             **plot_kwargs):
    """Performs an errorbar plot of a specific universe.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The data for this universe
        hlpr (PlotHelper): The PlotHelper
        model_name (str): The name of the model the data resides in
        path_to_data (str): The path to the data within the model data
        transform_data (dict, optional): Transformations to apply to the data.
            This can be used for dimensionality reduction of the data, but
            also for other operations, e.g. to selecting a slice.
            For available parameters, see
            :py:func:`utopya.dataprocessing.transform`
        transformations_log_level (int, optional): The log level of all the
            transformation operations.
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        ValueError: On invalid data dimensionality
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]

    # Apply transformations
    if transform_data:
        data = transform(data, *transform_data,
                         log_level=transformations_log_level)

    # Require 1D data now
    if data.ndim != 1:
        raise ValueError("Lineplot requires 1D data, but got {}D data of "
                         "shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_data` argument to arrive "
                         "at plottable data."
                         "".format(data.ndim, data.shape, data))

    # Create the line plot
    hlpr.ax.plot(data.coords[data.dims[0]], data, **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator)
def errorbar_single(dm: DataManager, *,
                    uni: UniverseGroup,
                    hlpr: PlotHelper,
                    model_name: str,
                    path_to_data: str,
                    transform_data: Tuple[Union[dict, str]]=None,
                    plot_std: Union[str, bool]=False,
                    transform_std: Tuple[Union[dict, str]]=None,
                    transformations_log_level: int=10,
                    **errorbar_kwargs):
    """Plot of a single error bar line
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The data for this universe
        hlpr (PlotHelper): The PlotHelper
        model_name (str): The name of the model the data resides in
        path_to_data (str): The path to the data within the model data
        transform_data (Tuple[Union[dict, str]], optional): Apply
            transformations to the ``to_plot`` data in ``mv_data``.
            With the parameters specified here, multiple transformations can
            be applied. This can be used for dimensionality reduction of the
            data, but also for other operations, e.g. to selecting a slice.
            For available parameters, see
            :py:func:`utopya.dataprocessing.transform`
        plot_std (Union[str, bool], optional): Either a boolean or a path to
            dataset within the model to use for plotting the standard
            deviation. If True, the mean data will be used. If False, no
            standard deviation is plotted.
        transform_std (Tuple[Union[dict, str]], optional): Like
            ``transform_data``, but applied to the data selected in
            ``plot_std``.
        transformations_log_level (int, optional): With which log level to
            perform the transformations. Useful for debugging.
        **errorbar_kwargs: Passed on to plt.errorbar
    """
    # Get the data and apply transformations, e.g. to reduce dimensionality
    data = uni['data'][model_name][path_to_data]
    
    if transform_data:
        data = transform(data, *transform_data,
                         log_level=transformations_log_level)

    # Get the standard deviation data, if given, and also apply transformations
    std = None
    if plot_std:
        std = uni['data'][model_name][plot_std if isinstance(plot_std, str)
                                      else path_to_data]

        if transform_std:
            std = transform(std, *transform_std,
                            log_level=transformations_log_level)

    # Just plot a single line
    _errorbar(hlpr=hlpr, data=data, std=std, **errorbar_kwargs)

    # Done.    

@is_plot_func(creator_type=UniversePlotCreator)
def errorbar(dm: DataManager, *,
             uni: UniverseGroup,
             hlpr: PlotHelper,
             model_name: str,
             to_plot: dict,
             cmap: str=None,
             transformations_log_level: int=10,
             **errorbar_kwargs):
    """Perform an errorbar plot from the selected multiverse data.
    
    This plot, ultimately, requires 1D data, where the remaining dimension is
    plotted on the x-axis. The ``transform_data`` or ``lines_from`` arguments
    can be used to work with higher-dimensional data.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The data for this universe
        hlpr (PlotHelper): The PlotHelper
        model_name (str): The common name of the model the data resides in.
            Can be overwritten by information in ``to_plot``.
        to_plot (dict): A dict of specifications of lines to plot. For details
            see :py:func:`utopya.plot_funcs.basic_uni.errorbar_single`.
        cmap (str, optional): If given, the lines created from ``to_plot``
            will be colored according to this color map.
        transformations_log_level (int, optional): The log level of all
            transformation operations specified in each ``to_plot`` entry.
        **errorbar_kwargs: Passed on to plt.errorbar
    
    Deleted Parameters:
        mv_data (xr.Dataset): The extracted multidimensional data
    """
    num_lines = len(to_plot)

    if num_lines > 1:
        hlpr.provide_defaults('set_legend', use_legend=True)

    # Determine whether there will be colours according to a color map
    if cmap is not None:
        cmap = mpl.cm.get_cmap(cmap)
        colors = [cmap(i/max(num_lines-1, 1)) for i in range(num_lines)]
    else:
        colors = [None] * num_lines

    # Iterate over the plot specifications
    for (path_to_data, plot_spec), color in zip(to_plot.items(), colors):
        # Prepare additional kwargs
        add_kwargs = dict()

        if color is not None and 'color' not in plot_spec:
            add_kwargs['color'] = color

        if 'model_name' not in plot_spec:
            add_kwargs['model_name'] = model_name

        if 'label' not in plot_spec:
            add_kwargs['label'] = path_to_data

        if 'transformations_log_level' not in plot_spec:
            add_kwargs['transformations_log_level'] = transformations_log_level

        # Pass it on to the function that plots only a single line
        errorbar_single(dm, uni=uni, hlpr=hlpr, path_to_data=path_to_data,
                        **plot_spec, **add_kwargs, **errorbar_kwargs)

