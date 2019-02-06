"""SandPile-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager, UniverseGroup
from ..tools import save_and_close

# -----------------------------------------------------------------------------

def slope(  dm: DataManager, *, 
            uni: UniverseGroup, 
            out_path: str, 
            save_kwargs: dict=None, 
            **plot_kwargs):
    """Calculates the mean slope minus the critical slope and performs a scatterplot.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        out_path (str): Where to store the plot to
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/SandPile']

    # Extract the critical slope from the model configuration
    critical_slope = uni['cfg']['SandPile']['critical_slope']

    # Extract the y data which is the mean slope averaged over all grid cells 
    # for each time step
    y_data = [np.mean(s) - critical_slope for s in grp['slope']]

    # Call the plot function
    plt.plot(y_data, **plot_kwargs)

    # Add labels to the figure
    plt.xlabel('iteration step')
    plt.ylabel(r'slope $\langle n \rangle - n_c$')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)



def compl_cum_prob_dist(dm: DataManager, *, 
                        uni: UniverseGroup, 
                        out_path: str, 
                        save_kwargs: dict=None, 
                        **plot_kwargs):
    """Calculates the complementary cumulative probability distribution and 
    performs a logarithmic scatter plot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The selected universe data
        out_path (str): Where to store the plot to
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = uni['data/SandPile']

    ### Extract the y data 
    # Get the avalanche data averaged over all grid cells for each time step
    y_data = [np.sum(d) for d in grp['avalanche']]

    # Remove the first element, ...
    y_data.pop(0)
    # ... count the size of the avalanches, ...
    y = np.bincount(y_data)
    # ... and cummulatively sum them up
    y = (np.cumsum(y[::-1])[::-1])[1:]
    
    # Get the index 
    index = (y - np.roll(y, 1)) != 0

    # Normalize the cummulated counts
    y = y / y[0]

    # Calculate the logarithmic values
    y_data = np.log10(np.arange(len(y)) + np.min(y_data))[index], np.log10(y)[index]

    # Call the plot function
    plt.plot(*y_data, **plot_kwargs)

    # Set labels
    plt.xlabel(r'$\log_{10}(A)$')
    plt.ylabel(r'$\log_{10}(P_A(A))$')

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)