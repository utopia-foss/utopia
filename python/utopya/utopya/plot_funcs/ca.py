"""This module provides plotting functions to visualize cellular automata."""

import copy
import logging
from typing import Union, Dict, Callable

import numpy as np
import xarray as xr

import matplotlib as mpl
from matplotlib.colors import ListedColormap
from matplotlib.collections import RegularPolyCollection
from mpl_toolkits.axes_grid1 import make_axes_locatable

from .. import DataManager, UniverseGroup
from ..plotting import UniversePlotCreator, PlotHelper, is_plot_func
from ..dataprocessing import transform
from ..tools import recursive_update


# Get a logger
log = logging.getLogger(__name__)

# Increase log threshold for animation module
logging.getLogger('matplotlib.animation').setLevel(logging.WARNING)


# -----------------------------------------------------------------------------

def _get_ax_size(ax, fig) -> tuple:
    """The width and height of the given axis in pixels"""
    bbox = ax.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
    width, height = bbox.width, bbox.height
    width *= fig.dpi
    height *= fig.dpi
    return width, height


def imshow_hexagonal(data: xr.DataArray, *, hlpr: PlotHelper,
                     colormap: Union[str, mpl.colors.Colormap],
                     **kwargs
                     ) -> mpl.image.AxesImage:
    """Display data as an image, i.e., on a 2D hexagonal grid.

    Args:
        data (xr.DataArray): The array-like data to plot as image.
        hlpr (PlotHelper): The plot helper.
        colormap (str or mpl.colors.Colormap): The colormap to use.

    Returns:
        The RegularPolyCollection representing the hexgrid.
    """
    width, height = _get_ax_size(hlpr.ax, hlpr.fig)
    s = (height / data.y.size) / 0.75 / 2
    # NOTE the 0.75 factor is required because of the hexagonal offset geometry
    area = 3**1.5 / 2 * s**2

    # distinguish pair and impair rows (impair have offset)
    hex_s = 2 * data.isel(y=0).y
    y_ids = ((data.y / hex_s - 0.5) / 0.75).round().astype(int)

    # compute offsets of polygons
    xx, yy = np.meshgrid(data.x, data.y)
    x_offsets = xr.DataArray(data=xx,
                             dims=("y", "x"),
                             coords=dict(x=data.x, y=data.y))
    y_offsets = xr.DataArray(data=yy,
                             dims=("y", "x"),
                             coords=dict(x=data.x, y=data.y))

    # ... and add an x-offset for impair rows
    x_origin = data.isel(x=0, y=0).coords['x']
    x_offsets[y_ids % 2 == 1] += x_origin

    # # assign the true coordinates
    # d = d.assign_coords(x=('x', d.x))
    # d_offset = d_offset.assign_coords(x=('x', d_offset.x + x_origin))

    # get the color mapping
    if isinstance(colormap, str):
        cmap = mpl.cm.get_cmap(name=colormap)
    else:
        cmap = colormap
    map_color = mpl.cm.ScalarMappable(cmap=cmap)

    pcoll = RegularPolyCollection(
        6, sizes=(area,), rotation=0,
        facecolor=map_color.to_rgba(data.data.flatten()),
        offsets=np.transpose(
            [x_offsets.data.flatten(), y_offsets.data.flatten()]
        ),
        transOffset=hlpr.ax.transData,
        animated=True,
    )
    hlpr.ax.add_collection(pcoll)

    # use same length scale in x and y
    hlpr.ax.set_aspect('equal')

    # rescale cmap
    im = mpl.image.AxesImage(hlpr.ax)

    im.set_cmap(colormap)
    if 'vmin' in kwargs and 'vmax' in kwargs:
        # From `limits` argument; will either have none or both
        im.set_clim(kwargs['vmin'], kwargs['vmax'])


    return im


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
                - \**imshow_kwargs: passed on to imshow invocation

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
            \**cbar_kwargs: Passed to fig.colorbar

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

        # Parse additional colorbar kwargs and set some default values
        add_cbar_kwargs = dict()
        if 'fraction' not in cbar_kwargs:
            add_cbar_kwargs['fraction'] = 0.05

        if 'pad' not in cbar_kwargs:
            add_cbar_kwargs['pad'] = 0.02

        # Fill imshow_kwargs, using defaults
        imshow_kwargs = imshow_kwargs if imshow_kwargs else {}
        imshow_kwargs = recursive_update(copy.deepcopy(imshow_kwargs),
                                         default_imshow_kwargs
                                         if default_imshow_kwargs else {})
        if limits:
            imshow_kwargs['vmin'] = limits[0]
            imshow_kwargs['vmax'] = limits[1]

        # Create imshow object on the currently selected axis
        structure = data.attrs.get('grid_structure')
        if structure == 'square' or structure is None:
            im = hlpr.ax.imshow(data.T, cmap=colormap, animated=True,
                                origin='lower', aspect='equal',
                                **imshow_kwargs)

        elif structure == 'hexagonal':
            hlpr.ax.clear()
            im = imshow_hexagonal(data=data, hlpr=hlpr, animated=True,
                                  colormap=colormap, **imshow_kwargs)
            title=(title if title else prop_name)
            hlpr.ax.set_title(title)


        elif structure == 'triangular':
            raise ValueError("Plotting of triangular grid not implemented!")

        else:
            raise ValueError("Unknown grid structure '{}'!", structure)

        # Create the colorbar
        # For hexagonal grids, manually create a separate axis next to the current
        # plotting axis. Add a cbar and manually set the ticks and boundaries (as
        # the cbar returned by the mpl.image.AxesImage has default range (0, 1)
        if structure == 'hexagonal':
            cax.clear()
            if bounds:
                num_colors = len(cmap)
                boundaries = [i/num_colors for i in range(num_colors+1)]
                tick_locs = [(2*i+1)/(2*num_colors) for i in range(num_colors)]
            else:
                boundaries = None
                tick_locs = [i*0.25 for i in range(5)]
                if isinstance(colormap, str):
                    colormap = mpl.cm.get_cmap(name=colormap)

            cbar = mpl.colorbar.ColorbarBase(cax, cmap=colormap, boundaries = boundaries)
            cbar.set_ticks(tick_locs)

            # For discrete colorbars, set the tick labels at the positions
            # defined; for continuous colorbars, get the upper and lower
            # boundaries from the dataset.
            if bounds:
                cbar.ax.set_yticklabels(cmap.keys())
            else:
                lower = np.min(data.data)
                upper = np.max(data.data)
                if lower == upper:
                    diff = 0.01
                else:
                    diff = upper - lower

                ticklabels = [i*diff/4+lower for i in range(5)]
                cbar.ax.set_yticklabels(ticklabels)

        else:
            cbar = hlpr.fig.colorbar(im, ax=hlpr.ax, ticks=bounds,
                                     **cbar_kwargs, **add_cbar_kwargs)
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

    structure = prepare_data(list(to_plot.keys())[0],
                      all_data=all_data, time_idx=0).attrs.get('grid_structure')


    # Prepare the figure ......................................................
    # Prepare the figure to have as many columns as there are properties
    hlpr.setup_figure(ncols=len(to_plot),
                      scale_figsize_with_subplots_shape=True)
    if structure == 'hexagonal':
        old_figsize = hlpr.fig.get_size_inches()  # (width, height)
        hlpr.fig.set_size_inches(
            old_figsize[0] * 1.25,
            old_figsize[1],
        )

    # Store the imshow objects such that only the data has to be updated in a
    # following iteration step. Keys will be the property names.
    ims = dict()

    # Do the single plot for all properties, looping through subfigures
    for col_no, (prop_name, props) in enumerate(to_plot.items()):
        # Select the axis
        hlpr.select_axis(col_no, 0)

        # For hexagonal grids, add custom colorbar axis
        if structure == 'hexagonal':
            divider = make_axes_locatable(hlpr.ax)
            cax = divider.append_axes("right", size="5%", pad=0.2)
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

                # Get the structure of the grid data
                structure = data.attrs.get('grid_structure')

                # For hexagonal grids recreate the plot each time.
                # Just resetting the data does not show the updated states
                # otherwise because the facecolors have to be updated, too.
                # For other grid structures just update the data and colormap.
                if structure == 'hexagonal':
                    ims[prop_name] = plot_property(prop_name, data=data,
                                                   **props)

                else:
                    # Update imshow data without creating a new object
                    ims[prop_name].set_data(data.T)

                    # If no limits are provided, autoscale the new limits in
                    # the case of continuous colormaps. A discrete colormap,
                    # that is provided as a dict, should never have to
                    # autoscale.
                    if not isinstance(props.get('cmap'), dict):
                        if not props.get('limits'):
                            ims[prop_name].autoscale()

            # Done with this frame; yield control to the animation framework
            # which will grab the frame...
            yield

        log.info("Animation finished.")

    # Register this update method with the helper, which takes care of the rest
    hlpr.register_animation_update(update_data)
