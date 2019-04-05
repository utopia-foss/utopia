"""SandPile-model specific plot function for the state"""

import logging
from typing import Tuple

import numpy as np
import xarray as xr

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                set_limits=dict(x=[0., None]),
                set_labels=dict(x="Time [Iteration Steps]",
                                y=r"Slope $\langle n \rangle - n_c$")
              ))
def mean_slope(dm: DataManager, *, uni: UniverseGroup, hlpr: PlotHelper,
               show_critical_slope_value: bool=False, **plot_kwargs):
    """Calculates the mean slope (averaged over the whole grid) and plots it
    against time. The mean slope is further reduced by the constant value for
    the critical slope.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        show_critical_slope_value (bool, optional): Whether to add a text box
            with the critical slope value into the top-right hand corner
        **plot_kwargs: Passed on to plt.plot
    """
    # Extract the critical slope value from the model configuration
    critical_slope = uni['cfg']['SandPile']['critical_slope']

    # Calculate the slope averaged over all grid cells for each time step
    mean_slope = uni['data/SandPile/slope'].mean(dim=('x', 'y'))

    # If there are no time coordinates for this, use the universe time data
    if 'time' not in mean_slope.coords:
        mean_slope.coords['time'] = uni.get_times_array()

    # Plot the mean normalised slope over time
    hlpr.ax.plot(mean_slope.coords['time'],
                 mean_slope - critical_slope,
                 **plot_kwargs)

    # Provide info on the value of the critical slope
    if show_critical_slope_value:
        hlpr.ax.text(1, 1, "$n_c = {}$".format(critical_slope),
                     transform=hlpr.ax.transAxes,
                     verticalalignment='top', horizontalalignment='right',
                     fontdict=dict(fontsize="small"),
                     bbox=dict(facecolor="white", linewidth=1.))
    
    
@is_plot_func(creator_type=UniversePlotCreator)
def area_fraction(dm: DataManager, *, uni: UniverseGroup, **kwargs):
    """This plot function wraps the .time_series.density function and
    calculates and stores the area fraction values in the universe group such
    that they can be used for 
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        **kwargs: Passed on to utopya.plot_funcs.time_series.density
    """
    # Get the base group and the avalanche dataset
    grp = uni['data/SandPile']
    avalanche = grp['avalanche']

    # Compute and store the relative avalanche sizes

