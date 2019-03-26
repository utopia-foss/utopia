import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager
from utopya.datagroup import UniverseGroup

import logging
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def average_state_over_time(dm: DataManager, *, 
                            out_path: str, 
                            uni: UniverseGroup,
                            save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (UniverseGroup): The universe to use
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the dataset
    dset = uni['data']['Vegetation/plant_mass']

    # Extract the y data which is 'state' avaraged over all grid cells for every time step
    y_data = np.mean(dset, axis=(1, 2))

    # Call the plot function
    plt.plot(y_data, **plot_kwargs)

    # Add some labels
    plt.xlabel("Time")
    plt.ylabel("Plant Mass")

    plt.xlim(left=0)
    plt.ylim(bottom=0)

    # Save and close figure
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()
