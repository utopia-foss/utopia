"""ContDisease-model specific plot function for state-based figures"""

import matplotlib.pyplot as plt

from utopya.eval import (
    DataManager,
    PlotHelper,
    UniverseGroup,
    UniversePlotCreator,
    is_plot_func,
)

from ..tools import colorline

# -----------------------------------------------------------------------------


@is_plot_func(creator_type=UniversePlotCreator)
def phase_diagram(
    dm: DataManager,
    *,
    uni: UniverseGroup,
    hlpr: PlotHelper,
    x: str,
    y: str,
    cmap: str = None,
    **plot_kwargs,
):
    """Plots the the phase diagram of tree and infected tree densities

    .. TODO::

        Migrate to use DAG framework

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
    densities = uni["data/ContDisease/densities"]

    # If a colormap was given, use it to color-code the time
    cc_kwargs = (
        {} if cmap is None else dict(c=densities.coords["time"], cmap=cmap)
    )

    # Plot the phase space
    sc = hlpr.ax.scatter(
        densities.sel(kind=x),
        densities.sel(kind=y),
        **cc_kwargs,
        **plot_kwargs,
    )

    # Add a colorbar
    if cc_kwargs:
        hlpr.fig.colorbar(
            sc,
            ax=hlpr.ax,
            fraction=0.05,
            pad=0.02,
            label="Time [Iteration Steps]",
        )
