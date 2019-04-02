
"""This module provides plotting functions to visualize distributions."""

from ._setup import *

import logging

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator, MultiversePlotCreator

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x=r"$\log_{10}(A)$",
                                  y=r"$\log_{10}(P_A(A))$"),
                  save_figure=dict(bbox_inches="tight")
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


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x=r"$\log_{10}(A)$",
                                  y=r"$\log_{10}(N_A)$"),
                  save_figure=dict(bbox_inches="tight")
              )
            )
def cluster_size_dist(dm: DataManager, *, 
                      uni: UniverseGroup, 
                      hlpr: PlotHelper,
                      model_name: str,
                      path_to_data: str,
                      **plot_kwargs):
    """Calculates the cluster size distribution and performs a logarithmic 
    scatter plot
    
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

    # Calculate a mask that will be applied to the data to erase multiple
    # occurrences of the same data-point pair
    mask = (cumsum - np.roll(cumsum, 1)) != 0

    # Plot the data
    hlpr.ax.plot(np.log10(unique[mask]), 
                 np.log10(cumsum[mask]), 
                 **plot_kwargs)
