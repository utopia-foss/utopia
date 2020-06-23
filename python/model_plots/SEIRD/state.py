"""SEIRD-model specific plot function for spatial figures"""

import copy

import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator
from utopya.tools import recursive_update


# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator)
def phase_diagram(dm: DataManager, *,
                  uni: UniverseGroup,
                  hlpr: PlotHelper,
                  x: str,
                  y: str,
                  cmap: str=None,
                  cbar_kwargs: dict=None,
                  **plot_kwargs):
    """Plots the the phase diagram using data from ``SEIRD/densities`` and
    color-codes the time.
    For ``x`` and ``y``, choose as coordinates the names of the SEIRD model
    cell states, e.g. ``infected``, ``susceptible``.

    Args:
        dm (DataManager): The data manager
        uni (UniverseGroup): The universe data to use
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        x (str): Which ``kind`` of density to plot on the x-axis
        y (str): Which ``kind`` of density to plot on the y-axis
        cmap (str, optional): Which color map to use for encoding the time
            dimension.
        cbar_kwargs (dict, optional): Additional arguments that are passed on
            to the plt.colorbar function.
        **plot_kwargs: Passed on to plt.scatter
    """
    # Get the group that all datasets are in
    densities = uni['data/SEIRD/densities']

    # If a colormap was given, use it to color-code the time
    cc_kwargs = ({} if cmap is None
                 else dict(c=densities.coords['time'], cmap=cmap))

    # Plot the phase space
    sc = hlpr.ax.scatter(densities.sel(kind=x),
                         densities.sel(kind=y),
                         **cc_kwargs, **plot_kwargs)

    if not cc_kwargs:
        return

    # else: Prepare arguments for colorbar, then add it
    cbar_kwargs = cbar_kwargs if cbar_kwargs else {}
    cbar_defaults = dict(fraction=.05, pad=.02, label="Time [Iteration Steps]")
    cbar_kwargs = recursive_update(cbar_defaults, cbar_kwargs)

    hlpr.fig.colorbar(sc, ax=hlpr.ax, **cbar_kwargs)
