"""CopyMe-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt

from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def state_mean(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    for uni in dm['uni']:

        # Get the group that all datasets are in
        grp = dm['uni'][uni]['data/Savanna']

        # Get the shape of the data
        uni_cfg = dm['uni'][uni]['cfg']
        num_steps = uni_cfg['num_steps']
        grid_size = uni_cfg['Savanna']['grid_size']

        # Extract the y data which is 'some_state' avaraged over all grid cells for every time step
        G_data = grp['density_G']#.reshape(grid_size[0], grid_size[1], num_steps+1)
        #G_mean = [np.mean(d) for d in G_data]
        S_data = grp['density_S']#.reshape(grid_size[0], grid_size[1], num_steps+1)
        #S_mean = [np.mean(d) for d in S_data]
        T_data = grp['density_T']#.reshape(grid_size[0], grid_size[1], num_steps+1)
        #T_mean = [np.mean(d) for d in T_data]

        # Assemble the arguments
        #args = [G_data, S_data, T_data]
        #if fmt:
        #    args.append(fmt)

        # Call the plot function
        #plt.plot(*args, **plot_kwargs)
        plt.plot(G_data, "green")
        plt.plot(S_data, "blue")
        plt.plot(T_data, "black")

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)

def state_final(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    # Get the group that all datasets are in
    grp = dm['uni'][uni]['data/Savanna']

    # Get the shape of the data
    uni_cfg = dm['uni'][uni]['cfg']
    num_steps = uni_cfg['num_steps']
    grid_size = uni_cfg['Savanna']['grid_size']

    # Extract the y data which is 'some_state' avaraged over all grid cells for every time step
    G_data = grp['density_G'].reshape(grid_size[0], grid_size[1], num_steps+1)
    print(np.shape(G_data[:,:,0]))
    #G_mean = [np.mean(d) for d in G_data]
    S_data = grp['density_S'].reshape(grid_size[0], grid_size[1], num_steps+1)
    #S_mean = [np.mean(d) for d in S_data]
    T_data = grp['density_T'].reshape(grid_size[0], grid_size[1], num_steps+1)
    #T_mean = [np.mean(d) for d in T_data]

    # Assemble the arguments
    #args = [G_data, S_data, T_data]
    #if fmt:
    #    args.append(fmt)

    # Call the plot function
    #plt.plot(*args, **plot_kwargs)
    time = num_steps
    fig, ax = plt.subplots()
    for i in range(grid_size[0]):
        for j in range(grid_size[1]):
            plt.scatter(i,j,c=(G_data[i,j,time],S_data[i,j,time],T_data[i,j,time]))

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)

def state_grass_bifurcation(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Calculates the state mean and performs a lineplot
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    for uni in dm["uni"]:
        # Get the group that all datasets are in
        grp = dm['uni'][uni]['data/Savanna']

        # Get the shape of the data
        uni_cfg = dm['uni'][uni]['cfg']
        num_steps = uni_cfg['num_steps']
        grid_size = uni_cfg['Savanna']['grid_size']
        beta = uni_cfg['Savanna']['beta']

        # Extract the y data which is 'some_state' avaraged over all grid cells for every time step
        G_data = grp['density_G'].reshape(grid_size[0], grid_size[1], num_steps+1)
        G = G_data[0,0,num_steps]
        S_data = grp['density_S'].reshape(grid_size[0], grid_size[1], num_steps+1)
        S = S_data[0,0,num_steps]
        T_data = grp['density_T'].reshape(grid_size[0], grid_size[1], num_steps+1)
        T = T_data[0,0,num_steps]

        plt.scatter(beta,G,c='green')
        plt.scatter(beta,T,c='black',s=5)

    # Call the plot function
    #plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
