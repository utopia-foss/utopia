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
