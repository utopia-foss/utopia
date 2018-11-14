"""This module provides plotting functions to 
   visualize the attractive set of a dynamical system."""

from ._setup import *

import logging

import numpy as np
from scipy.signal import find_peaks

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
                                mv_data,
                                model_name: str,
                                to_plot: dict,
                                param_dim: str,
                                fmt: str=None,
                                spin_up_fraction: float=0,
                                find_peaks_kwargs: dict=None,
                                plot_kwargs: dict=None,
                                save_kwargs: dict=None):
    """Plots a bifurcation diagram for one parameter dimension (param_dim)
        i.e. plots the final state over the parameter
    
    Arguments:
        dm (DataManager): The data manager from which to retrieve the data
        out_path (str): Where to store the plot to
        mv_data (xr.Dataset): The extracted multidimensional dataset
        model_name (str): The name of the model instance, in which the data is
            located.
        to_plot (dict): The plotting configuration. The entries of this key
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels.
            sub_keys: 
                label (str, optional): label in plot
                time_fraction (str, optional): fraction of simulation scattered
                plot_kwargs (dict, optional): passed to scatter for every universe
                    color (str, recommended): unique color for multiverses
        param_dim (str): The parameter dimension of the bifurcation diagram
        fmt (str, optional): the plt.plot format argument
        spin_up_fraction(float, optional): fraction of the simulation for 
            equilibration of system
        find_peaks_kwargs (dict, optional): if given requires `height` key
            passed on to find_peaks
            then only the return points are plotted (for stable oscillations)
            may be overwritten by `to_plot/*/find_peak_kwargs`
        plot_kwargs (dict, optional): Passed on to ax.set
        save_kwargs (dict, optional): kwargs to the plt.savefig function

    Raises:
        ValueError: for an invalid `param_dim` value
        ValueError: for an invalid or non-matching `prop_name` value
        ValueError: for an invalid `to_plot/*/time_fraction` value
        Warning: no color defined for `to_plot` entry (coloring misleading)
    """
    if not param_dim in mv_data.dims:
        raise ValueError("Dimension param_dim not available in multiverse data."
                         " Was: {} with value: '{}'."
                         " Available: {}"
                         "".format(type(param_dim), 
                                   param_dim,
                                   mv_data.dims))

    fig = plt.figure()
    ax = fig.add_subplot(111)

    legend_handles = []    
    ## scatter data for all properties to plot
    for (prop_name, props) in to_plot.items():
        if not prop_name in mv_data.data_vars:
            raise ValueError("Key to_plot/prop_name not available"
                             " in multiverse data."
                             " Was: {} with value: '{}'."
                             " Available in multiverse field: {}"
                             "".format(type(prop_name),
                                       prop_name,
                                       mv_data.data_vars))

        # get label
        if 'label' in props.keys(): 
            label = props['label']
        else:
            label = prop_name

        # add legend
        if 'plot_kwargs' in props.keys() and 'color' in props['plot_kwargs']:
            legend_handles.append(mpatches.Patch(label=label, 
                           color=props['plot_kwargs']['color']))
        else:
            # coloring misleading
            log.warning("Warning: No color defined for to_plot "+ prop_name)

        for data in mv_data[prop_name]:
            if 'time_fraction' in props.keys():
                # plot fraction of datapoints
                plot_time = int(props['time_fraction'] * len(data))
                if plot_time < 1 or plot_time > len(data):
                    raise ValueError("Value of argument"
                            " `to_plot/{}/time_fraction` not valid."
                            " Was: {} with value: '{}'"
                            " for a simulation with {} steps."
                            " Min: {} (or None), Max: 1.0"
                            "".format(prop_name, 
                                      type(props['time_fraction']),
                                      props['time_fraction'], 
                                      len(data), 1./len(data)))
            else:
                # plot last datapoint
                time = 1
            spin_up_time = int(spin_up_fraction * len(data))

            if 'find_peaks_kwargs' in props.keys():
                find_peaks_kwargs_prop = props['find_peaks_kwargs']
            else:
                find_peaks_kwargs_prop = find_peaks_kwargs

            # plot peaks
            if find_peaks_kwargs is not None:
                maxima, max_props = find_peaks(data[spin_up_time:],
                                        **(find_peaks_kwargs_prop
                                            if find_peaks_kwargs_prop
                                            else {}))
                amax = np.amax(data)
                minima, min_props = find_peaks(amax-data[spin_up_time:],
                                        **(find_peaks_kwargs_prop
                                            if find_peaks_kwargs_prop
                                            else {}))
                if maxima.size is 0:
                    plt.scatter(data[param_dim], data[-1],
							**props['plot_kwargs'])
                else:
                    plt.scatter([ data[param_dim] ]*len(maxima), 
                                max_props['peak_heights'], 
                                **props['plot_kwargs'])
                    plt.scatter([ data[param_dim] ]*len(minima), 
                                [ amax ]*len(minima) - min_props['peak_heights'], 
                                **props['plot_kwargs'])
            
            # plot final state(s)
            else:
                plt.scatter([data[param_dim]]*plot_time, data[-plot_time:],
							**props['plot_kwargs'])
            


    if len(legend_handles) > 0:
        ax.legend(handles=legend_handles)

    ax.set_xlabel(param_dim)
    ax.set_ylabel("final State")
    ax.set(**(plot_kwargs if plot_kwargs else {}))

    # Save and close figure
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()