"""This module holds time-series plotting functions.

These plotting functions have in common, that they represent some kind of time
series. The time may be visualized on the x-axis or in some other way, e.g.
via the color of scatter points.
"""

import logging
from typing import Tuple, Union

import numpy as np
import xarray as xr

from .. import DataManager, UniverseGroup
from ..plotting import is_plot_func, PlotHelper, UniversePlotCreator
from ..dataprocessing import transform

# Get a logger
log = logging.getLogger(__name__)

# Global variables, e.g. shared helper settings
_density_hlpr_kwargs = dict(set_labels=dict(x="Time [Iteration Steps]",
                                            y="Density"),
                            set_limits=dict(x=('min', 'max'),
                                            y=(0., 1.)))

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(**_density_hlpr_kwargs)
              )
def density(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
            model_name: str, path_to_data: str,
            mean_of: Tuple[str],
            preprocess: Tuple[Union[str, dict]]=None,
            transformations_log_level: int=10,
            sizes_from: str=None, size_factor: float=1.,
            **plot_kwargs) -> None:
    """Plot the density of a mask, i.e. of a dataset holding booleans.
    
    This plotting function is useful when creating a plot from data that is
    encoded in a binary fashion, e.g.: arrays containing True to denote the
    existence of some entity or some value and False to denote its absence.
    
    If the dataset chosen via ``path_to_data`` is not already of boolean data
    type, the ``preprocess`` argument is to be used to generate the
    array-like boolean. By means of this argument, a binary operation of form
    ``data <operator> rhs_value`` is carried out, which results in the desired
    mask.
    
    Another feature of this plotting function is that it can include another
    data source to use for the sizes of the plots; in that case, a scatter plot
    rather than a line plot is carried out.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The name of the model the data resides in, i.e. the
            base path within the UniverseGroup.
        path_to_data (str): Which data to use as the mask
        mean_of (Tuple[str], optional): which data dimensions to calculate the
            density over. If this evaluates to False, the operation will be
            skipped
        preprocess (Tuple[Union[str, dict]], optional): Apply pre-processing
            transformations to the selected data.
            With the parameters specified here, multiple transformations can
            be applied to the data. This can be used for dimensionality
            reduction of the data, but also for other operations, e.g. to
            select only a slice of the data.
            See :py:func:`utopya.dataprocessing.transform` for more info.
            NOTE The operations are carried out _before_ calculating the
            density over the parameters specified in ``mean_of``.
            The ``preprocess``ing should not be used for calculating the mean.
        transformations_log_level (int, optional): With which log level to
            perform the preprocess. Useful for debugging.
        sizes_from (str, optional): If given, this is expected to be the path
            to a dataset that contains size values for a scatter plot. This
            leads to a scatter rather than a line-plot. The sizes are not used
            directly but are normalized by dividing with the maximum size; this
            makes configuration via the ``size_factor`` parameter feasible.
        size_factor (float, optional): The factor by which to scale the sizes
            given in the ``sizes_from`` argument.
        **plot_kwargs: Passed on to plt.plot or plt.scatter
    
    Raises:
        ValueError: If the selected data is not a boolean mask. This error can
            be alleviated by providing the ``preprocess`` argument.
    
    Returns:
        None: Description
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]

    # Apply a mask, if configured
    if preprocess:
        data = transform(data, *preprocess,
                         log_level=transformations_log_level)

    # Check if the data is binary now, which is required for all below
    if data.dtype is not np.dtype('bool'):
        raise ValueError("Data at '{}' was of dtype {}, which needs to be "
                         "converted to a binary mask for use in the "
                         "density plot function. To do so, provide the "
                         "`transformations` argument to the method."
                         "".format(data.path, data.dtype))

    # Calculate the mean over the specified dimensions, if specified to do so
    density = data.mean(dim=mean_of) if mean_of else data

    # Associate time coordinates, if not already the case
    if 'time' not in density.coords:
        density.coords['time'] = uni.get_times_array()

    # If there are no sizes to be plotted, can directly do the line plot
    if sizes_from is None:
        hlpr.ax.plot(density.coords['time'], density, **plot_kwargs)
        return
    
    # Otherwise, need to get the sizes and performs a scatter plot
    sizes = uni['data'][model_name][sizes_from]
    # Need to normalize the sizes to the maximum value, otherwise it get's
    # really hard to choose a sensible size_factor.

    # Can now call the scatter plot
    hlpr.ax.scatter(density.coords['time'], density,
                    s=(sizes.data / sizes.max() * size_factor),
                    **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(**_density_hlpr_kwargs)
              )
def densities(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
              model_name: str, to_plot: dict, **common_plot_kwargs):
    """Like density, but for several specifications given by the ``to_plot``
    argument.

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The name of the model the data resides in, i.e. the
            base path within the UniverseGroup.
        to_plot (dict): Which data to plot the densities of. The keys of this
            dict are used as ``path_to_data`` for the ``density`` function. The
            values are unpacked and passed to ``density``
        **common_plot_kwargs: Passed along to the ``density`` plot function for
            all calls. Note that this may not contain any keys that are given
            within ``to_plot``!
    """
    for path_to_data, specs in to_plot.items():
        if not isinstance(specs, dict):
            log.warning("Parameters for `path_to_data` '%s' were not a dict "
                        "but '%s'! Skipping this plot.", path_to_data, specs)
            continue

        density(dm, uni=uni, hlpr=hlpr, model_name=model_name,
                path_to_data=path_to_data, **specs, **common_plot_kwargs)

    if len(to_plot) > 1:
        hlpr.provide_defaults('set_legend', use_legend=True, loc='best')
