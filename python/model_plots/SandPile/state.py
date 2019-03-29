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
        path_to_data (str): The path to the data
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