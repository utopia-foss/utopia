import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup, MultiverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time averaged density", y="lightning probability"),
                #set_scale=dict(x='linear', y='log'),
                set_title=dict(title='Mean tree densitiy')
                )
              )

def temporal_mean_multiverse(dm: DataManager, *,
                            mv_data: xr.Dataset,
                            hlpr: PlotHelper,
                            save_kwargs: dict=None,
                            plot_kwargs: dict=None):
    '''Plots the asymptotic mean for multiple universes'''

    state_data = mv_data['mean_density']

    for uni in dm['multiverse'].values():
        times = uni.get_times_array()
        # TODO Find a cleaner way to do this (on a general level)
        break

    for lightning in range(len(state_data['lightning_probability'])):

        #Take mean of the states for each time step and convert it
        mean = mv_data[{'lightning_probability': lightning}].mean(dim=['dim_0'])
        mean = mean.to_array().values.flatten()

        light_freq = float(state_data['lightning_probability'][lightning])

        #Set the labels
        label=('%.2E' % light_freq)
        plot_kwargs['label'] = 'lightning probability = {}'.format(label)
        
        # Call the plot function
        hlpr.ax.set_yscale('log')
        hlpr.ax.plot(mean, light_freq, **plot_kwargs)

    #plt.legend(loc='upper right', prop={'size': 9})
    #plt.tight_layout()