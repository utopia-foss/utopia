"""This module provides plotting functions to visualize the attractive set of
a dynamical system.
"""

import copy
import logging
from typing import Sequence, Union, Dict, Callable, Tuple

import numpy as np
import xarray as xr
from scipy.signal import find_peaks

import matplotlib as mpl
import matplotlib.patches as mpatches

import utopya
import utopya.dataprocessing as utdp
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator
from utopya.tools import recursive_update


# Get a logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------
@is_plot_func(creator_type=MultiversePlotCreator,
              helper_defaults=dict(
                set_labels=dict(x="bifurcation parameter", y='state')
                )
              )
def bifurcation_diagram(dm: DataManager, *,
                        hlpr: PlotHelper, 
                        mv_data,
                        dim: str,
                        to_plot: dict,
                        analysis_steps: Sequence[Union[str, Tuple[str, str]]],
                        custom_analysis_funcs: Dict[str, Callable]=None,
                        analysis_kwargs: dict=None) -> None:
    """Plots a bifurcation diagram for one parameter-dimension (dim). 

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        mv_data (xr.Dataset): The extracted multidimensional dataset
        dim (str): The parameter dimension of the bifurcation diagram
        to_plot (dict): The configuration for the data to plot. The entries
            of this key need to match the data_vars selected in mv_data.
            sub_keys: 

            - ``label`` (str, optional): label in plot
            - ``plot_kwargs`` (dict, optional): passed to scatter for every
                universe

                - color (str, recommended): unique color for every 
                    data_variable accross universes
                    
        analysis_steps (Sequence): The analysis steps that are to be made
            until one is conclusive.

            - If seq of str: The str will also be used as attractor key for 
                plotting if the test is conclusive.
            - If seq of Tuple(str, str): The first str defines the attractor
                key for plotting, the second str is a key within 
                custom_analysis_funcs.

        custom_analysis_funcs (dict): A collection of custom analysis functions
            that will overwrite the default analysis funcs (recursive update).
    """
    def plot_attractor(attractor: xr.Dataset, attractor_key: str, *,
                       coord: float=None, **plot_kwargs):
        """Resolves how to plot attractor of specified type 
        at specific bifurcation parameter value.

        Args:
            attractor (xr.Dataset): The Dataset with the encoded attractor 
                information. See possible encodings
            attractor_key (str): Key according to which to decode the attractor
            coord (float, optional): The bifurcation parameter's coordinate,
                if None its derived from the attractors coordinates

        Possible encodings, i.e. values for ``attractor_key``:
            ``fixpoint``: xr.Dataset with dimensions ()
            ``scatter``: xr.Dataset with dimensions (time: >=1)
            ``multistability``: xr.Dataset with dimensions
                (<initial_state>: >= 1)
            ``oscillation``: xr.Dataset with dimensions (osc: 2), the minimum
                and maximum
        
        NOTE the attractor must contain the bifurcation parameter coordinate
        
        Raises:
            KeyError: Unknown attractor_key
            KeyError: No bifurcation coordinate received
            ValueError: Attractor encoding mismatched with the given 
                attractor_key
        """
        def check_compatibility(*, plot_kwargs):
            if attractor_key == 'scatter':
                if 'time' not in attractor.dims:
                    raise ValueError("Attractor does not match the encoding "
                                     "of type 'scatter'. Must be {'time': "
                                     ">= 1} and has no dimension 'time'. "
                                     "Was {}.".format(attractor.dims))
                
                if len(attractor.dims) > 1:
                    raise ValueError("Attractor does not match the encoding "
                                     "of type 'scatter'. Must be {'time': >= "
                                     "1}, but has further dimension(s). "
                                     "Was {}.".format(attractor.dims))
                
                # Remove duplicate entries
                for data_var_name in attractor.data_vars:
                    data_var_plot_kwargs = plot_kwargs.get(data_var_name, {})

                    # Resolve colour map
                    if 'cmap' in data_var_plot_kwargs:
                        data_var_plot_kwargs.pop('c', None)
                        data_var_plot_kwargs.pop('color', None)
                        plot_kwargs[data_var_name] = data_var_plot_kwargs
                return
        
        # Get the bifurcation parameter coordinate
        if not coord:
            try:
                coord = attractor[dim]

            except KeyError as err:
                raise KeyError("No bifurcation parameter coordinate '{}' "
                               "could be found! Either have it as a "
                               "coordinate in 'attractor' or pass it to "
                               "'plot_attractor' explicitly.".format(dim)
                               ) from err

        # Check the encoding of the attractor
        check_compatibility(plot_kwargs=plot_kwargs)

        # A list of dicts to fill with arguments to the plot function
        scatter_kwargs = []

        # Resolve the scatter kwargs depending on attractor key
        if attractor_key in ('fixpoint', 'endpoint', 'multistability'):
            for data_var_name, data_var in attractor.data_vars.items():
                entries = 1
                if data_var.shape:
                    entries = len(data_var)

                scatter_kwargs.append(dict(x=[coord]*entries,
                                      y=data_var,
                                      **plot_kwargs.get(data_var_name, {})))
        
        elif attractor_key == 'scatter':
            for data_var_name, data_var in attractor.data_vars.items():
                if 'cmap' in plot_kwargs.get(data_var_name, {}):
                    scatter_kwargs.append(
                        dict(x=[coord]*len(data_var.data),
                             y=data_var,
                             c=attractor['time'],
                             **plot_kwargs.get(data_var_name, {}))
                    )
                
                else:
                    scatter_kwargs.append(
                        dict(x=[coord]*len(data_var.data),
                             y=data_var,
                             **plot_kwargs.get(data_var_name, {}))
                    )
        
        elif attractor_key == 'oscillation':
            for data_var_name, data_var in attractor.data_vars.items():
                scatter_kwargs.append(dict(x=[coord]*len(data_var.data),
                                        y=data_var,
                                        **plot_kwargs.get(data_var_name, {})))
        else:
            raise KeyError("No attractor-key '{}' known. "
                           "Available keys: ['endpoint', fixpoint',"
                           " 'multistability', 'scatter', 'oscillation']."
                           "".format(attractor_key))

        # Now, call the plot function
        for kws in scatter_kwargs:
            hlpr.ax.scatter(**kws)

    def resolve_plot_kwargs(to_plot: dict) -> tuple:
        """Resolves the to_plot dict, e.g. to create legend handles

        Returns:
            dara_vars_plot_kwargs (dict): A dict with the 'plot_kwargs' for 
                every data_var.
            legend_handles (list): List of mpatches.Patch, the handles for the 
                creation of a legend.
        """
        data_vars_plot_kwargs = {}
        legend_handles = []

        for k, v in to_plot.items():
            data_vars_plot_kwargs[k] = v.pop("plot_kwargs", {})
            
            # Get label and add to legend handles
            label = v.pop('label', k)
            if 'color' in data_vars_plot_kwargs[k]:
                legend_handles.append(mpatches.Patch(label=label, 
                            color=data_vars_plot_kwargs[k]['color']))

            elif 'cmap' in data_vars_plot_kwargs[k]:
                cmap = mpl.cm.get_cmap(data_vars_plot_kwargs[k]['cmap'])
                c = cmap(1.)
                legend_handles.append(mpatches.Patch(label=label, color=c))

            else:
                log.warning("No color defined for data_var '{}'!".format(k))

        return data_vars_plot_kwargs, legend_handles
    
    def resolve_scatter(data: xr.Dataset, *, spin_up_time: int=0,
                        **kwargs) ->tuple:
        """A mock analysis function to plot all times larger than a spin
        up time.
        """
        return True, data.where(data.time >= spin_up_time, drop=True)

    # .. Finished with helper function definitions ............................

    plot_kwargs, legend_handles = resolve_plot_kwargs(copy.deepcopy(to_plot))

    # Define default analysis functions
    analysis_funcs = dict(endpoint=utdp.find_endpoint,
                          fixpoint=utdp.find_fixpoint,
                          oscillation=utdp.find_oscillation,
                          scatter=resolve_scatter)

    # If given, update
    if custom_analysis_funcs:
        log.debug("Updating with custom analysis functions ...")
        analysis_funcs = recursive_update(analysis_funcs,
                                          custom_analysis_funcs)
    
    # .. Plotting .............................................................
    # Go over the selected parameter dimension data
    for param_coord in mv_data[dim].values:
        data = mv_data.sel({dim: param_coord})

        for analysis_step in analysis_steps:
            # get key and func for the analysis step
            if isinstance(analysis_step, str):
                analysis_key = analysis_step
                analysis_func = analysis_step
                analysis_func_kwargs = analysis_kwargs.get(analysis_key, {})

            else:
                # Expecting 2-tuple or list; unpack
                analysis_key, analysis_func = analysis_step
                analysis_func_kwargs = analysis_kwargs.get(analysis_func, {})
            
            # resolve the analysis function from its name
            if isinstance(analysis_func, str):
                if analysis_func in analysis_funcs:
                    analysis_func = analysis_funcs[analysis_func]

                else:
                    # Try to get it from dataprocessing ... might fail.
                    analysis_func = getattr(utopya.dataprocessing,
                                            analysis_func)
            
            # Perfom the analysis step
            conclusive, attractor = analysis_func(data, **analysis_func_kwargs)

            # Plot it, if conclusive
            if conclusive:
                plot_attractor(attractor, analysis_key, **plot_kwargs)
                log.debug("Conclusive analysis step '{}' for coord '{}'."
                          " Plotting result and stopping here."
                          "".format(analysis_step, param_coord))

                # Done here. :)
                break

        if not conclusive:
            # Inconclusive. Only reached if break was _not_ called...
            log.warning("No conclusive analysis for universe coordinate {} "
                        "({}). Nothing to plot! Performed the following "
                        "analysis steps: {}."
                        "".format(param_coord, mv_data.coords,
                                  ", ".join(analysis_steps)))
        
    # Done plotting.
    # Provide some (dynamic) default values for helpers
    hlpr.provide_defaults('set_labels', x=dim)
    if legend_handles:
        hlpr.ax.legend(handles=legend_handles)
