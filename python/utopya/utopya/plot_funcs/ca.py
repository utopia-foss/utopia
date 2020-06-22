"""This module provides plotting functions to visualize cellular automata."""

import copy
import logging
from typing import Union, Dict, Callable

import numpy as np
import xarray as xr

import matplotlib as mpl
from matplotlib.colors import ListedColormap

from .. import DataManager, UniverseGroup
from ..plotting import UniversePlotCreator, PlotHelper, is_plot_func
from ..dataprocessing import transform
from ..tools import recursive_update


# Get a logger
log = logging.getLogger(__name__)

# Increase log threshold for animation module
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
          transform_data: dict=None, transformations_log_level: int=10,
          preprocess_funcs: Dict[str, Callable]=None,
          default_imshow_kwargs: dict=None):
    """Plots the state of the cellular automaton as a 2D heat map.
    This plot function can be used for a single plot, but also supports
    animation.

    Which properties of the state to plot can be defined in ``to_plot``.

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
                - **imshow_kwargs: passed on to imshow invocation

        time_idx (int): Which time index to plot the data of. Is ignored when
            creating an animation.
        transform_data (dict, optional): Transformations to apply to the data.
            The top-level entries must correspond to the entries of `to_plot`.
            This can be used for dimensionality reduction of the data, but
            also for other operations, e.g. to selecting a slice.
            For available parameters, see
            :py:func:`utopya.dataprocessing.transform`
        transformations_log_level (int, optional): The logging level of all the
            data transformation operations.
        preprocess_funcs (Dict[str, Callable], optional): A dictionary of pre-
            processing callables, where keys need to correspond to the
            property name in ``to_plot`` that is to be pre-processed.
            This argument can be used to implement model-specific preprocessing
            by implementing another plot function, which defines this dict and
            passes it to this function.
            NOTE If possible, use ``transform_data``.
        default_imshow_kwargs (dict, optional): The default parameters passed
            to the underlying imshow plotting function. These are updated by
            the values given via ``to_plot``.

    Raises:
        ValueError: Shape mismatch of data selected by ``to_plot``

    Warns:
        Use of transform_data and preprocess_funcs (is called in this order).
    """
    # Helper functions ........................................................

    def prepare_data(prop_name: str, *,
                     all_data: dict, time_idx: int) -> np.ndarray:
        """Prepares the data for plotting"""
        # Get the data from the dict of 2d data
        data = all_data[prop_name][time_idx]

        # If preprocessing is available for this property, call that function
        if transform_data and preprocess_funcs:
            log.warning("Received both arguments `transform_data` and "
                        "`preprocess_funcs`! Will perform first the "
                        "transformations, then the additional preprocessing.")

        if transform_data:
            data = transform(data, *transform_data.get(prop_name, {}),
                             log_level=transformations_log_level)

        if preprocess_funcs and prop_name in preprocess_funcs:
            data = preprocess_funcs[prop_name](data)

        return data

    def plot_property(prop_name: str, *, data: xr.DataArray,
                      limits: list=None, cmap: Union[str, dict]='viridis',
                      title: str=None,
                      no_cbar_markings: bool=False, imshow_kwargs: dict=None,
                      **cbar_kwargs):
        """Helper function to plot a property on a given axis and return
        an imshow object

        Args:
            prop_name (str): The property to plot
            data (xr.DataArray): The array-like data to plot as image
            limits (list, optional): The imshow limits to use; will also be
                the limits of the colorbar.
            cmap (Union[str, dict], optional): The colormap to use
            title (str, optional): The title of this figure
            no_cbar_markings (bool, optional): Whether to suppress colorbar
                markings (ticks and tick labels)
            imshow_kwargs (dict, optional): Passed to plt.imshow
            **cbar_kwargs: Passed to fig.colorbar

        Returns:
            imshow object

        Raises:
            TypeError: For invalid ``cmap`` argument.
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

        # Fill imshow_kwargs, using defaults
        imshow_kwargs = imshow_kwargs if imshow_kwargs else {}
        imshow_kwargs = recursive_update(copy.deepcopy(imshow_kwargs),
                                         default_imshow_kwargs
                                         if default_imshow_kwargs else {})
        if limits:
            imshow_kwargs['vmin'] = limits[0]
            imshow_kwargs['vmax'] = limits[1]

        # Create imshow object on the currently selected axis
        im = hlpr.ax.imshow(data.T, cmap=colormap, animated=True,
                            origin='lower', aspect='equal',
                            **imshow_kwargs)

        # Parse additional colorbar kwargs and set some default values
        add_cbar_kwargs = dict()
        if 'fraction' not in cbar_kwargs:
            add_cbar_kwargs['fraction'] = 0.05

        if 'pad' not in cbar_kwargs:
            add_cbar_kwargs['pad'] = 0.02

        # Create the colorbar
        cbar = hlpr.fig.colorbar(im, ax=hlpr.ax, norm=norm, ticks=bounds,
                                 **cbar_kwargs, **add_cbar_kwargs)
        # TODO Should be done by helper

        # For a discrete colormap, adjust the tick positions
        if bounds:
            num_colors = len(cmap)
            tick_locs = (  (np.arange(num_colors) + 0.5)
                         * (num_colors-1)/num_colors)
            cbar.set_ticks(tick_locs)
            cbar.ax.set_yticklabels(cmap.keys())

        # Remove markings, if configured to do so
        if no_cbar_markings:
            cbar.set_ticks([])
            cbar.ax.set_yticklabels([])

        # Remove main axis labels and ticks
        hlpr.ax.axis('off')

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
                ims[prop_name].set_data(data.T)

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
