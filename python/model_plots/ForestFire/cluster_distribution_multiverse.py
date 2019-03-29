import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x='cluster size A', y='$N_A$'),
                set_scale=dict(x='log', y='log'),
                set_title=dict(title='Distribution of Cluster sizes')
                )
              )

def cluster_distribution_multiverse(dm: DataManager, *,
                                    mv_data: xr.Dataset,
                                    hlpr: PlotHelper,
                                    time: int=-1,
                                    save_kwargs: dict=None,
                                    plot_kwargs: dict=None):
    '''Plots the cluster distribution for multiple universes'''
    
    cluster_data = mv_data['cluster_id'][{'time': time}]

    for lightning in range(len(cluster_data['lightning_frequency'])):
        data = cluster_data[{'lightning_frequency': lightning}].values.flatten()

        #get the cluster sizes from ids
        cluster_sizes = np.zeros(np.amax(data), int)
        for id in data[data > 0]:
            cluster_sizes[id-1] += 1
        
        # make histogram of cluster_sizes
        size_max = np.amax(cluster_sizes)
        histogram = np.zeros(size_max, int)
        for size in cluster_sizes:
            histogram[size-1] += 1

        # make cumulative histogram
        for i in reversed(range(1,len(histogram))):
            histogram[i-1] += histogram[i]

        # Assemble the arguments
        args = [ np.linspace(1,size_max, len(histogram)), histogram ]
    
        # Call the plot function
        label=('%.2E' % float(cluster_data['lightning_frequency'][lightning]))
        plot_kwargs['label'] = 'lightning frequency = {}'.format(label)   

        #Possibly calculate average Cluster size --- NECESSARY?
        #ACS = ...

        hlpr.ax.plot(*args, **plot_kwargs)

    plt.legend(loc = 'upper right', prop={'size': 9})



