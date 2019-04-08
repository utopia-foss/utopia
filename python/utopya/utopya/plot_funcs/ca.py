"""This module provides plotting functions to visualize cellular automata."""

from ._setup import *

import os
import logging
import warnings
from typing import Union, Dict, Callable

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.animation
from matplotlib.colors import ListedColormap

from utopya import DataManager
from utopya.plotting import UniversePlotCreator, PlotHelper, is_plot_func

from ._file_writer import FileWriter

# Get a logger
log = logging.getLogger(__name__)

# Increase log threshold for animation plotting
logging.getLogger('matplotlib.animation').setLevel(logging.WARNING)


# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              supports_animation=True)
def state(dm: DataManager, *,
          uni: UniverseGroup,
          hlpr: PlotHelper,
          model_name: str,
          to_plot: dict,
          time_idx: int,
          preprocess_funcs: Dict[str, Callable]=None):
    """Plots the state of the cellular automaton as a 2D heat map. 
    This plot function can be used for a single plot, but also supports
    animation.
    
    Which properties of the state to plot can be defined in `to_plot`.
    
    Args:
        dm (DataManager): The DataManager that holds all loaded data
        uni (UniverseGroup): The currently selected universe, parsed by the
            `UniversePlotCreator`.
        hlpr (PlotHelper): The plot helper
        model_name (str): The name of the model of which the data is to be
            plotted
        to_plot (dict): Which data to plot and how. The keys of this dict
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels. Each of these keys is expected to
            hold yet another dict, supporting the following configuration
            options (all optional):
                - cmap (str or dict): The colormap to use. If it is a dict, a
                    discrete colormap is assumed. The keys will be the labels
                    and the values the color. Association happens in the order
                    of entries.
                - title (str): The title for this sub-plot
                - limits (2-tuple, list): The fixed heat map limits of this
                    property; if not given, limits will be auto-scaled.
        time_idx (int): Which time index to plot the data of. Is ignored when
            creating an animation.
        preprocess_funcs (Dict[str, Callable], optional): A dictionary of pre-
            processing callables, where keys need to correspond to the
            property name in ``to_plot`` that is to be pre-processed.
            This argument can be used to implement model-specific preprocessing
            by implementing another plot function, which defines this dict and
            passes it to this function.
    
    Raises:
        NotImplementedError: ``to_plot`` of length != 1
        ValueError: Shape mismatch of data selected by ``to_plot``
    """
    # Helper functions ........................................................

    def prepare_data(prop_name: str, *,
                     all_data: dict, time_idx: int) -> np.ndarray:
        """Prepares the data for plotting"""
        # Get the data from the dict of 2d data
        data = all_data[prop_name][time_idx]

        # If preprocessing is available for this property, call that function
        if preprocess_funcs and prop_name in preprocess_funcs:
            data = preprocess_funcs[prop_name](data)

        return data

    def plot_property(prop_name: str, *, data, cmap='viridis',
                      title: str=None, limits: list=None):
        """Helper function to plot a property on a given axis and return
        an imshow object
        """
        # Get colormap, either a continuous or a discrete one
        if isinstance(cmap, str):
            norm = None
            bounds = None
            colormap = cmap

        elif isinstance(cmap, dict):
            colormap = ListedColormap(cmap.values())
            bounds = limits
            norm = mpl.colors.BoundaryNorm(bounds, colormap.N)

        else:
            raise TypeError("Argument cmap needs to be either a string with "
                            "name of the colormap or a dict with values for a "
                            "discrete colormap. Was: {} with value: '{}'"
                            "".format(type(cmap), cmap))

        # Create additional kwargs
        kws = dict()
        if limits:
            kws = dict(vmin=limits[0], vmax=limits[1], **kws)

        # Create imshow object on the currently selected axis
        im = hlpr.ax.imshow(data, cmap=colormap, animated=True, origin='lower',
                            **kws)
        
        # Create colorbars
        # TODO Should be done by helper
        cbar = hlpr.fig.colorbar(im, ax=hlpr.ax, norm=norm,
                                 ticks=bounds, fraction=0.05, pad=0.02)

        if bounds:
            # Adjust the ticks for the discrete colormap
            num_colors = len(cmap)
            tick_locs = (  (np.arange(num_colors) + 0.5)
                         * (num_colors-1)/num_colors)
            cbar.set_ticks(tick_locs)
            cbar.ax.set_yticklabels(cmap.keys())

        hlpr.ax.axis('off')  # TODO should be done by helper

        # Provide configuration options to plot helper
        hlpr.provide_defaults('set_title',
                              title=(title if title else prop_name))

        return im

    # Prepare the data ........................................................
    # Get the group that all datasets are in
    grp = uni['data'][model_name]

    # Collect all data
    all_data = {p: grp[p] for p in to_plot.keys()}
    shapes = [d.shape for p, d in all_data.items()]

    if any([shape != shapes[0] for shape in shapes]):
        raise ValueError("Shape mismatch of properties {}: {}! Cannot plot."
                         "".format(", ".join(to_plot.keys()), shapes))

    # Can now be sure they all have the same shape, 
    # so its fine to take the first shape to extract the number of steps
    num_steps = shapes[0][0]  # TODO use xarray

    # Prepare the figure ......................................................
    # Prepare the figure to have as many columns as there are properties
    hlpr.setup_figure(ncols=len(to_plot),
                      scale_figsize_with_subplots_shape=True)

    # Store the imshow objects such that only the data has to be updated in a
    # following iteration step. Keys will be the property names.
    ims = dict()

    # Do the single plot for all properties, looping through subfigures
    for col_no, (prop_name, props) in enumerate(to_plot.items()):
        # Select the axis
        hlpr.select_axis(col_no, 0)

        # Get the data for this time step
        data = prepare_data(prop_name, all_data=all_data, time_idx=time_idx)
        
        # In the first time step create a new imshow object
        ims[prop_name] = plot_property(prop_name, data=data, **props)

    # End of single frame CA state plot function ..............................
    # NOTE The above variables are all available below, but the update function
    #      is supposed to start plotting from frame 0.
    
    def update_data():
        """Updates the data of the imshow objects"""
        log.info("Plotting animation with %d frames of %d %s each ...",
                 num_steps, len(to_plot),
                 "property" if len(to_plot) == 1 else "properties")

        for time_idx in range(num_steps):
            log.debug("Plotting frame for time index %d ...", time_idx)

            # Loop through the columns
            for col_no, (prop_name, props) in enumerate(to_plot.items()):
                # Select the axis
                hlpr.select_axis(col_no, 0)

                # Get the data for this time step
                data = prepare_data(prop_name,
                                    all_data=all_data, time_idx=time_idx)

                # Update imshow data without creating a new object
                ims[prop_name].set_data(data)

                # If no limits are provided, autoscale the new limits in the
                # case of continuous colormaps. A discrete colormap, that is
                # provided as a dict, should never have to autoscale.
                if not isinstance(props.get('cmap'), dict):
                    if not props.get('limits'):
                        ims[prop_name].autoscale()

            # Done with this frame; yield control to the animation framework
            # which will grab the frame...
            yield

        log.info("Animation finished.")

    # Register this update method with the helper, which takes care of the rest
    hlpr.register_animation_update(update_data)

# -----------------------------------------------------------------------------

# Deprecated state_anim function
def state_anim(dm: DataManager, *, 
               out_path: str, 
               uni: UniverseGroup, 
               model_name: str,
               to_plot: dict,
               writer: str,
               frames_kwargs: dict=None, 
               base_figsize: tuple=None,
               fps: int=2, 
               dpi: int=96,
               preprocess_funcs: Dict[str, Callable]=None) -> None:
    """Create an animation of the states of a two dimensional cellular automaton.
    The function can use different writers, e.g. write out only the frames or create
    an animation with an external program (e.g. ffmpeg). 
    Multiple properties can be plotted next to each other, specified by the to_plot dict.
    
    Arguments:
        dm (DataManager): The DataManager object containing the data
        out_path (str): The output path
        uni (UniverseGroup): The selected universe data
        model_name (str): The name of the model instance, in which the data is
            located.
        to_plot (dict): The plotting configuration. The entries of this key
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels.
        writer (str): The writer that should be used. Additional to the
            external writers such as 'ffmpeg', it is possible to create and
            save the individual frames with 'frames'
        frames_kwargs (dict, optional): The frames configuration that is used
            if the 'frames' writer is used.
        base_figsize (tuple, optional): The size of the figure to use for a
            single property
        fps (int, optional): The frames per second
        dpi (int, optional): The dpi setting
        preprocess_funcs (Dict[str, Callable], optional): For keys matching
            the keys in `to_plot`, the given function is called before the
            data is passed on to the plotting function.
    
    Raises:
        ValueError: For an invalid `writer` argument
    """
    log.warning("The 'ca.state_anim' plot function is deprecated and will "
                "soon be removed! Please use the 'ca.state' plot function "
                "instead.")

    def plot_property(*, data, ax, cmap, limits: list=None, title: str=None):
        """Helper function to plot a property on a given axis and return
        an imshow object
        """

        # Get colormap
        # Case continuous colormap
        if isinstance(cmap, str):
            norm = None
            bounds = None
            colormap = cmap

        # Case discrete colormap
        elif isinstance(cmap, dict):
            colormap = ListedColormap(cmap.values())
            bounds = limits
            norm = mpl.colors.BoundaryNorm(bounds, colormap.N)

        else:
            raise TypeError("Argument cmap needs to be either a string with "
                            "name of the colormap or a dict with values for a "
                            "discrete colormap. Was: {} with value: '{}'"
                            "".format(type(cmap), cmap))

        # Create imshow
        if limits:
            im = ax.imshow(data, cmap=colormap, animated=True,
                        origin='lower', vmin=limits[0], vmax=limits[1])
        else:
            im = ax.imshow(data, cmap=colormap, animated=True,
                        origin='lower')

        # Set title
        if title is not None:
            ax.set_title(title, fontsize=20)

        # Create colorbars
        fig = plt.gcf()
        cbar = fig.colorbar(im, ax=ax, norm=norm, ticks=bounds,
                            fraction=0.05, pad=0.02)

        if bounds is not None:
            # Adjust the ticks for the discrete colormap
            num_colors = len(cmap)
            tick_locs = (  (np.arange(num_colors) + 0.5)
                         * (num_colors-1)/num_colors)
            cbar.set_ticks(tick_locs)
            cbar.ax.set_yticklabels(cmap.keys())

        ax.axis('off')

        return im

    def prepare_data(*, data_2d: dict, prop_name: str, t: int) -> np.ndarray:
        """Prepares the data for plotting"""
        # Get the data from the dict of 2d data
        data = data_2d[prop_name][t]

        # If preprocessing is available for this property, call that function
        if preprocess_funcs and prop_name in preprocess_funcs:
            data = preprocess_funcs[prop_name](data)

        return data


    # Prepare the data ........................................................
    # Get the group that all datasets are in
    grp = uni['data'][model_name]

    data_2d = {p: grp[p] for p in to_plot.keys()}
    shapes = [d.shape for p, d in data_2d.items()]

    if any([shape != shapes[0] for shape in shapes]):
        raise ValueError("Shape mismatch; cannot plot.")

    # Can now be sure they all have the same shape, 
    # so its fine to take the first shape to extract the number of steps
    steps = shapes[0][0]
    
    # Prepare the writer ......................................................
    if writer == 'frames':
        # Use the file writer to create individual frames
        w = FileWriter(name_padding=len(str(steps)),
                       **(frames_kwargs if frames_kwargs else {}))

    elif mpl.animation.writers.is_available(writer):
        # Create animation writer if the writer is available
        wCls = mpl.animation.writers[writer]
        w = wCls(fps=fps,
                 metadata=dict(title="Grid Animation — {}"
                                     "".format(", ".join(to_plot.keys()),
                               artist="Utopia")))

    else:
        raise ValueError("The writer '{}' is not available on your system!"
                         "".format(writer))


    # Set up plotting .........................................................
    # Create figure, not squeezing to always have axs be something iterable
    fig, axs = plt.subplots(1, len(to_plot), squeeze=False,
                            figsize=base_figsize)

    # Adjust figure size in width to accommodate all properties
    figsize = fig.get_size_inches()
    fig.set_size_inches(figsize[0] * len(to_plot), figsize[1])

    # Store the imshow objects such that only the data has to be updated in a
    # following iteration step. Keys will be the property names
    ims = dict()
    
    log.info("Plotting %d frames of %d %s each ...",
             steps, len(to_plot),
             "property" if len(to_plot) == 1 else "properties")

    with w.saving(fig, out_path, dpi=dpi):
        # Loop through time steps, corresponding to rows in the 2d arrays
        for t in range(steps):
            log.debug("Plotting frame %d ...", t)

            # Loop through the subfigures
            for ax, (prop_name, props) in zip(axs.flat, to_plot.items()):
                # Get the data for this time step
                data = prepare_data(data_2d=data_2d, prop_name=prop_name, t=t)
                
                # In the first time step create a new imshow object
                if t == 0:
                    ims[prop_name] = plot_property(data=data, ax=ax, **props)

                    # Important to not continue with the rest
                    continue

                # For t>0, it is better to update imshow data without creating
                # a new object
                ims[prop_name].set_data(data)

                # If no limits are provided, autoscale the new limits
                # in the case of continuous colormaps.
                # A discrete colormap, that is provided as a dict, should never
                # autoscale. 
                if not isinstance(to_plot[prop_name]['cmap'], dict):
                    if not ('limits' in to_plot[prop_name]):
                        ims[prop_name].autoscale()
            
            # Updated all subfigures now and can now tell the writer that the
            # frame is finished and can be grabbed.
            w.grab_frame()

    log.info("Finished plotting of %d frames.", steps)
