import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup, MultiverseGroup

def temporal_mean_multiverse(dm: DataManager, *,
                            out_path: str,                                    
                            mv_data: xr.Dataset,
                            save_kwargs: dict=None,
                            plot_kwargs: dict=None):
    '''Plots the cluster distribution for multiple universes'''

    fig = plt.figure()
    ax = fig.add_subplot(111)

    state_data = mv_data['mean_density']

    for uni in dm['multiverse'].values():
        times = uni.get_times_array()
        # TODO Find a cleaner way to do this (on a general level)
        break

    for lightning in range(len(state_data['lightning_frequency'])):

        #Take mean of the states for each time step and convert it
        mean = mv_data[{'lightning_frequency': lightning}].mean(dim=['time'])
        mean = mean.to_array().values.flatten()

        #Set the labels
        label=('%.2E' % float(state_data['lightning_frequency'][lightning]))
        plot_kwargs['label'] = 'lightning frequency = {}'.format(label)
        
        # Call the plot function
        ax.plot(mean, float(state_data['lightning_frequency'][lightning]), **plot_kwargs)

    # Set plot properties
    ax.set_title('Mean tree densitiy')
    ax.set_xlabel('Time averaged density')
    ax.set_ylabel('lightning probability')
    ax.set_yscale('log')
    plt.legend(loc='upper right', prop={'size': 9})
    plt.tight_layout()

    # Save and close the figure
    plt.savefig(out_path)
    plt.close()