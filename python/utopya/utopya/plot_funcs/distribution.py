
"""This module provides plotting functions to visualize distributions."""

import logging
from typing import Tuple

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator, MultiversePlotCreator

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="Values", y="Counts")
              )
            )
def histogram(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
              model_name: str, path_to_data: str, sum_over: Tuple[str]=(),
              only_unique: bool=False, **plot_kwargs):
    """Calculates a histogram from the data and plots it.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The model name that the data resides in
        path_to_data (str): The path to the data relative to the model data
            output
        sum_over (Tuple[str], optional): Which dimensions to calculate sums
            over. If not given, no sums are calculated.
        only_unique (bool, optional): Whether to only plot unique values, i.e.
            if true, this filters out multiple occurrences of the same data
            point and only leaves the first one.
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the data
    data = uni['data'][model_name][path_to_data]
    # TODO Allow slicing here?!

    # Sum over all specified values, then calculate the histogram
    unique, counts = np.unique(data.sum(dim=sum_over), return_counts=True)

    # Cumulatively sum up the counts and reverse the ordering and remove the 
    # first entry because there are no avalanches with size zero
    cumsum = (np.cumsum(counts[::-1])[::-1])

    # Calculate a mask that will be applied to the data to erase multiple
    # occurrences of the same data-point pair
    if only_unique:
        mask = (cumsum - np.roll(cumsum, 1)) != 0
    else:
        mask = np.ones(unique.shape, dtype=bool)

    # Plot the data
    hlpr.ax.plot(unique[mask], cumsum[mask], **plot_kwargs)

    # Provide some information to the helper
    # hlpr.provide_defaults('set_title',
    #                       title="Histogram of '{}'".format(path_to_data))
    # TODO Make this work recessively, i.e. that it does not dominate over
    #      any values given via the plot configuration.

    # TODO Consider adding log-log option here?


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x=r"$\log_{10}(A)$",
                                y=r"$\log_{10}(P_A(A))$")
                )
              )
def compl_cum_prob_dist(dm: DataManager, *, 
                        uni: UniverseGroup, 
                        hlpr: PlotHelper,
                        model_name: str,
                        path_to_data: str,
                        **plot_kwargs):
    """Calculates the complementary cumulative probability distribution and 
    performs a logarithmic scatter plot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        model_name (str): The model name
        path_to_data (str): The path to the data relative to the model data
            output
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the data, remove the initial time step 
    data = uni['data'][model_name][path_to_data][1:]

    # Sum over all values from all dimensions except for the time
    areas = data.sum(dim=[d for d in data.dims if d != 'time'])

    # Count the avalanche sizes
    unique, counts = np.unique(areas, return_counts=True)

    # Cumulatively sum up the sizes and reverse the ordering and remove the 
    # first entry because there are no avalanches with size zero
    cumsum = (np.cumsum(counts[::-1])[::-1])

    # Normalize the accumulated counts to get the normalized complementary
    # cumulative area distribution
    cumsum_norm = cumsum / cumsum[0]
 
    # Calculate a mask that will be applied to the data to erase multiple
    # occurrences of the same data-point pair
    mask = (cumsum_norm - np.roll(cumsum_norm, 1)) != 0

    # Plot the data
    hlpr.ax.plot(np.log10(unique[mask]), 
                 np.log10(cumsum_norm[mask]), 
                 **plot_kwargs)
