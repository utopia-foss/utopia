
"""This module provides plotting functions to visualize distributions."""

import logging
from typing import Tuple, Union

import numpy as np

from .. import DataManager, UniverseGroup
from ..plotting import is_plot_func, PlotHelper, UniversePlotCreator
from ..dataprocessing import transform

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="Values", y="Counts")
              ))
def histogram(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
              model_name: str, path_to_data: str,
              transformations: Tuple[Union[dict, str]]=None,
              transformations_log_level: int=None,
              histogram_kwargs: dict=None,
              normalize: bool=False,
              cumulative: Union[bool, str]=False,
              use_unique: bool=False,
              mask_repeated: bool=False,
              bin_width_scale: float=1.,
              show_histogram_info: bool=True,
              pyplot_func_name: str='bar',
              **pyplot_func_kwargs):
    """Calculates a histogram from the data and plots it.
    
    This function is very versatile. Its capabilities range from a plain old
    histogram (only required arguments set) to the plot of a complementary
    cumulative probability distribution function.
    
    Don't despair. The documentation of arguments below should give a good
    idea of what each parameter does.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The model name that the data resides in
        path_to_data (str): The path to the data relative to the model data
            output
        transformations (Tuple[Union[dict, str]], optional): With the
            parameters specified here, multiple transformations can be applied
            to the data. This can be used for dimensionality reduction of the
            data, but also for other operations, e.g. to select a slice.
            The operations are carried out before calculating the
            histogram. For available parameters, see
            :py:func:`utopya.dataprocessing.transform`
        transformations_log_level (int, optional): With which log level to
            perform the transformations. Useful for debugging.
        histogram_kwargs (dict, optional): Passed to np.histogram. This can be
            used to adjust the number of `bins` or set the `range` the bins
            should be spread over; the latter also allows to pass a 2-tuple
            containing None, which will be resolved to data.min() or
            data.max(). See `np.histogram` documentation for other arguments.
        normalize (bool, optional): Whether to normalize the counts by the sum
            of the counts.
        cumulative (Union[bool, str], optional): Whether to make a cumulative
            histogram. If 'complementary', the complement of the cumulative
            sum is plotted.
        use_unique (bool, optional): If this option is set, will not do a
            regular histogram but count unique values.
        mask_repeated (bool, optional): In `use_unique` mode, will mask the
            counts such that repeated values are not shown.
        bin_width_scale (float, optional): Factor by which to scale bin widths
            in the bar plot.
        show_histogram_info (bool, optional): Whether to show an info box in
            the top right-hand corner
        pyplot_func_name (str, optional): The name of the matplotlib.pyplot
            function to use for plotting. By default, a bar plot is performed.
            For unique data, it might make more sense to do a line or scatter
            plot. Note that for the bar plot, the bar widths are automatically
            passed to the plot call and can not be adjusted.
        **pyplot_func_kwargs: The kwargs passed on to the pyplot function
            chosen via the `pyplot_func_name` argument.
    
    Raises:
        ValueError: When trying to make a bar plot with `use_unique` option
            enabled.
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]

    # Assuming this is an xr.DataArray now
    # Apply transformations, e.g. to reduce dimensionality
    if transformations:
        data = transform(data, *transformations,
                         log_level=transformations_log_level)

    # Calculate the histogram, either via np.histogram or np.unique
    if not use_unique:
        # Prepare histogram kwargs
        histogram_kwargs = histogram_kwargs if histogram_kwargs else {}

        # Replace None in `range` argument with min or max value
        if None in histogram_kwargs.get('range', []):
            rg = tuple(histogram_kwargs['range'])
            histogram_kwargs['range'] = (rg[0] if rg[0] is not None
                                         else float(data.min()),
                                         rg[1] if rg[1] is not None
                                         else float(data.max()))
            log.debug("Auto-determined histogram range to %s",
                      histogram_kwargs['range'])

        # Calculate the histogram
        counts, bin_edges = np.histogram(data, **histogram_kwargs)

        # Calculate bin positions, i.e. midpoint of bin edges
        bin_pos = bin_edges[:-1] + (np.diff(bin_edges) / 2.)

    else:
        if not np.issubdtype(data.dtype, np.integer):
            log.warning("Calculating a histogram via np.unique with data of "
                        "non-integer dtype %s might not lead to the desired "
                        "results! You should reconsider your choice of the "
                        "`use_unique` argument.", data.dtype)

        bin_pos, counts = np.unique(data, return_counts=True)

        # Mask repeated values
        if mask_repeated:
            mask = (counts - np.roll(counts, 1)) != 0

            bin_pos = bin_pos[mask]
            counts = counts[mask]

    # Might want to normalize
    if normalize:
        log.debug("Normalizing ...")
        counts = counts / np.sum(counts)

    # Can use cumulative or complementary cumulative values
    if cumulative is True:
        log.debug("Using cumulative sum for counts ...")
        counts = np.cumsum(counts)

    elif cumulative == 'complementary':
        log.debug("Using complementary cumulative sum for counts ...")
        counts = np.cumsum(counts[::-1])[::-1]

    # Might want to only plot a subset. For that, allow a slice operation

    # Plot the data using the given plot function name
    if pyplot_func_name == 'bar':
        if use_unique:
            raise ValueError("Cannot do a 'bar' plot with `use_unique` option "
                             "enabled! Select a different pyplot function by "
                             "setting `pyplot_func_name` to, e.g., 'plot'.")

        # Special case of bar plot, where the bar widths need be given
        hlpr.ax.bar(bin_pos, counts,
                    width=bin_width_scale * np.diff(bin_edges),
                    **pyplot_func_kwargs)

    else:
        plt_func = getattr(hlpr.ax, pyplot_func_name)
        plt_func(bin_pos, counts,
                 **pyplot_func_kwargs)

    # Add histogram information
    if show_histogram_info:
        # Generate an info string, containing total count and bin count and,
        # for the non-unique histogram: the range info
        if not use_unique:
            info_str = ("$N_{{data}} = {Ntot:}$\n"
                        "$N_{{bins}} = {Nbins:d}$\n"
                        "range: $[{rg_min:.3g},\ {rg_max:.3g}]$"
                        "".format(Ntot=data.size,
                                  Nbins=len(bin_pos),
                                  rg_min=bin_edges[0],
                                  rg_max=bin_edges[-1]))
            # NOTE .format operations can be escaped with '{' and '}'
        
        else:
            info_str = ("$N_{{data}} = {Ntot:}$\n"
                        "$N_{{unique}} = {Nunique:d}$"
                        "".format(Ntot=data.size,
                                  Nunique=len(bin_pos)))

        hlpr.ax.text(1, 1, info_str,
                     transform=hlpr.ax.transAxes,
                     verticalalignment='top', horizontalalignment='right',
                     fontdict=dict(fontsize="small"),
                     bbox=dict(facecolor="white", linewidth=1.))
    
