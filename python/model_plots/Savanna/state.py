"""CopyMe-model specific plot function for the state"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

from utopya import DataManager

from ..tools import save_and_close

# -----------------------------------------------------------------------------

def state_time(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Performs a lineplot on the state 
    
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
    G_data = grp['density_G']
    T_data = grp['density_T']
    F_data = grp['density_F']
    S_data = np.empty(G_data.shape)
    for i in range(G_data.shape[0]):
        for j in range(G_data.shape[1]):
            S_data[i][j] = 1-G_data[i][j]-T_data[i][j]-F_data[i][j]

    # Call the plot function
    plt.plot(G_data, "green",label="grass")
    plt.plot(S_data, "yellow",label="sappling")
    plt.plot(T_data, "orange",label="tree")
    plt.plot(F_data, "red",label="forest tree")
    plt.legend()

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)

def state_trajectory(dm: DataManager, *, out_path: str, uni: int, fmt: str=None, save_kwargs: dict=None, **plot_kwargs):
    """Performs a lineplot on the state 
    
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
    G_data = grp['density_G']
    T_data = grp['density_T']
    F_data = grp['density_F']
    S_data = np.empty(G_data.shape)
    for i in range(G_data.shape[0]):
        for j in range(G_data.shape[1]):
            S_data[i][j] = 1-G_data[i][j]-T_data[i][j]-F_data[i][j]

    fig = plt.figure()
    ax = fig.gca(projection='3d')



    forest_exists = False
    for f in F_data[:,0]:
        if (f != 0):
            forest_exists = True
    if (forest_exists):
        ax.scatter(G_data[:,0],T_data[:,0],F_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis)
        ax.scatter(G_data[:,0],T_data[:,0],np.zeros(G_data.shape[0]),c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.scatter(G_data[:,0],np.ones(G_data.shape[0]) ,F_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.scatter(np.zeros(G_data.shape[0]),T_data[:,0],F_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.set_xlabel("Grass")
        ax.set_ylabel("Trees")
        ax.set_zlabel("Forest")
    else:
        ax.scatter(G_data[:,0],S_data[:,0],T_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis)
        ax.scatter(G_data[:,0],S_data[:,0],np.zeros(G_data.shape[0]),c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.scatter(G_data[:,0],np.ones(G_data.shape[0]) ,T_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.scatter(np.zeros(G_data.shape[0]),S_data[:,0],T_data[:,0],c=range(G_data.shape[0]),cmap=plt.cm.viridis,alpha=0.3)
        ax.set_xlabel("Grass")
        ax.set_ylabel("Saplings")
        ax.set_zlabel("Trees")
    ax.set_xlim(0,1)
    ax.set_ylim(0,1)
    ax.set_zlim(0,1)

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
    #print(grp['density_G'])
    # Extract the y data which is 'some_state' avaraged over all grid cells for every time step
    G_data = grp['density_G'].reshape(num_steps+1, grid_size[0], grid_size[1])
    #G_mean = [np.mean(d) for d in G_data]
    S_data = grp['density_S'].reshape(num_steps+1, grid_size[0], grid_size[1])
    #S_mean = [np.mean(d) for d in S_data]
    T_data = grp['density_T'].reshape(num_steps+1, grid_size[0], grid_size[1])
    #T_mean = [np.mean(d) for d in T_data]
    x_data = grp['position_x'].reshape(num_steps+1, grid_size[0], grid_size[1])
    y_data = grp['position_y'].reshape(num_steps+1, grid_size[0], grid_size[1])

    # Assemble the arguments
    #args = [G_data, S_data, T_data]
    #if fmt:
    #    args.append(fmt)
    # Call the plot function
    #plt.plot(*args, **plot_kwargs)
    time = 0
    fig, ax = plt.subplots()
    for i in range(grid_size[0]):
        for j in range(grid_size[1]):
            if (x_data[0,i,j]/grid_size[0]+y_data[0,i,j]/grid_size[1]<=1):
                l = ((G_data[1,i,j]-G_data[0,i,j])**2 + (T_data[1,i,j]-T_data[0,i,j])**2)**0.5
                plt.quiver(x_data[0,i,j]/grid_size[0], y_data[0,i,j]/grid_size[1], \
                (G_data[1,i,j]-G_data[0,i,j])/l,(T_data[1,i,j]-T_data[0,i,j])/l,\
                width=0.001, headwidth=5)
                    #,c=(G_data[0,i,j],S_data[0,i,j],T_data[0,i,j]))
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
        x_data = grp['position_x'].reshape(grid_size[0], grid_size[1], num_steps+1)
        y_data = grp['position_y'].reshape(grid_size[0], grid_size[1], num_steps+1)

        plt.scatter(beta,G,c='green',s=6)
        plt.scatter(beta,T,c='black',s=4)

    # Call the plot function
    #plt.plot(*args, **plot_kwargs)

    # Save and close figure
    save_and_close(out_path, save_kwargs=save_kwargs)
