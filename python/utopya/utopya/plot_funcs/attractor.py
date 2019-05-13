"""This module provides plotting functions to visualize the attractive set of
a dynamical system.
"""

import copy
import logging
from typing import Sequence, Union, Dict, Callable, Tuple
import itertools

import numpy as np
import xarray as xr
from scipy.signal import find_peaks

import matplotlib as mpl
from matplotlib.patches import Circle, Rectangle
from matplotlib.collections import PatchCollection

import utopya
import utopya.dataprocessing as utdp
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator
from utopya.tools import recursive_update
from utopya.plot_funcs._utils import calc_pxmap_rectangles
from utopya.plot_funcs._mpl_helpers import HandlerEllipse


# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=MultiversePlotCreator)
def bifurcation_diagram(dm: DataManager, *, hlpr: PlotHelper,
                        mv_data: xr.Dataset, dim: str=None, 
                        dims: Tuple[str, str]=None,                        
                        analysis_steps: Sequence[Union[str, Tuple[str, str]]],
                        custom_analysis_funcs: Dict[str, Callable]=None,
                        analysis_kwargs: dict=None,
                        visualization_kwargs: dict=None,
                        to_plot: dict=None,
                        **kwargs) -> None:
    """Plots a bifurcation diagram for one or two parameter dimensions
       (arguments ``dim`` or ``dims``). 

    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        mv_data (xr.Dataset): The extracted multidimensional dataset
        dim (str, optional): The required parameter dimension of the 1d
            bifurcation diagram. 
        dims (str, optional): The required parameter dimensions (x, y) of the 
            2d-bifurcation diagram.                  
        analysis_steps (Sequence): The analysis steps that are to be made
            until one is conclusive. Applied per universe.

            - If seq of str: The str will also be used as attractor key for 
                plotting if the test is conclusive.
            - If seq of Tuple(str, str): The first str defines the attractor
                key for plotting, the second str is a key within 
                custom_analysis_funcs.

            Default analysis_funcs are:

            - endpoint: utopya.dataprocessing.find_endpoint
            - fixpoint: utopya.dataprocessing.find_fixpoint
            - multistability: utdp.find_multistability
            - oscillation: utdp.find_oscillation
            - scatter: resolve_scatter
        custom_analysis_funcs (dict): A collection of custom analysis functions
            that will overwrite the default analysis funcs (recursive update).
        analysis_kwargs (dict, optional): The entries need to match the 
            analysis_steps. The subentry (dict) is passed on to the analysis 
            function.
        visualization_kwargs (dict, optional): The entries need to match the 
            analysis_steps. The subentry (dict) is used to configure a
            rectangle to visualize the conclusive analysis step. Is passed to
            matplotlib.patches.rectangle. xy, width, height, and angle are
            ignored and set automatically. Required in 2d bifurcation diagram.
        to_plot (dict, optional): The configuration for the data to plot. The
            entries of this key need to match the data_vars selected in mv_data.
            It is used to visualize the state of the attractor additionally to 
            the visualization kwargs. Only for 1d-bifurcation diagram.
            sub_keys: 

            - ``label`` (str, optional): label in plot
            - ``plot_kwargs`` (dict, optional): passed to scatter for every
                universe

                - color (str, recommended): unique color for every 
                    data_variable accross universes  
        **kwargs: Collection of optional dicts passed to different functions

            - plot_coords_kwargs (dict): Passed to ax.scatter to mark the
                universe's center in the bifurcation diagram
            - rectangle_map_kwargs (dict): Passed to 
                utopya.plot_funcs._utils.calc_pxmap_rectangles
            - legend_kwargs (dict): Passed to ax.legend
    """
    def resolve_analysis_steps(analysis_steps: Sequence[Union[str,
                                                              Tuple[str, str]]]
                               ) -> Sequence[Tuple[str, str]]:
        """Resolve instance of str to Tuple[str, str] in sequence
        
        Args:
            analysis_steps (Sequence[Union[str, Tuple[str, str]]]): The 
                original sequence
        
        Returns:
            analysis_steps (Sequence[Tuple[str, str]]): The sequence of
                attractor_key and analysis_func pairs.
        """
        for i, analysis_step in enumerate(analysis_steps):
            # get key and func for the analysis step
            if isinstance(analysis_step, str):
                analysis_steps[i] = [analysis_step, analysis_step]

        return analysis_steps
    
    def resolve_to_plot_kwargs(to_plot: dict) -> dict:
        """Resolves the to_plot dict, e.g. adding labels if not explicitly
        specified.
        
        Args:
            to_plot (dict): The to_plot dict to parse

        Returns:
            dara_vars_plot_kwargs (dict): A dict with the 'plot_kwargs' for 
                every data_var in to_plot.
        
        """
        if not to_plot:
            return {}
        
        data_vars_plot_kwargs = {}

        for k, v in to_plot.items():
            plot_kwargs = v.get("plot_kwargs", {})
            if not plot_kwargs.get('label'):
                plot_kwargs['label'] = v.get('label', k)
            data_vars_plot_kwargs[k] = plot_kwargs

        return data_vars_plot_kwargs
    
    def create_legend_handles(*, visualization_kwargs: dict, 
                              data_vars_plot_kwargs: dict):
        """Creates legend handles
        
        Processes entries in data_vars_plot_kwargs (from to_plot) and
        visualization_kwargs.
        
        Args:
            visualization_kwargs (dict): The visualization kwargs
            data_vars_plot_kwargs (dict): The resolved entries in to_plot
        
        Returns:
            Tuple[list, list]: Tuple of legend handles and legend labels lists
                as required by ax.legend(handles, labels)
            data_vars_plot_kwargs: The updated entries data_vars_plot_kwargs
                as required by plot_attractor.
        """        
        # Some defaults
        circle_kwargs = dict(xy=(.5, .5), radius=.25, edgecolor="none")
        rect_kwargs = dict(xy=(0., 0.), height=.75, width=1., edgecolor="none")

        # Lists to be populated for matplotlib legend
        legend_handles = []
        legend_labels = []

        for k, kwargs in data_vars_plot_kwargs.items():
            label = kwargs.pop('label', kwargs)
            kwargs['linewidth'] = kwargs.get('linewidth', 0.)
            data_vars_plot_kwargs[k] = kwargs

            # Determine color
            if 'color' in kwargs:
                color = kwargs['color']

            elif 'cmap' in kwargs:
                cmap = mpl.cm.get_cmap(kwargs['cmap'])
                color = cmap(1.)

            else:
                log.warning("No color defined for data_var '{}'!".format(k))
                color = None

            # Create and add the handle and the label
            legend_handles.append(Circle(**circle_kwargs, facecolor=color))
            legend_labels.append(label)
        
        for k, kwargs in visualization_kwargs.items():
            if 'to_plot' in kwargs:
                for dvar_name, dvar_kwargs in kwargs['to_plot'].items():
                    # Make sure a linewidth is set
                    dvar_kwargs['linewidth'] = dvar_kwargs.get('linewidth', 0.)
                    kwargs['to_plot'][dvar_name] = dvar_kwargs

                    # Create and add the handle and the label
                    legend_handles.append(Rectangle(**rect_kwargs,
                                                    **dvar_kwargs))
                    legend_labels.append(dvar_kwargs.get('label', dvar_name))
            else:
                kwargs['linewidth'] = kwargs.get('linewidth', 0.)
                data_vars_plot_kwargs[k] = kwargs

                legend_handles.append(Rectangle(**rect_kwargs, **kwargs))
                legend_labels.append(kwargs.get('label', k))
        
        return [legend_handles, legend_labels], data_vars_plot_kwargs

    def apply_analysis_steps(data: xr.Dataset, 
                             analysis_steps: Sequence[Union[str,
                                                            Tuple[str, str]]], 
                             *, analysis_funcs: dict, analysis_kwargs: dict):
        """Perform the sequence of analysis steps until the first conclusive.
        
        Args:
            data (xr.Dataset): The data to analyse.
            analysis_steps (Sequence[Union[str, Tuple[str, str]]]): The analysis steps that are to be made
                until one is conclusive. Applied per universe.
            analysis_funcs (dict): The entries need to match the 
                analysis_steps. Map of the analysis_steps to their Callables
            analysis_kwargs (dict): The entries need to match the 
                analysis_steps. The subentry (dict) is passed on to the
                corresponding analysis function.
        
        Returns:
            analysis_key (str): The key of the conclusive analysis step.
            attractor (xr.Dataset): The data corresponding to this analysis.
        """
        for analysis_key, analysis_func in analysis_steps:
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

            # Return if conclusive
            if conclusive:
                return analysis_key, attractor

        # Return non-conclusive
        return None, None        

    def resolve_rectangle(coord: dict, rectangles: xr.Dataset
                          ) -> Rectangle:
        """Resolve the rectangle patch at this coordinate
        
        Args:
            coord (dict): The bifurcation parameter's coordinate
            rectangles (xr.Dataset): The rectangles that cover the 2D space
                spanned by the coordiantes. The `coord` should be one entry of
                rectangles.coords.
        
        Raises:
            ValueError: Coordinate not available in rectangles.
        
        Returns:
            Rectangle: A rectangle around a universe with coord and
                shape defined by rectangles.
        """
        try:
            rectangle = rectangles.sel(coord)

        except Exception as exc:
            raise ValueError("The requested paramspace coordinate(s) {} are "
                             "not coordinates of rectangles {}. Plot failed."
                             "".format(coord, rectangles.coords)) from exc

        rect_spec = rectangle['rect_spec']
        return Rectangle(*rect_spec.item())

    def append_vis_patch(attrator_key: str, attractor: xr.Dataset,
                         vis_patches: dict, vis_kwargs: dict,
                         **resolve_rectangle_args):
        """Append visualization patch

        Performs postprocess for

            - attractor key 'fixpoint' and 'endpoint' if: 'to_plot' in entry of
                vis_kwargs. Then finds the data_var with highest valued
                datapoint.
        
        Args:
            attractor_key (str): Key according to which to decode the attractor
            attractor (xr.Dataset): The Dataset with the encoded attractor 
                information. See possible encodings
            vis_patches (dict): The map of attractor_key to
                List[Rectangle] where to append the new patch
            vis_kwargs (dict): The visualization kwargs
            **resolve_rectangle_args: Passed on to resolve_rectangle
        
        Raises:
            ValueError: Bad postprocess key
        
        Returns:
            vis_patches (dict): The new map of attractor_key to
                List[Rectangle]
        
        Deleted Parameters:
            resolve_rectangle_args (dict): Args as required by
                resolve_rectangle
        """
        # Depending on the kind of attractor, add different patches
        kwargs = vis_kwargs.get(attractor_key)
        if kwargs is None:
            return vis_patches
        
        # Postprocess fixpoint and to_plot, append rectangle
        if (    (attractor_key == 'fixpoint' or attractor_key == 'endpoint')
            and 'to_plot' in kwargs):
            max_value = -np.inf
            for data_var_name, data_var in attractor.data_vars.items():
                if data_var.max() > max_value:
                    max_value = data_var.max()
                    max_name = data_var_name
        
            rect = resolve_rectangle(**resolve_rectangle_args)

            attractor_var_key = attractor_key + '_' + max_name
            vis_patches[attractor_var_key].append(rect)

        # Append rectangle
        elif vis_patches.get(attractor_key) is not None:
            rect = resolve_rectangle(coord=coord, rectangles=rects)
            vis_patches[attractor_key].append(rect)

        return vis_patches

    def append_plot_attractor(attractor_key: str, attractor: xr.Dataset, *,
                              coord: float=None, scatter_kwargs: list,
                              **plot_kwargs):
        """Resolves how to plot attractor of specified type 
        at specific bifurcation parameter value.

        Args:
            attractor_key (str): Key according to which to decode the attractor
            attractor (xr.Dataset): The Dataset with the encoded attractor 
                information. See possible encodings
            coord (float, optional): The bifurcation parameter's coordinate,
                if None its derived from the attractors coordinates
            scatter_kwargs (list): The list of scatter datasets where to append
                the new scatter.
            plot_kwargs (dict, optional): The kwargs used to specify ax.scatter
                where the entries match the attractor.data_vars 

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

        Returns:
            scatter_kwargs: The new list of scatter datasets
        """        
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

        # Resolve the scatter kwargs depending on attractor key
        if attractor_key in ('fixpoint', 'endpoint', 'multistability'):
            for data_var_name, data_var in attractor.data_vars.items():
                data_var = data_var.where(data_var != np.nan, drop=True)
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
        
        elif attractor_key:
            raise KeyError("Invalid attractor-key '{}'! "
                           "Available keys: 'endpoint', fixpoint',"
                           " 'multistability', 'scatter', 'oscillation'."
                           "".format(attractor_key))
        
        return scatter_kwargs
  
    def resolve_scatter(data: xr.Dataset, *, spin_up_time: int=0,
                        **kwargs) ->tuple:
        """A mock analysis function to plot all times larger than a spin
        up time.
        """
        return True, data.where(data.time >= spin_up_time, drop=True)

    # .........................................................................

    # Check argument values
    if not dim and not dims:
        raise KeyError("No dim (str) or dims (Tuple[str, str]) specified. "
                       "Use dim for a 1d-bifurcation diagram and dims for a "
                       "2d-bifurcation diagram.")

    if dim and dims:
        raise KeyError("dim='{}' and dims='{}' specified. "
                       "Use either dim for a 1d-bifurcation diagram or dims "
                       "for a 2d-bifurcation diagram."
                       "".format(dim, dims))

    if dims is not None and len(dims) != 2:
        raise ValueError("Argument dims should be of length 2, but was: {}"
                         "".format(dims))

    # TODO In the future, consider not using `dim` below here but handling it
    #      via the length of `dims`.
    
    # Default values
    if visualization_kwargs is None:
        visualization_kwargs = {}
    
    if analysis_kwargs is None:
        analysis_kwargs = {}

    # Resolve legend handles and visualization kwargs
    data_vars_plot_kwargs = resolve_to_plot_kwargs(to_plot)
    legend_handles, data_vars_plot_kwargs = create_legend_handles(
        visualization_kwargs=visualization_kwargs,
        data_vars_plot_kwargs=data_vars_plot_kwargs
    )
    
    # Define default analysis functions
    analysis_funcs = dict(endpoint=utdp.find_endpoint,
                          fixpoint=utdp.find_fixpoint,
                          multistability=utdp.find_multistability,
                          oscillation=utdp.find_oscillation,
                          scatter=resolve_scatter)

    # If given, update
    if custom_analysis_funcs:
        log.debug("Updating with custom analysis functions ...")
        analysis_funcs = recursive_update(analysis_funcs,
                                          custom_analysis_funcs)

    analysis_steps = resolve_analysis_steps(analysis_steps)
    
    # Obtain the rectangles covering space spanned by the coordinates
    rectangle_map_kwargs = kwargs.get('rectangle_map_kwargs', {})
    if dim:
        rects, limits = calc_pxmap_rectangles(x_coords=mv_data[dim].values,
                                              y_coords=None,
                                              **rectangle_map_kwargs)
    elif dims:
        rects, limits = calc_pxmap_rectangles(x_coords=mv_data[dims[0]].values,
                                              y_coords=mv_data[dims[1]].values,
                                              **rectangle_map_kwargs)

    # Obtain the list of param_coords to iterate
    if dim:
        param_iter = mv_data[dim].values
    
    elif dims:
        param_iter = itertools.product(mv_data[dims[0]].values,
                                       mv_data[dims[1]].values)

    # Map of analysis_key to list[mpatch.Rectangle]
    vis_patches = {}
    for analysis_key, _ in analysis_steps:
        if not visualization_kwargs.get(analysis_key):
            continue

        if 'to_plot' in visualization_kwargs[analysis_key]:
            for var_key, _ in visualization_kwargs[analysis_key]['to_plot'].items():
                analysis_var_key = analysis_key + '_' + var_key
                vis_patches[analysis_var_key] = []

        else:
            vis_patches[analysis_key] = []

    # The List[dict] passed to ax.scatter
    scatter_kwargs = []
    scatter_coords_kwargs = []

    # Iterate the parameter coordinates
    for param_coord in param_iter:
        # Resolve the param_coord to dict
        if dim:
            param_coord = {dim: param_coord}
        
        elif dims:
            param_coord = {dims[0]: param_coord[0], dims[1]: param_coord[1]}
        
        # Plot coord
        if dim and kwargs.get('plot_coords_kwargs'):
            plot_coords_kwargs = kwargs.get('plot_coords_kwargs')
            scatter_coords_kwargs.append({'x': param_coord[dim],
                                          'y': plot_coords_kwargs.pop('y', 0.),
                                          **plot_coords_kwargs})

        if dims and kwargs.get('plot_coords_kwargs'):
            scatter_coords_kwargs.append({'x': param_coord[dims[0]],
                                          'y': param_coord[dims[1]],
                                          **kwargs.get('plot_coords_kwargs')})
        
        # Select the data and analyse
        data = mv_data.sel(param_coord)
        attractor_key, attractor = apply_analysis_steps(
                                        data, analysis_steps,
                                        analysis_funcs=analysis_funcs,
                                        analysis_kwargs=analysis_kwargs)
        
        # If conclusive, append a rectangular patch to the attractor_key's
        # patch collection
        if attractor_key:
            # Determine coordinate value
            if dim:
                rect_map_kwargs = kwargs.get('rectangle_map_kwargs', {})
                y = rect_map_kwargs.get('default_pos', (0., 0.))[1]
                coord = dict(x=param_coord[dim], y=y)
            
            elif dims:
                coord = dict(x=param_coord[dims[0]], y=param_coord[dims[1]])

            vis_patches = append_vis_patch(attractor_key, attractor,
                                           vis_patches, visualization_kwargs, 
                                           coord=coord, rectangles=rects)

        # For 1d case ...
        if dim and to_plot:
            scatter_kwargs = append_plot_attractor(attractor_key, attractor,
                                                   coord=param_coord[dim], 
                                                   scatter_kwargs=scatter_kwargs,
                                                   **data_vars_plot_kwargs)

    # Draw collection of visualization patches
    for analysis_key, _ in analysis_steps:
        if not visualization_kwargs.get(analysis_key):
            continue

        if analysis_key in visualization_kwargs:
            vis_kwargs = visualization_kwargs[analysis_key]
            
            if 'to_plot' in vis_kwargs:
                for var_key, var_kwargs in vis_kwargs['to_plot'].items():
                    attractor_var_key = analysis_key + '_' + var_key

                    pc = PatchCollection(vis_patches[attractor_var_key],
                                         **var_kwargs)
                    hlpr.ax.add_collection(pc)

            else:
                pc = PatchCollection(vis_patches[analysis_key], **vis_kwargs)
                hlpr.ax.add_collection(pc)

        # else: nothing to do
    
    # Scatter the universe's coordinates
    for kws in scatter_coords_kwargs:
        hlpr.ax.scatter(**kws)

    # Scatter the attractor
    for kws in scatter_kwargs:
        hlpr.ax.scatter(**kws)

    # Provide PlotHelper defaults
    hlpr.provide_defaults('set_limits', **limits)

    if dim:
        hlpr.provide_defaults('set_labels', x=dim, y='state')
    elif dims:
        hlpr.provide_defaults('set_labels', x=dims[0], y=dims[1])

    if legend_handles:
        hlpr.ax.legend(legend_handles[0], legend_handles[1], 
                       handler_map={Circle: HandlerEllipse()}, 
                       **kwargs.get('legend_kwargs', {}))
