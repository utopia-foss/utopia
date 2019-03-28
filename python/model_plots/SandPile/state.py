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
                                  y=r"slope $\langle n \rangle - n_c$"),
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
                        **plot_kwargs):
    """Calculates the complementary cumulative probability distribution and 
    performs a logarithmic scatter plot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/SandPile']

    ### Extract the y data 
    # Get the avalanche data averaged over all grid cells for each time step
    y_data = [np.sum(d) for d in grp['avalanche']]

    # Remove the first element, ...
    y_data.pop(0)
    # ... count the size of the avalanches, ...
    y = np.bincount(y_data)
    # ... and cummulatively sum them up
    y = (np.cumsum(y[::-1])[::-1])[1:]
    
    # Get the index 
    index = (y - np.roll(y, 1)) != 0

    # Normalize the cummulated counts
    y = y / y[0]

    # Calculate the logarithmic values
    y_data = np.log10(np.arange(len(y)) + np.min(y_data))[index], np.log10(y)[index]

    # Call the plot function
    hlpr.ax.plot(*y_data, **plot_kwargs)


@is_plot_func(creator_type=MultiversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x=r"$\log_{10}(A)$",
                                  y=r"$\log_{10}(P_A(A))$"),
                  save_figure=dict(bbox_inches="tight")
              )
            )
def mult_cum_prob(dm: DataManager, *,
               hlpr: PlotHelper,
               mv_data: xr.Dataset,
               **plot_kwargs):
    """This is the same as above, but for a multiverse run
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    number_of_universes = mv_data['avalanche'].shape[0]
    x=0
    for x in range(number_of_universes):
        ### Extract the y data 
        # Get the avalanche data averaged over all grid cells for each time step
        y_data = [np.sum(d) for d in mv_data['avalanche'][x,:,:,:]]

        # Remove the first element, ...
        y_data.pop(0)
        # ... count the size of the avalanches, ...
        y = np.bincount(y_data)
        # ... and cumulatively sum them up
        y = (np.cumsum(y[::-1])[::-1])[1:]
        
        # Get the index 
        index = (y - np.roll(y, 1)) != 0

        # Normalize the cumulated counts
        y = y / y[0]

        # Calculate the logarithmic values
        y_data = np.log10(np.arange(len(y)) + np.min(y_data))[index], np.log10(y)[index]

        # Call the plot function
        hlpr.ax.plot(*y_data, **plot_kwargs)
        
        x += 1


@is_plot_func(creator_type=MultiversePlotCreator,
              helper_defaults=dict(
                  set_labels=dict(x=r"time",
                                  y=r"Area fraction $A/l^2$"),
                  save_figure=dict(bbox_inches="tight")
              )
            )
def plot_area_frac_t(dm: DataManager, *, 
                        uni: UniverseGroup, 
                        hlpr: PlotHelper,
                        **plot_kwargs):
    """Calculates the complementary cumulative probability distribution and 
    performs a logarithmic scatter plot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        uni (UniverseGroup): The selected universe data
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/SandPile']
    grid_size = uni['cfg']['SandPile']['cell_manager']['grid']['resolution']
    
    ### Extract the y data 
    # Get the avalanche data averaged over all grid cells for each time step
    y_data = [np.sum(d) for d in grp['avalanche']]

    # Remove the first element, ...
    y_data.pop(0)

    #convert to numpy.array to do arithmetics
    y=np.array(y_data)

    #normalise by total area
    y=y/(grid_size*grid_size)

    # Call the plot function, adjust marker size 's' to size of avalanche
    hlpr.ax.scatter(range(len(y_data)),y, s=25*y, **plot_kwargs)