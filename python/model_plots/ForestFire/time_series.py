"""ForestFire model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

from ..tools import save_and_close

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="cluster size A", y="$N_A$"),
                set_scale=dict(x='log', y='log'),
                set_title=dict(title='Size distribution of clusters')
                )
              )

def cluster_distribution(dm: DataManager, *, 
                         uni: UniverseGroup, 
                         hlpr: PlotHelper,
                         fmt: str=None, 
                         time: int=-1, 
                         save_kwargs: dict=None, 
                         plot_kwargs: dict=None):
    """Calculates the size distribution of all tree clusters and plots 
        a cumulative histogram
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving        
        fmt (str, optional): the plt.plot format argument
        time (int, optional): timestep of output data. default: last step
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        plot_kwargs (dict, optional): Passed on to plt.plot
    
    Raises:
        TypeError: Description
    """
    # Get the group that all datasets are in
    grp = uni['data/ForestFire']

    # get the cluster_id data
    try:
        data = grp['cluster_id'].isel(time=time).stack(grid=['x', 'y']).values
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

