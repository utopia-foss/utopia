import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup

def cluster_distribution_multiverse(dm: DataManager, *,
                                    out_path: str,                                    
                                    mv_data: xr.Dataset,
                                    time: int=-1,
                                    save_kwargs: dict=None,
                                    plot_kwargs: dict=None):
    '''Plots the cluster distribution for multiple universes'''
    
    fig = plt.figure()
    ax = fig.add_subplot(111)

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

        #Possibly calculate average Cluster size --- IS THIS NECESSARY?
        #ACS = ...

        ax.plot(*args, **plot_kwargs)

    # Set plot properties
    ax.set_title('Distribution of Cluster sizes')
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('cluster size A')
    ax.set_ylabel('$N_A$')
    plt.legend(loc = 'upper right', prop={'size': 9})

    # Save and close the figure
    plt.savefig(out_path)
    plt.close()

