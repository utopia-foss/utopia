"""This module holds time-series plotting functions"""

import logging
from typing import Tuple, Union

import numpy as np
import xarray as xr

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# Get a logger
log = logging.getLogger(__name__)

# Global variables, e.g. shared helper settings
_density_hlpr_kwargs = dict(set_labels=dict(x="Time [Iteration Steps]",
                                            y="Density"),
                            set_limits=dict(x=(0., None),
                                            y=(0., 1.)))

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(**_density_hlpr_kwargs)
              )
def density(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
            model_name: str, path_to_data: str,
            mean_of: Tuple[str], threshold: Union[float, None],
            sizes_from: str=None, size_factor: float=1., **plot_kwargs):
    """Plot the density of a mask, i.e. of a dataset holding booleans.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The name of the model the data resides in, i.e. the
            base path within the UniverseGroup.
        path_to_data (str): Which data to use as the mask
        mean_of (Tuple[str]): which data dimensions to calculate the density
            over.
        threshold (Union[float, None]): If not None, this threshold is applied
            to the data. All values larger or equal to this value are regarded
            as 1, all others as 0.
        sizes_from (str, optional): If given, this is expected to be the path
            to a dataset that contains size values for a scatter plot. This
            leads to a scatter rather than a line-plot. The sizes are used
            directly, only scaled with `size_factor`.
        size_factor (float, optional): The factor by which to scale the sizes
            given in the `sizes_from` argument.
        **plot_kwargs: Passed on to plt.plot or plt.scatter
    
    Raises:
        ValueError: If the selected data is not a boolean mask. This error can
            be alleviated by applying a threshold.
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]

    # Apply a threshold to get the mask.
    if threshold is not None:
        data = (data >= threshold)

    # TODO Should have other kinds of comparisons here

    # Now check if the data is binary
    if data.dtype is not np.dtype('bool'):
        raise ValueError("Data at '{}' was not of boolean dtype but "
                         "{}! Apply a threshold to the data using the "
                         "`threshold` argument."
                         "".format(data.path, data.dtype))

    # Calculate the mean over the specified dimensions
    density = data.mean(dim=mean_of)

    # Associate time coordinates, if not already the case
    if 'time' not in density.coords:
        density.coords['time'] = uni.get_times_array()

    # If there are no sizes to be plotted, can directly do the line plot
    if sizes_from is None:
        hlpr.ax.plot(density.coords['time'], density, **plot_kwargs)
        return
    
    # Otherwise, need to get the sizes and performs a scatter plot
    sizes = uni['data'][model_name][sizes_from] * size_factor

    hlpr.ax.scatter(density.coords['time'], density, s=sizes,
                    **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(**_density_hlpr_kwargs)
              )
def densities(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
              model_name: str, to_plot: dict, **common_plot_kwargs):
    """Like density, but for several specifications given by the `to_plot`.

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The name of the model the data resides in, i.e. the
            base path within the UniverseGroup.
        to_plot (dict): Which data to plot the densities of. The keys of this
            dict are used as `path_to_data` for the `density` function. The
            values are unpacked and passed to `density`
        **common_plot_kwargs: Passed along to the `density` plot function for
            all calls. Note that this may not contain any keys that are given
            within `to_plot`!
    """
    for path_to_data, specs in to_plot.items():
        density(dm, uni=uni, hlpr=hlpr, model_name=model_name,
                path_to_data=path_to_data, **specs, **common_plot_kwargs)

    if len(to_plot) > 1:
        hlpr.provide_defaults('set_legend', use_legend=True, loc='best')
