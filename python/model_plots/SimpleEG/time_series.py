"""SimpleEG-model specific plot function for time-series"""

import logging
from typing import Union

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager

from ..tools import save_and_close

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def frequency(dm: DataManager, *, out_path: str, uni: int, strategy: Union[str, list] = ['S0', 'S1'], save_kwargs: dict=None, **plot_kwargs):
    """Calculates the frequency of a given strategy and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        strategy (Union[str, list], optional): The strategy to plot
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    
    Raises:
        TypeError: For invalid strategy argument
    """
    # Get the group that all datasets are in
    grp = dm['uni'][uni]['data/SimpleEG']

    # Extract the data of the frequency
    strategy_data = grp['strategy']
    num_cells = len(strategy_data[0])
    frequencies = [np.bincount(s) / num_cells for s in strategy_data]

    # Get the frequencies of the desired strategy and plot it
    # Single strategy
    if isinstance(strategy, str):
        y_data = [f[int(strategy[-1])] for f in frequencies]

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
