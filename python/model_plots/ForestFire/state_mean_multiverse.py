import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup, MultiverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time", y="Density"),
                set_scale=dict(x='linear', y='log'),
                set_title=dict(title='Mean tree density')
                )
              )

def state_mean_multiverse(  dm: DataManager, *,
                            mv_data: xr.Dataset,
                            hlpr: PlotHelper,
                            save_kwargs: dict=None,
                            plot_kwargs: dict=None):
    '''Plots the cluster distribution for multiple universes'''

    state_data = mv_data['mean_density']

    for uni in dm['multiverse'].values():
        times = uni.get_times_array()
        # TODO Find a cleaner way to do this (on a general level)
        break

    for lightning in range(len(state_data['lightning_frequency'])):

        #Take mean of the states for each time step and convert it
        mean = mv_data[{'lightning_frequency': lightning}]
        mean = mean.to_array().values.flatten()

        #Set the labels
        label=('%.2E' % float(state_data['lightning_frequency'][lightning]))
        plot_kwargs['label'] = 'lightning frequency = {}'.format(label)

        # Call the plot function
        hlpr.ax.set_xlim(left=0, right=np.max(times))
        hlpr.ax.set_ylim(bottom=0)
        hlpr.ax.plot(times, mean, **plot_kwargs)

    plt.legend(loc='upper right', prop={'size': 9})
    #plt.tight_layout()
