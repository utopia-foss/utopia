"""ForestFire model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def state_mean(dm: DataManager, *, out_path: str, uni: UniverseGroup,
               save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The selected universe data
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the raw data of the state, either 0 (empty) or 1 (tree)
    data = uni['data']['ForestFire/state']

    # Calculate the mean along the time axis (iteration axis)
    mean_state = [np.mean(d) for d in data]

    # Get the times array
    times = uni.get_times_array()

    # Call the plot function
    plt.plot(times, mean_state, **plot_kwargs)

    plt.xlabel('Time [steps]')
    plt.ylabel('Tree density')

    plt.xlim(left=0, right=np.max(times))
    plt.ylim(bottom=0)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)


# -----------------------------------------------------------------------------

def cluster_distribution(dm: DataManager, *, 
                         out_path: str, 
                         uni: UniverseGroup, 
                         fmt: str=None, 
                         time: int=-1, 
                         save_kwargs: dict=None, 
                         plot_kwargs: dict=None):
    """Calculates the size distribution of all tree clusters and plots a cumulative histogram
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The selected universe data
        fmt (str, optional): the plt.plot format argument
        time (int, optional): timestep in output data for plot. default: last step
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        plot_kwargs (dict, optional): Passed on to plt.plot
    
    Raises:
        TypeError: Description
    """
    # Get the group that all datasets are in
    grp = uni['data/ForestFire']

    # get the cluster_id data
    try:
        data = grp['cluster_id'][time,:].flatten()
    except:
        raise TypeError("Argument time needs to be a int within " 
                        "the number of timesteps or -1. "
                        "Was: {} with value: '{}'"
                        "".format(type(time), time))

    # get the cluster sizes from ids
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
    if fmt:
        args.append(fmt)
    
    # Call the plot function
    plt.plot(*args, **plot_kwargs)

    # Set plot properties
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('cluster size A')
    plt.ylabel('$N_A$')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
