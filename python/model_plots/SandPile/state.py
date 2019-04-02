"""SandPile-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt
import xarray as xr
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator, MultiversePlotCreator
from ..tools import save_and_close

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x="Iteration step",
                                  y=r"Slope $\langle n \rangle - n_c$"),
                  save_figure=dict(bbox_inches="tight")
              )
            )
def slope(  dm: DataManager, *, 
            uni: UniverseGroup, 
            hlpr: PlotHelper,
            **plot_kwargs):
    """Calculates the mean slope minus the critical slope and performs a 
    scatterplot.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/SandPile']

    # Extract the critical slope from the model configuration
    critical_slope = uni['cfg']['SandPile']['critical_slope']

    # Extract the y data which is the mean slope averaged over all grid cells 
    # for each time step
    y_data = [np.mean(s) - critical_slope for s in grp['slope']]

    # Call the plot function
    hlpr.ax.plot(y_data, **plot_kwargs)
    

@is_plot_func(creator_type=MultiversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x=r"Time",
                                  y=r"Area fraction $A/l^2$"),
                  save_figure=dict(bbox_inches="tight"),
                  set_scale=dict(y="log"),
              )
            )
def area_fraction(dm: DataManager, *, 
                  uni: UniverseGroup, 
                  hlpr: PlotHelper,
                  path_to_data: str,
                  **plot_kwargs):
    """Plot the area fraction of the avalanches over time
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        uni (UniverseGroup): The selected universe data
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the data
    data = uni['data/SandPile'][path_to_data]

    # Calculate the areas for each time step by summing over all values 
    # from all dimensions except for the time 
    areas = data.sum(dim=[d for d in data.dims if d != 'time'])

    # Remove the first element, ...
    areas[1:]

    # ... and calculate the area fraction by normalizing the data by the number
    # of cells
    # NOTE This calculation requires the time to be the first dimension
    area_frac = areas / np.prod(areas.shape[1:])

    # Call the plot function, adjust marker size 's' to size of avalanche
    hlpr.ax.scatter(area_frac['time'], area_frac, **plot_kwargs)

    hlpr.provide_defaults('set_limits', y=[0.95, np.prod(data.shape[1:])])