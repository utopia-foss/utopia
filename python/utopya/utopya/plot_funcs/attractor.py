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
# logging.getLogger('matplotlib.animation').setLevel(logging.WARNING)

def bifurcation_codimension_one(dm: DataManager, *,
                                out_path: str,
                                uni: UniverseGroup,
                                model_name: str,
                                to_plot: dict,
                                param_dim: str,
                                fmt: str=None,
                                plot_kwargs: dict=None,
                                save_kwargs: dict=None):
    """Plots a bifurcation diagram for one parameter dimension (param_dim)
        i.e. plots the final state over the parameter
    
    Arguments:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        uni (int): The universe to use
        model_name (str): The name of the model instance, in which the data is
            located.
        to_plot (dict): The plotting configuration. The entries of this key
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels.
        param_dim (str): The parameter dimension of the bifurcation diagram
        fmt (str, optional): the plt.plot format argument
        plot_kwargs (dict, optional): Passed on to plt.plot
        save_kwargs (dict, optional): kwargs to the plt.savefig function

    Raises:
        ValueError: for an invalid `param_dim` value
        ValueError: for an invalid `to_plot/*/time_fraction` value
        Warning: no color defined for `to_plot` entry (coloring misleading)
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
            # coloring misleading
            log.warning("Warning: No color defined for to_plot "+ prop_name)

	# plot for parameter dimension sweep
    for uni in dm['multiverse'].values():
        # Get the group that all datasets are in
        grp = uni['data'][model_name]

        # Get the shape of the data
        cfg = uni['cfg']
        steps = int(cfg['num_steps']/cfg.get('write_every', 1))
        # NOTE The steps variable does not correspond to the actual _time_ of the
        #      simulation at those frames!

        # get value of parameter
        try:
            param_value = cfg[model_name][param_dim]
        except:
            raise ValueError("Argument param_dim not available in Model."
                            " Was: {} with value: '{}'"
                            "".format(type(param_dim), param_dim))
        
        # Extract the densities
        data = {p: grp[p] for p in to_plot.keys()}
        
		# scatter data for all properties to plot
        for (prop_name, props) in to_plot.items():
            # if time_fraction defined
            # consider end of data with length time_fraction*steps
            if 'time_fraction' in props.keys():
                time = int(props['time_fraction'] * steps)
                if time < 1 or time > steps:
                    raise ValueError("Value of argument"
                            " `to_plot/{}/time_fraction` not valid."
                            " Was: {} with value: '{}'"
                            " for a simulation with {} steps."
                            " Min: {} (or None), Max: 1.0"
                            "".format(prop_name, type(props['time_fraction']),
                                      props['time_fraction'], steps, 1./steps))
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


    ax.set_xlabel(param_dim)
    ax.set_ylabel("final State")
    ax.set(**plot_kwargs)

    # Save and close figure
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()