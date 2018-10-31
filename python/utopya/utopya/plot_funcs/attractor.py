"""This module provides plotting functions to 
   visualize the attractive set of a dynamical system."""

from ._setup import *

import logging

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

from utopya import DataManager


# Get a logger
log = logging.getLogger(__name__)

# Increase log threshold for animation plotting
logging.getLogger('matplotlib.animation').setLevel(logging.WARNING)

def bifurcation_codimension_one(dm: DataManager, *,
                                out_path: str,
                                uni: UniverseGroup,
                                model_name: str,
                                to_plot: dict,
                                param_key: str,
                                fmt: str=None,
                                save_kwargs: dict=None,
                                **plot_kwargs):
    """Plots a bifurcation diagram for one parameter dimension (param_key)
        i.e. plots the final state over the parameter
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        model_name (str): The name of the model instance, in which the data is
            located.
        to_plot (dict): The plotting configuration. The entries of this key
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels.
        param_key (str): The parameter dimension of the bifurcation diagram
        fmt (str, optional): the plt.plot format argument
        save_kwargs (dict, optional): kwargs to the plt.savefig function
        **plot_kwargs: Passed on to plt.plot
    """
    
    fig = plt.figure()
    ax = fig.add_subplot(111)

    # get plot configuration
    handles = []
    plot_min_max_only = dict()
    for (prop_name, props) in to_plot.items():
        # if plot_min_max_only
        if 'plot_min_max_only' in props.keys():
            plot_min_max_only[prop_name] = props['plot_min_max_only']
        else:
            plot_min_max_only[prop_name] = False

        # get label
        if 'label' in props.keys(): 
            label = props['label']
        else:
            label = prop_name

        # add legend
        if 'plot_kwargs' in props.keys() and 'color' in props['plot_kwargs']:
            handles.append(mpatches.Patch(label=label, 
                           color=props['plot_kwargs']['color']))
        else:
            print("Warning: no color defined for to_plot ", prop_name, "!")

	# plot for parameter dimension sweep
    for uni in dm['multiverse'].values():
        # Get the group that all datasets are in
        grp = uni['data'][model_name]

        # Get the shape of the data
        cfg = uni['cfg']
        grid_size = cfg[model_name]['grid_size']
        steps = int(cfg['num_steps']/cfg.get('write_every', 1))
        # NOTE The steps variable does not correspond to the actual _time_ of the
        #      simulation at those frames!

        # get value of parameter
        try:
            param_value = cfg[model_name][param_key]
        except:
            raise TypeError("Argument param_value not available in Model."
                            " Was: {} with value: '{}'"
                            "".format(type(param_value), param_value))
        
        # Extract the densities
        data = {p: grp[p] for p in to_plot.keys()}
        
		# scatter data for all properties to plot
        for (prop_name, props) in to_plot.items():
            # if time_fraction defined
            # consider end of data with length time_fraction*steps
            if 'time_fraction' in props.keys():
                time = max(int(props['time_fraction'] * steps), 1)
            # else only last data point
            else:
                time = 1

            # plot minimum and maximum
            if plot_min_max_only[prop_name]:
                plt.scatter(param_value, np.amin(data[prop_name][-time:]), 
							**props['plot_kwargs'])
                plt.scatter(param_value, np.amax(data[prop_name][-time:]), 
							**props['plot_kwargs'])
            # plot final state(s)
            else:
                plt.scatter([param_value]*time, data[prop_name][-time:], 
							**props['plot_kwargs'])

    if len(handles) > 0:
        ax.legend(handles=handles)

    ax.set_xlabel(param_key)
    ax.set_ylabel("final State")

    # Save and close figure
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()