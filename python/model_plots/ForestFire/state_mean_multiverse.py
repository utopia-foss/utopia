import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup, MultiverseGroup
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator

@is_plot_func(creator_type=MultiversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time", y="Density"),
                set_scale=dict(x='linear', y='log'),
                set_title=dict(title='Mean tree density'),
                set_legend=dict(use_legend=True),
                save_figure=dict(bbox_inches="tight")
                )
              )
def mean_and_std(dm: DataManager, *,
                 mv_data: xr.Dataset,
                 hlpr: PlotHelper,
                 mean_of: str=None,
                 lines_from: str=None,
                 plot_kwargs: dict=None):
    '''Plots the mean and standard deviation of multiverses'''

    if 'mean_density' in mv_data:
        mv_data = mv_data['mean_density']

    # Calculate the mean over all dimensions except for the time
    mv_data.mean(dim=[d for d in mv_data.dims if d in mean_of])

    for line in mv_data[lines_from]:
        # Select the correct data that results in one line
        data_sel = mv_data.sel(**{line.name:line})

        # Get the averaged data
        mean = data_sel['mean']
        if 'std' in data_sel:
            std = data_sel['std']
        else:
            std = None

        # Need to create a dictionary for the plot kwargs if none is given
        if plot_kwargs is None:
            plot_kwargs = {} 

        # Get the values of the dimension from which to create the lines
        plot_kwargs['label'] = '${}$'.format(line.values)

        # Plot the data
        hlpr.ax.errorbar(mean['time'], mean, yerr=std, **plot_kwargs)

        # As default use the dimension name from which the lines originate
        hlpr.provide_defaults('set_legend', title='{}'.format(line.name))
