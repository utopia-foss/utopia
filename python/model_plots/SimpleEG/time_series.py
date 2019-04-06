"""SimpleEG-model specific plot function for time-series"""

import logging
from typing import Union

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager
from utopya.datagroup import UniverseGroup

from ..tools import save_and_close

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def frequency(dm: DataManager, *, uni: UniverseGroup, out_path: str, strategy: Union[str, list]=['S0', 'S1'], save_kwargs: dict=None, **plot_kwargs):
    """Calculates the frequency of a given strategy and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The universe from which to plot the data
        out_path (str): Where to store the plot to
        strategy (Union[str, list], optional): The strategy to plot
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid strategy argument
    """
    # Extract the data of the frequency
    strategy_data = uni['data']['SimpleEG']['strategy']

    grid_shape = strategy_data[0].shape 
    num_cells = grid_shape[0] * grid_shape[1]
    frequencies = [np.bincount(s.stack(grid=['x', 'y'])) / num_cells for s in strategy_data]

    # Get the frequencies of the desired strategy and plot it
    # Single strategy
    if isinstance(strategy, str):
        strategy_map = dict(S0=0, S1=1)
        y_data = [f[strategy_map[strategy]] for f in frequencies]

        # Create the plot
        plt.plot(y_data, label=strategy, **plot_kwargs)
    
    # Multiple strategies
    elif isinstance(strategy, list):
        for s in strategy:
            y_data = [f[int(s[-1])] for f in frequencies]

            # Create the plot
            plt.plot(y_data, label=s, **plot_kwargs)

    else:
        raise TypeError("Invalid argument 'strategy' of type {} and value "
                        "'{}'!".format(type(strategy), strategy))

    # Set general plot options
    plt.ylim(-0.05, 1.05)
    plt.xlabel('Iteration step')
    plt.ylabel('Frequency')
    plt.legend(loc='best')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
