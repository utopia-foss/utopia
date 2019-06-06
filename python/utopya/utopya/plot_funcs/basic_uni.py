"""Implements basic generic plot universe plot functions"""

import logging
from typing import Union, Tuple

import numpy as np
import xarray as xr
import pandas as pd

from .. import DataManager, UniverseGroup
from ..plotting import is_plot_func, PlotHelper, UniversePlotCreator
from ..dataprocessing import transform

from ._basic import _errorbar


# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator)
def lineplot(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
             model_name: str, path_to_data: Union[str, Tuple[str, str]],
             transform_data: dict=None, transformations_log_level: int=10,
             **plot_kwargs):
    """Performs an errorbar plot of a specific universe.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The data for this universe
        hlpr (PlotHelper): The PlotHelper
        model_name (str): The name of the model the data resides in
        path_to_data (str or Tuple[str, str]): The path to the data within the
            model data or the paths to the x and the y data, respectively
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
        ValueError: On mismatch of data shapes
    """
    # Get the data
    if isinstance(path_to_data, str):
        data_y = uni['data'][model_name][path_to_data]

        # Apply transformations 
        if transform_data:
            data_y = transform(data_y, *transform_data,
                               aux_data=uni['data'][model_name],
                               log_level=transformations_log_level)
        
        data_x = data_y.coords[data_y.dims[0]]
    
    else:
        data_x = uni['data'][model_name][path_to_data[0]]
        data_y = uni['data'][model_name][path_to_data[1]]

        # Apply transformations 
        if transform_data:
            data_x = transform(data_x, *transform_data.get('x', {}),
                               aux_data=uni['data'][model_name],
                               log_level=transformations_log_level)
            data_y = transform(data_y, *transform_data.get('y', {}),
                               aux_data=uni['data'][model_name],
                               log_level=transformations_log_level)

        if data_x.shape != data_y.shape:
            raise ValueError("Mismatch of datashapes! Were {} and {}, but have"
                             " to be of same shape (after transfromation)."
                             "".format(data_x.shape, data_y.shape))

    # Require 1D data now
    if data_x.ndim != 1:
        raise ValueError("Lineplot requires 1D data, but got {}D data of "
                         "shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_data` argument to arrive "
                         "at plottable data."
                         "".format(data_x.ndim, data_x.shape, data_x))
    if data_y.ndim != 1:
        raise ValueError("Lineplot requires 1D data, but got {}D data of "
                         "shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_data` argument to arrive "
                         "at plottable data."
                         "".format(data_y.ndim, data_y.shape, data_y))

    # Create the line plot
    hlpr.ax.plot(data_x, data_y, **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator)
def lineplots(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
              model_name: str, to_plot: dict, **common_plot_kwargs):
    """Like lineplot, but for several specifications given by the ``to_plot``
    argument.

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The name of the model the data resides in, i.e. the
            base path within the UniverseGroup.
        to_plot (dict): Which data to plot. The keys of this
            dict are used as ``path_to_data`` for the ``lineplot`` function.
            The values are unpacked and passed to ``lineplot``
        **common_plot_kwargs: Passed along to the ``lineplot`` plot function
            for all calls. Note that this may not contain any keys that are
            given within ``to_plot``!
    """
    for name, specs in to_plot.items():
        if not isinstance(specs, dict):
            raise TypeError("Parameters for property '{}' were not a dict "
                            "but {} with value: '{}'!"
                            "".format(name, type(specs), specs))
        
        path_to_data = specs.pop('path_to_data', name)
        lineplot(dm, uni=uni, hlpr=hlpr, model_name=model_name,
                 path_to_data=path_to_data, **specs, **common_plot_kwargs)

    if len(to_plot) > 1:
        hlpr.provide_defaults('set_legend', use_legend=True, loc='best')


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

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="$t_0$", y="$t_1$")
              )
             )
def distance_map(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
                 model_name: str, path_to_data: str,
                 transform_data: dict=None, transformations_log_level: int=10,
                 cbar_kwargs: dict=None, **plot_kwargs):
    """Creates a distance map of data at t_0 to data at t_1.
    
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
        cbar_kwargs (dict, optional): Passed on to fig.colorbar
        **plot_kwargs: Passed on to ax.matshow
    
    Raises:
        ValueError: On invalid data dimensionality
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]

    # Apply transformations 
    if transform_data:
        data = transform(data, *transform_data,
                         aux_data=uni['data'][model_name],
                         log_level=transformations_log_level)

    # Require 1D data now
    if data.ndim != 1:
        raise ValueError("Distance map requires 1D data, but got {}D data of "
                         "shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_data` argument to arrive "
                         "at plottable data."
                         "".format(data.ndim, data.shape, data))

    # Create new dim
    data = data.expand_dims('t1')

    # Plot distance_map
    cax = hlpr.ax.matshow(abs(data.data - data.T.data), **plot_kwargs)

    # Adjust ticks
    hlpr.ax.invert_yaxis()
    hlpr.fig.gca().xaxis.tick_bottom()
    
    # Add colobar
    if not cbar_kwargs:
        cbar_kwargs = {'label': 'distance'}
    hlpr.fig.colorbar(cax, **cbar_kwargs)