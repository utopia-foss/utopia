"""This module provides graph plotting functions."""

import os
import copy
import logging
import warnings
from itertools import product, chain
from typing import Sequence, Union, Callable, Dict, Tuple, Any

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import networkx as nx
import xarray as xr

from utopya.plotting import is_plot_func, PlotHelper
from utopya.plot_funcs._graph import GraphPlot
from utopya.plot_funcs._mpl_helpers import ColorManager

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------
# Helper Methods

def graph_array_from_group(
    graph_group,
    *,
    graph_creation: dict=None,
    register_property_maps: dict=None,
    clear_existing_property_maps: bool=True,
    times: dict=None,
    sel: dict=None,
    isel: dict=None
) -> xr.DataArray:
    """From a ``GraphGroup`` creates a DataArray containing the networkx graphs
    created from the graph group at the specified points in the group's
    coordinate space.
    
    From all coordinates provided via the selection kwargs the cartesian
    product is taken. Each of those points represents one entry in the returned
    DataArray. The selection kwargs in ``graph_creation`` are ignored silently.
    
    Args:
        graph_group: The graph group
        graph_creation (dict, optional): Graph creation configuration
        register_property_maps (dict, optional): Property maps to be registered
            before graph creation
        clear_existing_property_maps (bool, optional): Whether to remove any
            existing property map at first
        times (dict, optional): *Deprecated*: Equivalent to a sel.time entry
        sel (dict, optional): Select by value
        isel (dict, optional): Select by index
    
    Returns:
        xr.DataArray: networkx graphs with the respective graph group coordinates
    """
    # Remove all selection kwargs from the configuration
    graph_creation = (
        copy.deepcopy(graph_creation) if graph_creation is not None else {}
    )
    graph_creation.pop("at_time", None)
    graph_creation.pop("at_time_idx", None)
    graph_creation.pop("sel", None)
    graph_creation.pop("isel", None)

    # Parse the selectors
    sel = sel if sel else {}
    isel = isel if isel else {}

    # Any ``times`` entry is transformed into the respective sel/isel entry.
    # TODO The ``times`` argument is deprecated. Remove it eventually.
    if times is not None:
        warnings.warn(
            "The 'times' argument is deprecated and will be removed. Use the "
            "'sel' and 'isel' arguments instead to specify a time selection.",
            DeprecationWarning
        )

        if len(times) > 1:
            raise TypeError(
                "Received ambiguous animation time specifications. Need "
                "_one_ of: from_property, sel , isel"
            )

        if "from_property" in times:
            sel["time"] = list(
                graph_group._get_item_or_pmap(times["from_property"])
                .coords["time"].values
            )

        elif "sel" in times:
            sel["time"] = times["sel"]

        elif "isel" in times:
            isel["time"] = times["isel"]

    # If needed, extract coordinates from property data
    for dim in sel:
        if isinstance(sel[dim], dict) and "from_property" in sel[dim]:
            sel[dim] = list(
                graph_group._get_item_or_pmap(sel[dim]["from_property"])
                .coords[dim].values
            )

    # With the given selectors, can now set up the graph-DataArray and create
    # a graph from each coordinate combination.
    # In order to take the cartesion product of all (sel and isel) selectors,
    # collect them all and separate them again afterwards.
    dims = list(sel.keys()) + list(isel.keys())
    coords = {d: [] for d in dims}
    
    all_selectors = [
        c if isinstance(c, (list, tuple, np.ndarray)) else [c]
        for c in chain(sel.values(), isel.values())
    ]

    graphs = np.empty(tuple(len(c) for c in all_selectors), dtype="object")
    
    indices = [[i for i in range(len(c))] for c in all_selectors]

    for idxs, selector in zip(product(*indices), product(*all_selectors)):
        sel_coords = selector[:len(sel)]
        isel_coords = selector[len(sel):]

        # Prepare the selectors for the single points in coordinate space
        select = {d: c for d, c in zip(sel.keys(), sel_coords)}
        iselect = {d: c for d, c in zip(isel.keys(), isel_coords)}

        g = GraphPlot.create_graph_from_group(
            graph_group=graph_group,
            register_property_maps=register_property_maps,
            clear_existing_property_maps=clear_existing_property_maps,
            sel=select,
            isel=iselect,
            **graph_creation,
        )

        graphs[idxs] = g

        # Extract the coordinate values from the networkx graph attributes
        for d, c in g.graph.items():
            if coords[d] == [] or coords[d][-1] != c:
                coords[d].append(c)

    coords = {d: coords[d] for d in dims}

    # Combine the data and coordinate information in a DataArray
    graphs = xr.DataArray(graphs, dims=dims, coords=coords)

    return graphs

def graph_animation_update(
    *,
    hlpr: PlotHelper,
    graphs: xr.DataArray=None,
    graph_group=None,
    graph_creation: dict=None,
    register_property_maps: dict=None,
    clear_existing_property_maps: bool=True,
    positions: dict=None,
    animation_kwargs: dict=None,
    suptitle_kwargs: dict=None,
    **drawing_kwargs
):
    """Graph animation frame generator. Yields whenever the plot helper may
    grab the current frame.

    If ``graphs`` is given, the networkx graphs in the array are used to create
    the frames.

    Otherwise, use a graph group. The frames are defined via the selectors in
    ``animation_kwargs``. From all provided coordinates the cartesian product
    is taken. Each of those points defines one graph and thus one frame.
    The selection kwargs in ``graph_creation`` are ignored silently.

    Args:
        hlpr (PlotHelper): The plot helper
        graphs (xr.DataArray, optional): Networkx graphs to draw. The array
            will be flattened beforehand.
        graph_group (None, optional): Required if ``graphs`` is None. The
            GraphGroup from which to generate the animation frames as specified
            via sel and isel in ``animation_kwargs``.
        graph_creation (dict, optional): Graph creation configuration. Passed
            to :py:meth:`~utopya.plot_funcs._graph.GraphPlot.create_graph_from_group`
            if ``graph_group`` is given.
        register_property_maps (dict, optional): Passed to
            :py:meth:`~utopya.plot_funcs._graph.GraphPlot.create_graph_from_group`
            if ``graph_group`` is given.
        clear_existing_property_maps (bool, optional): Passed to
            :py:meth:`~utopya.plot_funcs._graph.GraphPlot.create_graph_from_group`
            if ``graph_group`` is given.
        positions (dict, optional): The node position configuration.
            If ``update_positions`` is True the positions are reconfigured for
            each frame.
        animation_kwargs (dict, optional): Animation configuration. The
            following arguments are allowed:

            times (dict, optional):
                *Deprecated*: Equivaluent to a sel.time entry.
            sel (dict, optional):
                Select by value. Coordinate values (or ``from_property`` entry)
                keyed by dimension name.
            isel (dict, optional):
                Select by index. Coordinate indices keyed by dimension. May be
                given together with ``sel`` if no key appears in both.
            update_positions (bool, optional):
                Whether to reconfigure the node positions for each frame
                (default=False).
            update_colormapping (bool, optional):
                Whether to reconfigure the nodes' and edges'
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager` for
                each frame (default=False). If False, the colormapping (and the
                colorbar) is configured with the first frame and then fixed.
            skip_empty_frames (bool, optional):
                Whether to skip the frames where the selected graph is missing
                or of a type different than ``nx.Graph`` (default=False).
                If False, such frames are empty.

        suptitle_kwargs (dict, optional): Passed on to the PlotHelper's
            ``set_suptitle`` helper function. Only used in animation mode.
            The ``title`` can be a format string containing a placeholder with
            the dimension name as key for each dimension along which selection
            is done. The format string is updated for each frame of the
            animation. The default is ``<dim-name> = {<dim-name>}`` for each
            dimension.
        **drawing_kwargs: Passed to :py:class:`~utopya.plot_funcs._graph.GraphPlot`
    """
    suptitle_kwargs = (
        copy.deepcopy(suptitle_kwargs) if suptitle_kwargs is not None else {}
    )
    animation_kwargs = (
        copy.deepcopy(animation_kwargs) if animation_kwargs is not None else {}
    )
    update_positions = animation_kwargs.pop("update_positions", False)
    update_colormapping = animation_kwargs.pop("update_colormapping", False)
    skip_empty_frames = animation_kwargs.pop("skip_empty_frames", False)
    sel = animation_kwargs.pop("sel", None)
    isel = animation_kwargs.pop("isel", None)

    if graphs is None:
        graphs = graph_array_from_group(
            graph_group=graph_group,
            graph_creation=graph_creation,
            register_property_maps=register_property_maps,
            clear_existing_property_maps=clear_existing_property_maps,
            sel=sel,
            isel=isel,
            **animation_kwargs,
        )

    else:
        # Apply selectors to the graphs DataArray
        if sel is not None:
            graphs = graphs.sel(**sel)
        if isel is not None:
            graphs = graphs.isel(**isel)

    # Prepare the suptitle format string once for all frames
    if "title" not in suptitle_kwargs:
        suptitle_kwargs["title"] = "; ".join(
            [f"{dim}" + " = {" + dim + "}" for dim in graphs.coords]
        )
 
    def get_graph_and_coords(graphs: xr.DataArray):
        """Generator that yields the (graph, coordinates) pairs"""
        dims = graphs.dims
        if dims:
            # Stack all dimensions of the xr.DataArray
            graphs = graphs.stack(_flat=dims)

            # For each entry, yield the graph and the coords
            for el in graphs:
                g = el.item()
                coords = {
                    d: c for d, c in zip(dims, el.coords["_flat"].item())
                }
                # Also get the scalar coordinates which were not stacked
                coords.update(
                    {
                        d: c.item()
                        for d, c in el.coords.items()
                        if d != "_flat"
                    }
                )
                yield g, coords

        else:
            # zero-dimensional: extract single graph item
            g = graphs.item()
            coords = {d: c.item() for d, c in graphs.coords.items()}
            yield g, coords

    # Don't show the axis. This is also done in GraphPlot.draw but need to do
    # it here in case the first frame is empty.
    hlpr.ax.axis("off")

    # Indicator for things to be done only once after the first GraphPlot.draw
    # call.
    _first_graph = True

    # Always configure the colormanagers on the first GraphPlot.draw call.
    # Overwritten by the update_colormapping kwarg afterwards.
    _update_colormapping = True

    # Loop over all remaining graphs
    for g, coords in get_graph_and_coords(graphs):

        _missing_val = not isinstance(g, nx.Graph)

        # On missing entries, skip the frame if skip_empty_frames=True.
        # If skip_empty_frames=False, the suptitle helper is applied but no
        # graph is drawn.
        if _missing_val and skip_empty_frames:
            continue

        if not _missing_val:
            # Use a new GraphPlot for the next frame such that the `select`
            # kwargs are re-evaluated.
            gp = GraphPlot(
                g=g,
                fig=hlpr.fig,
                ax=hlpr.ax,
                positions=positions,
                **drawing_kwargs,
            )
            gp.draw(
                suppress_cbar=not _update_colormapping,
                update_colormapping=_update_colormapping,
            )

        # Update the suptitle format string and invoke the helper
        st_kwargs = copy.deepcopy(suptitle_kwargs)
        st_kwargs["title"] = st_kwargs["title"].format(**coords)
        hlpr.invoke_helper("set_suptitle", **st_kwargs)

        if _first_graph:
            # Fix the y-position of the suptitle after the first invocation
            # since repetitive invocations of the set-suptitle helper would
            # re-adjust the subplots size each time. Use the matplotlib default
            # if not given.
            if "y" not in suptitle_kwargs:
                suptitle_kwargs["y"] = 0.98

            _update_colormapping = update_colormapping

        # Let the writer grab the current frame
        yield
        
        if not _missing_val:
            # Clean up the frame
            gp.clear_plot(keep_colorbars=not _update_colormapping)
            
            if _first_graph:
                # If the positions should be fixed, overwrite the positions arg
                if not update_positions:
                    positions = dict(from_dict=gp.positions)

                # Done handling the first GraphPlot.draw call
                _first_graph = False

# -----------------------------------------------------------------------------
# Plot functions

@is_plot_func(use_dag=True, supports_animation=True)
def draw_graph(
    *,
    hlpr: PlotHelper,
    data: dict,
    graph_group_tag: str="graph_group",
    graph: Union[nx.Graph, xr.DataArray]=None,
    graph_creation: dict=None,
    graph_drawing: dict=None,
    graph_animation: dict=None,
    register_property_maps: Sequence[str]=None,
    clear_existing_property_maps: bool=True,
    suptitle_kwargs: dict=None
):
    """Draws a graph either from a :py:class:`~utopya.datagroup.GraphGroup` or
    directly from a ``networkx.Graph`` using the
    :py:class:`~utopya.plot_funcs._graph.GraphPlot` class.

    If the graph object is to be created from a graph group the latter needs to
    be selected via the TransformationDAG. Additional property maps can also be
    made available for plotting, see ``register_property_map`` argument.
    Animations can be created either from a graph group by using the select
    interface in ``graph_animation`` or by passing a DataArray of networkx
    graphs via the ``graph`` argument.

    For more information on how to use the transformation framework, refer to
    the `dantro documentation <https://dantro.readthedocs.io/en/stable/plotting/plot_data_selection.html>`_.

    For more information on how to configure the graph layout refer to the
    :py:class:`~utopya.plot_funcs._graph.GraphPlot` documentation.

    Args:
        hlpr (PlotHelper): The PlotHelper instance for this plot
        data (dict): Data from TransformationDAG selection
        graph_group_tag (str, optional): The TransformationDAG tag of the graph
            group
        graph (Union[nx.Graph, xr.DataArray], optional): If given, the ``data``
            and ``graph_creation`` arguments are ignored and this graph is
            drawn directly.
            If a DataArray of graphs is given, the first graph is drawn for a
            single graph plot. In animation mode the (flattened) array
            represents the animation frames.
        graph_creation (dict, optional): Configuration of the graph creation.
            Passed to ``GraphGroup.create_graph``.
        graph_drawing (dict, optional): Configuration of the graph layout.
            Passed to :py:class:`~utopya.plot_funcs._graph.GraphPlot`.
        graph_animation (dict, optional): Animation configuration. The
            following arguments are allowed:

            times (dict, optional):
                *Deprecated*: Equivaluent to a sel.time entry.
            sel (dict, optional):
                Select by value. Dictionary with dimension names as keys. The
                values may either be coordinate values or a dict with a single
                ``from_property`` (str) entry which specifies a container
                withing the GraphGroup or registered external data from which
                the coordinates are extracted.
            isel (dict, optional):
                Select by index. Coordinate indices keyed by dimension. May be
                given together with ``sel`` if no key appears in both.
            update_positions (bool, optional):
                Whether to update the node positions for each frame by
                recalculating the layout with the parameters specified in
                graph_drawing.positions. If this parameter is not given or
                false, the positions are calculated once initially and then
                fixed.
            update_colormapping (bool, optional):
                Whether to reconfigure the nodes' and edges'
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager` for
                each frame (default=False). If False, the colormapping (and the
                colorbar) is configured with the first frame and then fixed.
            skip_empty_frames (bool, optional):
                Whether to skip the frames where the selected graph is missing
                or of a type different than ``nx.Graph`` (default=False).
                If False, such frames are empty.

        register_property_maps (Sequence[str], optional): Names of properties
            to be registered in the graph group before the graph creation.
            The property names must be valid TransformationDAG tags, i.e., be
            available in ``data``. Note that the tags may not conflict with any
            valid path reachable from inside the selected ``GraphGroup``.
        clear_existing_property_maps (bool, optional): Whether to clear any
            existing property maps from the selected ``GraphGroup``. This is
            enabled by default to reduce side effects from previous plots.
            Set this to False if you have property maps registered with the
            GraphGroup that you would like to keep.
        suptitle_kwargs (dict, optional): Passed on to the PlotHelper's
            ``set_suptitle`` helper function. Only used in animation mode.
            The ``title`` can be a format string containing a placeholder with
            the dimension name as key for each dimension along which selection
            is done. The format string is updated for each frame of the
            animation. The default is ``<dim-name> = {<dim-name>}`` for each
            dimension.

    Raises:
        ValueError: On invalid or non-computed TransformationDAG tags in
            ``register_property_maps`` or invalid graph group tag.
    """
    # Work on a copy such that the original configuration is not modified
    graph_drawing = copy.deepcopy(graph_drawing if graph_drawing else {})

    # Get the sub-configurations for the drawing of the graph
    select = graph_drawing.get("select", {})
    pos_kwargs = graph_drawing.get("positions", {})
    node_kwargs = graph_drawing.get("nodes", {})
    edge_kwargs = graph_drawing.get("edges", {})
    node_label_kwargs = graph_drawing.get("node_labels", {})
    edge_label_kwargs = graph_drawing.get("edge_labels", {})
    mark_nodes_kwargs = graph_drawing.get("mark_nodes", {})
    mark_edges_kwargs = graph_drawing.get("mark_edges", {})

    def get_dag_data(tag):
        try:
            return data[tag]
        except KeyError as err:
            _available_tags = ", ".join(data.keys())
            raise ValueError(
                f"No tag '{tag}' found in the data selected by the DAG! Make "
                "sure the tag is named correctly and is selected to be "
                "computed; adjust the 'compute_only' argument if needed."
                "\nThe following tags are available in the DAG results: "
                f"{_available_tags}"
            ) from err

    # Prepare graph group and external property data
    graph_group = get_dag_data(graph_group_tag) if graph is None else None
    property_maps = None
    if register_property_maps:
        property_maps = {}
        for tag in register_property_maps:
            property_maps[tag] = get_dag_data(tag)

    # If not in animation mode, make a single graph plot
    if not hlpr.animation_enabled:
        # Set up a GraphPlot instance
        if graph_group is not None:
            # Create GraphPlot from graph group
            gp = GraphPlot.from_group(
                graph_group=graph_group,
                graph_creation=graph_creation,
                register_property_maps=property_maps,
                clear_existing_property_maps=clear_existing_property_maps,
                fig=hlpr.fig,
                ax=hlpr.ax,
                **graph_drawing,
            )

        else:
            if graph_creation is not None:
                warnings.warn(
                    "Received both a 'graph' argument and a 'graph_creation' "
                    "configuration. The latter will be ignored. To remove "
                    "this warning set graph_creation to None.",
                    UserWarning
                )

            if isinstance(graph, xr.DataArray):
                # Use the first array element for a single graph plot
                g = graph.values.flat[0]

                if graph.size > 1:
                    log.caution(
                        "Received a DataArray of size %d as 'graph' argument, "
                        "performing a single plot using the first entry. "
                        "Animations must be enabled by setting "
                        "animation.enabled to True.",
                        graph.size,
                    )

            else:
                g = graph
            
            # Create a GraphPlot from a nx.Graph
            gp = GraphPlot(g=g, fig=hlpr.fig, ax=hlpr.ax, **graph_drawing)

        # Make the actual plot via the GraphPlot
        gp.draw()

    # In animation mode, register the animation frame generator
    else:
        # Prepare animation kwargs for the update routine
        graph_animation = graph_animation if graph_animation else {}

        def update():
            """The animation frames generator.
            See :py:meth:`~utopya.plot_funcs.dag.graph.graph_animation_update`.
            """
            if graph_group is None and not isinstance(graph, xr.DataArray):
                raise TypeError(
                    "Failed to create animation due to invalid type of the "
                    "'graph' argument. Required: xr.DataArray (filled "
                    f"with graph objects). Received: {type(graph)}"
                )

            yield from graph_animation_update(
                hlpr=hlpr,
                graphs=graph if isinstance(graph, xr.DataArray) else None,
                graph_group=graph_group,
                graph_creation=graph_creation,
                register_property_maps=property_maps,
                clear_existing_property_maps=clear_existing_property_maps,
                suptitle_kwargs=suptitle_kwargs,
                animation_kwargs=graph_animation,
                **graph_drawing,
            )

        hlpr.register_animation_update(update)
