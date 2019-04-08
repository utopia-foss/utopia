"""ContDisease-model specific plot function for spatial figures"""

import numpy as np
import matplotlib.pyplot as plt
import utopya.plot_funcs.ca
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator


from ..tools import colorline

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="Time [Iteration Steps]", y="Density [1/A]"),
                  set_title=dict(title="Densities"),
                  set_limits=dict(y=[0, 1], x=['min', 'max']),
                  set_legend=dict(enabled=True, loc='best'),
              )
              )
def densities(dm: DataManager, *,
              uni: UniverseGroup,
              hlpr: PlotHelper,
              to_plot: dict,
              **plot_kwargs):
    """Plots the densities of all cell states

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        to_plot (dict): A dictionary with the information on what to plot.
            The keys are used to access the datasets and the values, which 
            should again be given as dict's are passed onto to the plt.plot
            function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all density datasets are in
    grp = uni['data/ContDisease/densities']

    # Get the time steps array for the given universe
    times = uni.get_times_array()

    # Loop through all entries and create a line plot for each one of them
    for key, value in to_plot.items():
        # Get the dataset
        data = grp[key]

        # If stones or sources should be plotted, expand their constant 1D 
        # datasets to 2D to be of valid length
        if key == 'stone' or key == 'source':
            data = np.full(len(times), data)

        # Call the plot function
        plt.plot(times, data, **value)


@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_title=dict(title="Phase Diagram")
              )
              )
def phase_diagram(dm: DataManager, *,
                  uni: UniverseGroup,
                  hlpr: PlotHelper,
                  x: str,
                  y: str,
                  cmap: str=None,
                  **plot_kwargs):
    """Plots the the phase diagram of tree and infected tree densities

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        x (str): What to plot on the x-axis
        y (str): What to plot on the y-axis
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    densities = uni['data/ContDisease/densities']

    # If a colormap was given, use it to color-code the time
    cc_kwargs = ({} if cmap is None
                 else dict(c=uni.get_times_array(), cmap=cmap))
    
    # Plot the phase space
    sc = hlpr.ax.scatter(densities[x], densities[y], **cc_kwargs, **plot_kwargs)

    # Add a colorbar
    if cc_kwargs:
        hlpr.fig.colorbar(sc, ax=hlpr.ax, fraction=0.05, pad=0.02,
                          label="Time [Iteration Steps]")
