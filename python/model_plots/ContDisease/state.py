"""ContDisease-model specific plot function for spatial figures"""

import numpy as np
import matplotlib.pyplot as plt
import utopya.plot_funcs.ca
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator


from ..tools import save_and_close, colorline

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="Time", y="Density"),
                  set_limits=dict(y=[0, 1])
              )
              )
def tree_density(dm: DataManager, *,
                 uni: UniverseGroup,
                 hlpr: PlotHelper,
                 **plot_kwargs):
    """Plots the density of trees and perfoms a lineplot

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the tree states
    data = grp["densities/tree"]

    # Get the time steps array for this universe
    times = uni.get_times_array()

    # Call the plot function
    plt.plot(times, data, **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="Time", y="Density"),
                  set_limits=dict(y=[0, 1]),
                  set_legend=dict(enabled=True, loc='best')
              )
              )
def densities(dm: DataManager, *,
              uni: UniverseGroup,
              hlpr: PlotHelper,
              **plot_kwargs):
    """Plots the densities of all cell states

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/ContDisease']

    # Extract the data for the tree states and convert it into a 3d-array
    d_empty = grp["densities/empty"]
    d_tree = grp["densities/tree"]
    d_infected = grp["densities/infected"]
    d_source = grp["densities/source"]
    d_stone = grp["densities/stone"]

    # Get the time steps array for the given universe
    times = uni.get_times_array()

    # Expand the stone and source arrays (both constant) to be of valid length
    d_stone = np.full(len(times), d_stone)
    d_source = np.full(len(times), d_source)

    # Call the plot function
    plt.plot(times, d_empty, color='black', label='empty', **plot_kwargs)
    plt.plot(times, d_tree, color='green', label='tree', **plot_kwargs)
    plt.plot(times, d_infected, color='red', label='infected', **plot_kwargs)
    plt.plot(times, d_source, color='orange', label='source', **plot_kwargs)
    plt.plot(times, d_stone, color='gray', label='stone', **plot_kwargs)


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="Tree density", y="Infected density"),
                  set_limits=dict(x=[0, 1], y=[0, 1])
              )
              )
def phase_diagram(dm: DataManager, *,
                  uni: UniverseGroup,
                  hlpr: PlotHelper,
                  x: str='tree',
                  y: str='infected',
                  **lc_kwargs):
    """Plots the the phase diagram of tree and infected tree densities

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        x (str, optional): What to plot on the x-axis
        y (str, optional): What to plot on the y-axis
        **lc_kwargs: Passed on to LineCollection.__init__
    """
    # Get the group that all datasets are in
    densities = uni['data/ContDisease/densities']

    # Call the plot function
    colorline(densities[x], densities[y], **lc_kwargs)