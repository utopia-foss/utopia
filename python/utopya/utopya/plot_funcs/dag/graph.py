"""This module provides a generic function for graph plotting."""
import os
import copy
import logging
import warnings
from typing import Sequence, Union

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import networkx as nx

from utopya.plotting import is_plot_func, PlotHelper
from utopya.plot_funcs._mpl_helpers import ColorManager

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

# Available networkx node layout options
POSITIONING_MODELS_NETWORKX = {
    'spring': nx.spring_layout,
    'circular': nx.circular_layout,
    'shell': nx.shell_layout,
    'bipartite': nx.bipartite_layout,
    'kamada_kawai': nx.kamada_kawai_layout,
    'planar': nx.planar_layout,
    'random': nx.random_layout,
    'spectral': nx.spectral_layout,
    'spiral': nx.spiral_layout,
}

# -----------------------------------------------------------------------------

@is_plot_func(use_dag=True,
              supports_animation=True,
              required_dag_tags=('graph_group',),
              compute_only_required_dag_tags=False)
def draw_graph(*, hlpr: PlotHelper,
               data: dict,
               graph_creation: dict,
               graph_drawing: dict=None,
               graph_animation: dict=None,
               register_property_maps: Sequence[str]=None,
               clear_existing_property_maps: bool=True,
               suptitle_kwargs: dict=None):
    """Draws a graph from a ``GraphGroup``.

    Data selection happens via the dantro data transformation framework. The
    graph group that is to be plotted needs to be selected via the DAG and
    tagged ``graph_group``. Additional property maps can also be made available
    for plotting, see ``register_property_map`` argument.

    For more information on how to use the transformation framework, refer to
    the `dantro documentation <https://dantro.readthedocs.io/en/stable/plotting/plot_data_selection.html>`_.

    .. note::

        For some graph layout properties it is possible to configure an
        automatized mapping from node/edge properties. The *property mapping*
        has the following syntax:

        .. code-block:: yaml

            some_layout_property:
              from_property: my_node_or_edge_property
              scale_to_interval: [low_lim, up_lim]

        The ``from_property`` specifies the node or edge property to be mapped
        from. If ``scale_to_interval`` is given, the layout property values are
        rescaled linearly the specified interval.

        When property mapping is done, its configuration is replaced by the
        created sequence of layout property values.

    Args:
        hlpr (PlotHelper): The PlotHelper instance for this plot
        data (dict): Data from TransformationDAG selection

        graph_creation (dict):
            Configuration of the graph creation. Passed to
            :py:meth:`~dantro.groups.graph.GraphGroup.create_graph`.

        graph_drawing (dict, optional):
            Configuration of the graph layout. The following keys are
            available:

            positions (dict, optional):
                Configuration for the node positioning. The following arguments
                are available:

                model (str, optional):
                    The layout model that is used to calculate the node
                    positions (default: ``spring``). Available
                    `networkx layout models <https://networkx.github.io/documentation/stable/reference/drawing.html#module-networkx.drawing.layout>`_
                    are: ``spring``, ``circular``, ``shell``, ``bipartite``,
                    ``kamada_kawai``, ``planar``, ``random``, ``spectral``,
                    ``spiral``.
                    
                    If installed, `GraphViz <https://pypi.org/project/graphviz/>`_
                    models can be selected with a prepended ``graphviz_``.
                    Options depend on the ``GraphViz`` version but may include:
                    ``dot``, ``neato``, ``fdp``, ``sfdp``, ``twopi``,
                    ``circo``. (Passed as ``prog`` to
                    `networkx.graphviz_layout <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pydot.graphviz_layout.html>`_).
                further kwargs:
                    Passed on to the chosen layout model.

            select (dict, optional):
                Plot only a subgraph induced by a selection of nodes. Either
                select a list of nodes by passing the ``nodelist`` argument or
                do a radial node selection by specifying a ``center`` node and
                the ``radius``. The following arguments can be passed
                additionally:

                open_edges (bool, optional):
                    Whether to plot the edges for which only one of source and
                    destination is in the set of selected nodes. Disabled by
                    default.
                drop (bool, optional):
                    Whether to remove the non-selected nodes from the graph.
                    If False, *all* nodes are passed to the node positioning
                    model. Enabled by default.

            nodes (dict, optional):
                Configuration for the node plotting. The following arguments
                are allowed:

                node_size (scalar or sequence of scalars, optional):
                    The node size (default: 300). Available for property
                    mapping. Can be mapped directly from the nodes' ``degree``,
                    ``in_degree``, or ``out_degree`` by setting the
                    ``from_property`` argument accordingly.
                node_color (color or sequene of colors, optional):
                    Single color (string or RGB(A) tuple or numeric value)
                    or sequence of colors (default: '#1f78b4'). If numeric
                    values are specified they will be mapped to colors using
                    the cmap and vmin, vmax parameters. Available for property
                    mapping.

                    If the ``node_color`` is mapped from categorical property
                    data, it can be mapped to scalar values by providing
                    a ``map_to_scalar`` dict of scalar target values keyed by
                    (categorical) source value.
                cmap (Union[str, dict], optional):
                    The colormap. Passed as ``cmap`` to
                    :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                cmap_norm (Union[str, dict], optional):
                    The norm used for the color-mapping. Passed as ``norm``
                    to :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                    May be overwritten, if a discrete colormap is specified in
                    ``cmap``.
                colorbar (dict, optional):
                    Configuration of the colorbar. The following arguments are
                    allowed:

                    enabled (bool, optional):
                        Whether to plot a colorbar. Enabled by default, if
                        property mapping was done for ``node_color``.
                    labels (dict, optional):
                        Colorbar tick-labels keyed by tick position (see
                        :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`).
                    further kwargs:
                        Passed on to :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`.
                
                further kwargs:
                    After applying property mapping, passed on to
                    `draw_networkx_nodes <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_nodes.html>`_.
                
                The following arguments are available for property mapping:
                ``node_size``, ``node_color``, ``alpha``.

            edges (dict, optional):
                Configuration for the edge plotting. The ``edge_color``,
                ``edge_cmap``, and ``colorbar`` argument behave analogously for
                the edges as nodes.node_color, nodes.cmap, and nodes.colorbar
                for the nodes. Any further kwargs are, after applying property
                mapping, passed on to
                `draw_networkx_edges <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edges.html>`_.
                
                The following arguments are available for property mapping:
                ``edge_color``, ``width``.

            node_labels (dict, optional):
                Configuration for the plotting of node labels. The following
                arguments are allowed:

                enabled (bool, optional):
                    Whether to plot node labels. Disabled by default. If
                    enabled, nodes are labeled by their index by default.
                show_only (list, optional):
                    If given, labels are plotted only for the nodes in this
                    list.
                labels (dict, optional):
                    Dictionary of custom text labels keyed by node.
                    Available for property mapping. If mapped from property,
                    a format string ``format`` with a ``label`` key can be
                    specified, which is used for all node labels.
                further kwargs:
                    Passed on to
                    `draw_networkx_labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_labels.html>`_.

            edge_labels (dict, optional):
                Configuration for the plotting of edge labels. The following
                arguments are allowed:

                enabled (bool, optional):
                    Whether to plot edge labels. Disabled by default. If
                    enabled, edges are labeled by their
                    (source, destination) pair by default.
                show_only (list, optional):
                    If given, labels are plotted only for the edges (2-tuples)
                    in this list.
                edge_labels (dict, optional):
                    Dictionary of custom text labels keyed by edge (2-tuples).
                    Available for property mapping. If mapped from property,
                    a format string ``format`` with a ``label`` key can be
                    specified, which is used for all edge labels.
                further kwargs:
                    Passed on to
                    `draw_networkx_edge_labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edge_labels.html>`_.

            mark_nodes (dict, optional):
                Configuration for highlighting nodes by their edge-color.
                Either specify a ``color`` (str) for a list of nodes
                (``nodelist``), or specify a ``colors`` dictionary of
                colors (str) keyed by node. Creates or updates an existing
                ``nodes.edgecolors`` entry.

            mark_edges(dict, optional):
                Configuration for highlighting edges by their color. Either
                specify a ``color`` (str) for a list of edges (``edgelist``),
                or specify a ``colors`` dictionary of colors (str) keyed by
                edge (2-tuples). Creates or updates an existing
                ``edges.edge_color`` entry.

        graph_animation (dict, optional):
            Only taken into account if an animation is done. Non-optional
            when doing an animation. The animation frames need to be
            specified by passing a ``times`` dictionary which may contain
            the following arguments:

            from_property (str, optional):
                Extract the animation times from the ``time`` coordinates of
                a container within the ``GraphGroup`` or from registered
                external data.
            sel (list, optional):
                Select the times by value.
            isel (list, optional):
                Select the times by index.

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
        suptitle_kwargs (dict, optional): Key passed on to the PlotHelper's
            ``set_suptitle`` helper function. Only used if animations are
            enabled. The ``title`` entry can be a format string with the
            ``value`` key, which is updated for each frame of the animation.
            Default: ``time = {value:d}`` if times given by value,
            ``time idx = {value:d}`` if times given by index.

    Raises:
        ValueError: On invalid or non-computed dag tags in
            ``register_property_maps``.
    """
    def select_from_list(g, *, nodelist: list, open_edges: bool=False):
        """Given a list of nodes, selects all nodes and edges needed for the
        graph plotting. If ``open_edges=False``, those edges are selected for
        which both ends are in ``nodes``.
        
        Args:
            g (nx.Graph): The graph
            nodelist (list): The nodes to be selected
            open_edges (bool, optional): Whether the loose edges (i.e., edges
                with only source *or* destination in ``nodelist``) are plotted.
                If True, the 'outer' nodes are shrinked to size zero.
        
        Returns:
            Tuple containing a list of selected nodes, a list of selected
            edges, and the nodes to be shrinked to size zero.
        """
        subgraph = nx.induced_subgraph(g, nodelist)

        if open_edges:
            # Create an outer subgraph from the given nodes and all their
            # neighbors
            node_selection = set(nodelist)
            outer_nodes = set()

            for n in nodelist:
                outer_nodes.update(nx.all_neighbors(g, n))

            outer_nodes -= node_selection
            node_selection = node_selection.union(outer_nodes)
            subgraph_outer = nx.induced_subgraph(g, node_selection)

            # The set of nodes to shrink is the difference of the two node sets
            nodes_to_shrink = list(subgraph_outer.nodes - set(nodelist))

            # From the outer subgraph remove edges between outer nodes
            edges_to_plot = (subgraph_outer.edges
                             - nx.induced_subgraph(g, outer_nodes).edges)

            return (list(subgraph_outer.nodes),
                    list(edges_to_plot),
                    nodes_to_shrink)

        return list(subgraph.nodes), list(subgraph.edges), None

    def select_radial(g, *, center: int, radius: int, open_edges: bool=False):
        """Selects all nodes around a given center within a given radius
        (measured in numbers of neighborhoods). If ``open_edges=False``, those
        edges are selected for which both ends are in the set of selected
        nodes.
        
        Args:
            g (nx.Graph): The graph
            center (int): index of the central node
            radius (int): selection radius
            open_edges (bool, optional): Whether the loose edges (i.e., edges
                with only source *or* destination in ``nodes``) are plotted.
                If True, the 'outer' nodes are shrinked to size zero.
        
        Returns:
            Tuple containing a list of selected nodes, a list of selected
            edges, and the nodes to be shrinked to size zero.
        """
        # After num_nodes-1 iterations (below), all nodes would be selected
        if radius > g.number_of_nodes()-1:
            radius = g.number_of_nodes()-1

        # Identify the nodes within the given radius around the central node.
        # Start by adding the central nodes and all of its neighbors to a set.
        # Then, iteratively add all neighbors of the previously added nodes to
        # the set, until the given radius is reached.
        # TODO It might be worth testing the computational efficiency of this
        #      (also for large subgraphs) as each node is tried to be added to
        #      the set at least two times.
        # Store the current node selection
        node_selection = set([center])
        # Store the nodes added to the selection in the previous step
        nbs_prev = set([center])
        # Store the new nodes to be selected
        nbs_new = set()

        for i in range(radius):
            for n in nbs_prev:
                nbs_new.update(nx.all_neighbors(g, n))

            nbs_prev = nbs_new - node_selection
            node_selection = node_selection.union(nbs_new)
            nbs_new.clear()

        if open_edges:
            # Create an inner subgraph from all nodes within the given radius
            subgraph_inner = nx.induced_subgraph(g, node_selection)

            # Create an outer subgraph from all nodes within radius=radius+1
            for n in nbs_prev:
                nbs_new.update(nx.all_neighbors(g, n))

            outer_nodes = nbs_new - node_selection
            node_selection = node_selection.union(nbs_new)
            subgraph_outer = nx.induced_subgraph(g, node_selection)

            # The set of nodes to shrink is the difference of the two node sets
            nodes_to_shrink = list(subgraph_outer.nodes - subgraph_inner.nodes)

            # From the outer subgraph remove edges between outer nodes
            edges_to_plot = (subgraph_outer.edges
                             - nx.induced_subgraph(g, outer_nodes).edges)

            # Return the inner subgraph nodes and the outer subgraph edges
            return (list(subgraph_outer.nodes),
                    list(edges_to_plot),
                    nodes_to_shrink)

        subgraph = nx.induced_subgraph(g, node_selection)

        return list(subgraph.nodes), list(subgraph.edges), None

    def scale_to_interval(data: list, interval=None):
        """Rescales the data linearly to the given interval. If not interval is
        given the data is returned as it is.
        
        Args:
            data (list): data that is rescaled linearly to the given interval
            interval (Sequence, optional): The target interval
        
        Returns:
            list: rescaled data
        
        Raises:
            TypeError: On invalid interval specification
        """
        if not interval:
            return data

        if len(interval)!=2:
            raise TypeError("'interval' must be a 2-tuple or list of length 2!"
                            f"Was: {interval}")

        data = np.array(data)
        max_val = np.max(data)
        min_val = np.min(data)

        if max_val > min_val:
            rescaled_data = ((data - min_val) / (max_val - min_val)
                             * (interval[1] - interval[0]) + interval[0])
        else:
            # If all values are equal, set them to the mean of the interval
            rescaled_data = np.zeros_like(data) + (interval[1]-interval[0])/2.

        return list(rescaled_data)

    def parse_positions(g, *, model: str, **kwargs):
        """Assigns a position to each node in graph g.

        Args:
            g (networkx graph or list of nodes): The graph
            model (str): The model used for node positioning
            **kwargs: Passed to the node positioning routine

        Returns:
            dict: A dictionary of positions keyed by node
        """
        if model.startswith('graphviz_'):
            try:
                # graphviz models
                model = model[9:]
                return nx.nx_pydot.graphviz_layout(g, prog=model, **kwargs)

            except ModuleNotFoundError as err:
                raise ModuleNotFoundError("When trying to use the graphviz "
                                          "node positioning model '{}': '{}'"
                                          "".format(model, err)) from err

        # networkx models
        return POSITIONING_MODELS_NETWORKX[model](g, **kwargs)

    def parse_node_kwargs(g, *, nodelist: list, node_kwargs: dict,
                          mark: dict=None, shrink_to_zero: list=None):
        """Parses node kwargs which are then passed to draw_networkx_nodes.
        
        Args:
            g (networkx graph): The graph
            nodelist (list): List of nodes for which property mapping is done
            node_kwargs (dict): node layout configuration
            mark (dict, optional): Configuration for node highlighting
            shrink_to_zero (list, optional): List of nodes for which the node
                size is set to zero. If given, this takes precendence over the
                entry in ``node_kwargs``.
        
        Returns:
            (dict, dict, ColorManager): (parsed node configuration, parsed node
                colorbar configuration, color manager)
        """
        # Update the list of nodes to be shown
        node_kwargs['nodelist'] = nodelist

        cbar_kwargs = node_kwargs.pop('colorbar', {})

        if shrink_to_zero is None:
            shrink_to_zero = []

        # Do the property mapping
        for plt_prop, g_prop in node_kwargs.items():

            if isinstance(g_prop, dict) and g_prop.get('from_property', False):

                prop = g_prop['from_property']
                interval = g_prop.get('scale_to_interval', None)

                if plt_prop == 'node_size':

                    if prop == 'degree':
                        node_sizes = np.array([g.degree[n] for n in nodelist])

                    elif prop == 'in_degree':
                        node_sizes = np.array([g.in_degree[n]
                                               for n in nodelist])

                    elif prop == 'out_degree':
                        node_sizes = np.array([g.out_degree[n]
                                               for n in nodelist])

                    else:
                        node_sizes = np.array([g.nodes[n][prop]
                                               for n in nodelist])

                    to_shrink = np.isin(nodelist, shrink_to_zero)

                    node_sizes[to_shrink] = 0

                    node_sizes[~to_shrink] = scale_to_interval(
                                            node_sizes[~to_shrink], interval)

                    node_kwargs['node_size'] = list(node_sizes)

                elif plt_prop == 'node_color':
                    node_colors = scale_to_interval(
                            [g.nodes[n][prop] for n in nodelist], interval)

                    if 'map_to_scalar' in g_prop:
                        map_to_scalar = np.vectorize(
                                                g_prop['map_to_scalar'].get)
                        node_colors = list(map_to_scalar(node_colors))
                    
                    node_kwargs['node_color'] = node_colors

                    cbar_kwargs['enabled'] = cbar_kwargs.get('enabled', True)

                elif plt_prop == 'alpha':
                    node_kwargs['alpha'] = scale_to_interval(
                            [g.nodes[n][prop] for n in nodelist], interval)

                else:
                    raise TypeError("'{}' can not be mapped to the '{}' "
                                    "property!".format(prop, plt_prop))

        # If no property mapping done for the node size, set it to zero for
        # nodes to be shrinked and set it to the default value (=300) else.
        if (shrink_to_zero
            and not isinstance(node_kwargs.get('node_size', None), list)):

            node_kwargs['node_size'] = [0 if n in shrink_to_zero else 300
                                        for n in nodelist]

        # Set up ColorManager
        vmin = node_kwargs.get('vmin', None)
        vmax = node_kwargs.get('vmax', None)
        cmap = node_kwargs.get('cmap', 'viridis')
        cmap_norm = node_kwargs.pop('cmap_norm', 'Normalize')
        cbar_labels = cbar_kwargs.pop('labels', None)
        
        colormanager = ColorManager(cmap=cmap, norm=cmap_norm,
                                    labels=cbar_labels, vmin=vmin, vmax=vmax)

        node_kwargs['cmap'] = colormanager.cmap

        # Prepare the node highlighting
        if mark:
            # First, create dict of colors keyed by node from 'node_color'
            node_color = node_kwargs.get('node_color', '#1f78b4')

            if not mpl.colors.is_color_like(node_color):
                # If 'node_color' contains numeric values, they need to be
                # transformed via the specified colormap.
                if not mpl.colors.is_color_like(node_color[0]):
                    node_color = colormanager.map_to_color(node_color)

                colors = {n: node_color[i] for i, n in enumerate(nodelist)}

            else:
                colors = {n: node_color for n in nodelist}

            # Update the color dict with the values from the mark configuration
            if 'nodelist' in mark:
                for n in mark['nodelist']:
                    colors[n] = mark['color']

            else:
                for n in mark['colors']:
                    colors[n] = mark['colors'][n]

            # The color values are aligned with 'nodelist' since dicts don't
            # change their ordering.
            node_kwargs['edgecolors'] = list(colors.values())

        return node_kwargs, cbar_kwargs, colormanager

    def parse_edge_kwargs(g, *, edgelist: list, edge_kwargs: dict,
                          mark: dict=None):
        """Parses edge kwargs which are then passed to draw_networkx_edges.

        Args:
            g (networkx graph): The graph
            edgelist (list): List of edges for which to do the property mapping
            edge_kwargs (dict): edge layout configuration
            mark (dict, optional): Configuration for edge highlighting

        Returns:
            (dict, dict, ColorManager): (parsed edge configuration, parsed edge
                colorbar configuration, color manager)
        """
        # Update the list of edges to be shown
        edge_kwargs['edgelist'] = edgelist

        cbar_kwargs = edge_kwargs.pop('colorbar', {})

        # Do the property mapping
        for plt_prop, g_prop in edge_kwargs.items():
            if isinstance(g_prop, dict) and g_prop.get('from_property', False):
                prop = g_prop['from_property']
                interval = g_prop.get('scale_to_interval', None)

                if plt_prop == 'edge_color':
                    edge_colors = scale_to_interval(
                            [g.edges[e][prop] for e in edgelist], interval)

                    if 'map_to_scalar' in g_prop:
                        map_to_scalar = np.vectorize(
                                                g_prop['map_to_scalar'].get)
                        edge_colors = list(map_to_scalar(edge_colors))

                    edge_kwargs['edge_color'] = edge_colors
                    
                    cbar_kwargs['enabled'] = cbar_kwargs.get('enabled', True)

                elif plt_prop == 'width':
                    edge_kwargs['width'] = scale_to_interval(
                            [g.edges[e][prop] for e in edgelist], interval)

                else:
                    raise TypeError("'{}' can not be mapped to the '{}' "
                                    "property!".format(prop, plt_prop))

        # Set up ColorManager
        vmin = edge_kwargs.get('edge_vmin', None)
        vmax = edge_kwargs.get('edge_vmax', None)
        cmap = edge_kwargs.get('edge_cmap', 'viridis')
        cmap_norm = edge_kwargs.pop('cmap_norm', 'Normalize')
        cbar_labels = cbar_kwargs.pop('labels', None)
        
        colormanager = ColorManager(cmap=cmap, norm=cmap_norm,
                                    labels=cbar_labels, vmin=vmin, vmax=vmax)

        edge_kwargs['edge_cmap'] = colormanager.cmap

        if isinstance(g, nx.DiGraph) and edge_kwargs.get('arrows', True):
            if type(colormanager.norm) != mpl.colors.Normalize:
                # NOTE In `draw_networkx_edges`, the `Normalize` norm is
                #      applied explicitly. Since the norm can't be updated
                #      later (as edges with arrows are `FancyArrowPatch`es),
                #      other norms than `Normalize` are forbidden here.
                raise TypeError("Received invalid norm type: "
                                f"{type(colormanager.norm)}. For directed "
                                "edges with `arrows = True`, only the "
                                "matplotlib.colors.Normalize base class is "
                                "supported.")

        # Prepare the edge highlighting
        if mark:
            # First, create dict of colors keyed by edge from 'edge_color'
            edge_color = edge_kwargs.get('edge_color', 'k')

            if not mpl.colors.is_color_like(edge_color):
                # Transform to color-like if needed
                if not mpl.colors.is_color_like(edge_color[0]):
                    edge_color = colormanager.map_to_color(edge_color)
                
                colors = {e[:2]: edge_color[i] for i, e in enumerate(edgelist)}

            else:
                colors = {e[:2]: edge_color for e in edgelist}

            # Update the color dict with the values from the mark configuration
            if 'edgelist' in mark:
                for e in mark['edgelist']:
                    e = tuple(e)
                    if not isinstance(g, nx.DiGraph) and e not in colors:
                        e = e[::-1]
                    colors[e] = mark['color']

            else:
                for e in mark['colors']:
                    if not isinstance(g, nx.DiGraph) and e not in colors:
                        e = e[::-1]
                    colors[e] = mark['colors'][e]

            edge_kwargs['edge_color'] = list(colors.values())

        return edge_kwargs, cbar_kwargs, colormanager

    def parse_node_label_kwargs(g, *, nodelist: list, label_kwargs: dict,
                                shrink_to_zero: list=None):
        """Parses node label kwargs which are then passed to
        draw_networkx_lables.
        
        Args:
            g (networkx graph or list of nodes): The graph
            nodelist (list): List of nodes that will be drawn
            label_kwargs (dict): label layout configuration
            shrink_to_zero (list, optional): List of nodes for which to hide
                the node label. If given, this takes precendence over the
                entry in ``label_kwargs``.
        
        Returns:
            (dict, bool): (parsed label_kwargs, whether to show labels)
        """
        # Labels are disabled by default.
        if not label_kwargs.pop('enabled', False):
            return {}, False

        if shrink_to_zero is None:
            shrink_to_zero = []

        show_only_given = 'show_only' in label_kwargs
        show_only = label_kwargs.pop('show_only', nodelist)
        decode = label_kwargs.pop('decode', None)

        if label_kwargs.get('labels', False):
            if label_kwargs['labels'].get('from_property', False):
                prop = label_kwargs['labels']['from_property']
                label_template = label_kwargs['labels'].get('format',"{label}")
                label_kwargs['labels'] = {
                        n: label_template.format(
                                label=(g.nodes[n][prop] if decode is None
                                       else g.nodes[n][prop].decode(decode))
                            )
                        for n in nodelist
                        if n in show_only and n not in shrink_to_zero
                }

            else:
                # show_only takes precedence over the provided node labels
                if show_only_given:
                    # If some labels are given, keep only those that are in
                    # show_only. nodes in show_only for which no label was
                    # given are labeled with their index.
                    for n in list(label_kwargs['labels'].keys()):
                        if n not in show_only or n in shrink_to_zero:
                            del label_kwargs['labels'][n]

                    for n in show_only:
                        if (n not in label_kwargs['labels'].keys()
                            and n not in shrink_to_zero
                        ):
                            label_kwargs['labels'][n] = n

        else:
            # If enabled but no labels given, label nodes with their index.
            label_kwargs['labels'] = {n:n for n in show_only if n in nodelist
                                      and n not in shrink_to_zero}

        return label_kwargs, True

    def parse_edge_label_kwargs(g, *, edgelist: list, label_kwargs: dict):
        """Parses edge label kwargs which are then passed to
        draw_networkx_edge_lables.

        Args:
            g (networkx graph or list of nodes): The graph
            edgelist (list): List of edges that will be drawn
            label_kwargs (dict): label layout configuration

        Returns:
            (dict, bool): (parsed label_kwargs, whether to show labels)
        """
        # Labels are disabled by default.
        if not label_kwargs.pop('enabled', False):
            return {}, False

        # Catch a dangerous pitfall: There is no 'labels' argument for the edge
        # labels (as there is for the node labels), here it is named
        # 'edge_labels'.
        if label_kwargs.get('labels', False):
            raise ValueError("Received 'labels' key in edge label "
                             "configuration. This is not a valid kwarg! "
                             "For specifying an edge label dict, use the key "
                             "'edge_labels'.")

        show_only_given = 'show_only' in label_kwargs
        show_only = label_kwargs.pop('show_only', edgelist)
        decode = label_kwargs.pop('decode', None)

        # Convert edges to tuples
        show_only = [tuple(e) for e in show_only]

        if label_kwargs.get('edge_labels', False):
            if label_kwargs['edge_labels'].get('from_property', False):
                prop = label_kwargs['edge_labels']['from_property']
                label_template = label_kwargs['edge_labels'].get('format',
                                                                 "{label}")
                label_kwargs['edge_labels'] = {
                        e[:2]: label_template.format(
                                label=(g.edges[e][prop] if decode is None
                                       else g.edges[e][prop].decode(decode))
                            )
                        for e in edgelist
                        if e in show_only or e[:2] in show_only
                }

            else:
                # show_only takes precedence over the provided edge_labels
                if show_only_given:
                    # If some labels are given, keep only those that are in
                    # show_only. edges in show_only for which no label was
                    # given are labeled with their (source, destination) pair.
                    for e in list(label_kwargs['edge_labels'].keys()):
                        # Delete edge label entries that are not in show_only
                        if (all([e[:2]!=edge[:2] for edge in edgelist])
                            or all([e[:2]!=edge[:2] for edge in show_only])
                        ):
                            del label_kwargs['edge_labels'][e]

                    for e in show_only:
                        if (any([e[:2]==edge[:2] for edge in edgelist])
                            and e[:2] not in label_kwargs['edge_labels'].keys()
                        ):
                            label_kwargs['edge_labels'][e[:2]] = e[:2]

        else:
            # If enabled but no labels given, label edges with the respective
            # (source, destination) pairs.
            label_kwargs['edge_labels'] = {e[:2]: e[:2] for e in edgelist
                                           if e in show_only
                                           or e[:2] in show_only}

        return label_kwargs, True

    def prepare_and_plot(graph_group, *,
                         graph_creation_cfg: dict,
                         graph_drawing_cfg: dict,
                         positions: dict=None,
                         suppress_cbars=False):
        """Plots graph with given configuration.
        
        Args:
            graph_group (GraphGroup): The GraphGroup from which the networkx
                graph is created
            graph_creation_cfg (dict): The configuration of the graph creation
            graph_drawing_cfg (dict): The configuration of the graph layout
            positions (dict, optional): dict assigning a position to each node
            suppress_cbars (bool, optional): Whether to suppress colorbars
        
        Returns:
            (dict): Dict containing node positions, PatchCollections of drawn
                nodes, edges, and labels, and drawn colorbars.
        
        Raises:
            Warning: On enabled colorbar for directed edges.
        """
        # Work on copies such that the original configuration is not modified
        graph_creation_cfg = copy.deepcopy(graph_creation_cfg)
        graph_drawing_cfg = copy.deepcopy(graph_drawing_cfg)

        # Create the graph from the GraphGroup.
        g = graph_group.create_graph(**graph_creation_cfg)

        # Get the sub-configurations for the drawing of the graph
        select = graph_drawing_cfg.get('select', {})
        pos_kwargs = graph_drawing_cfg.get('positions', dict(model='spring'))
        node_kwargs = graph_drawing_cfg.get('nodes', {})
        edge_kwargs = graph_drawing_cfg.get('edges', {})
        node_label_kwargs = graph_drawing_cfg.get('node_labels', {})
        edge_label_kwargs = graph_drawing_cfg.get('edge_labels', {})
        mark_nodes_kwargs = graph_drawing_cfg.get('mark_nodes', {})
        mark_edges_kwargs = graph_drawing_cfg.get('mark_edges', {})

        # Whether to remove the non-selected nodes and edges from the graph
        drop = select.pop('drop', True)

        # Select the nodes and edges to be shown
        if 'nodelist' in select:
            # Selection from a list of nodes
            (nodes_to_plot,
             edges_to_plot,
             nodes_to_shrink) = select_from_list(g, **select)

        elif select:
            # Radial selection
            (nodes_to_plot,
             edges_to_plot,
             nodes_to_shrink) = select_radial(g, **select)

        else:
            # If no selection was specified, select all nodes and edges in g
            (nodes_to_plot,
             edges_to_plot,
             nodes_to_shrink) = list(g.nodes), list(g.edges), None

        if drop:
            # Remove the nodes that are not selected. This automatically
            # removes all edges for which the source or destination was removed.
            nodes_to_remove = g.nodes - set(nodes_to_plot)
            g.remove_nodes_from(nodes_to_remove)
        
        # Do the node positioning
        pos = positions if positions else parse_positions(g, **pos_kwargs)

        # Now, parse all configuration dictionaries, e.g., do property mapping.
        node_kwargs, node_cbar_kwargs, node_colormanager = parse_node_kwargs(g,
                                                nodelist=nodes_to_plot,
                                                node_kwargs=node_kwargs,
                                                mark=mark_nodes_kwargs,
                                                shrink_to_zero=nodes_to_shrink)
        edge_kwargs, edge_cbar_kwargs, edge_colormanager = parse_edge_kwargs(g,
                                                edgelist=edges_to_plot,
                                                edge_kwargs=edge_kwargs,
                                                mark=mark_edges_kwargs)
        node_label_kwargs, show_node_labels = parse_node_label_kwargs(g,
                                                nodelist=nodes_to_plot,
                                                label_kwargs=node_label_kwargs,
                                                shrink_to_zero=nodes_to_shrink)
        edge_label_kwargs, show_edge_labels = parse_edge_label_kwargs(g,
                                                edgelist=edges_to_plot,
                                                label_kwargs=edge_label_kwargs)

        # Make sure that, in the case of a directed graph, the arrows end
        # exactly at the node boundaries.
        # NOTE This also means that edges can only be drawn if both their
        #      source and destination are drawn.
        if 'node_size' in node_kwargs:
            edge_kwargs['nodelist'] = nodes_to_plot
            edge_kwargs['node_size'] = node_kwargs['node_size']

        # Call the networkx plot functions with the parsed configurations
        nodes = nx.draw_networkx_nodes(g, pos=pos, ax=hlpr.ax, **node_kwargs)
        edges = nx.draw_networkx_edges(g, pos=pos, ax=hlpr.ax, **edge_kwargs)

        # NOTE networkx does not pass on the norms to the respective matplotlib
        #      functions. Hence, they need to be set manually. For the edges,
        #      the cmap also needs to be set manually. Can only be set for the
        #      edges if graph is undirected or `arrows=False`. 
        nodes.set_norm(node_colormanager.norm)

        if not isinstance(edges, list):
            edges.set_norm(edge_colormanager.norm)
            edges.set_cmap(edge_colormanager.cmap)

        node_labels = None
        edge_labels = None

        if show_node_labels:
            node_labels = nx.draw_networkx_labels(g, pos=pos, ax=hlpr.ax,
                                                        **node_label_kwargs)
        if show_edge_labels:
            edge_labels = nx.draw_networkx_edge_labels(g, pos=pos, ax=hlpr.ax,
                                                        **edge_label_kwargs)

        # Add colorbars
        cb_n = None
        cb_e = None

        if not suppress_cbars:
            show_node_cbar = node_cbar_kwargs.pop('enabled', False)
            show_edge_cbar = edge_cbar_kwargs.pop('enabled', False)

            if show_node_cbar:
                cb_n = node_colormanager.create_cbar(nodes, fig=hlpr.fig,
                                                     ax=hlpr.ax,
                                                     **node_cbar_kwargs)

            if show_edge_cbar:
                # When drawing arrows, draw_networkx_edges returns a list of
                # FancyArrowPatches which can not be used in fig.colorbar.
                if isinstance(edges, list):
                    warnings.warn("No colorbar can be shown for directed "
                                  "edges! To show the colorbar, hide the "
                                  "arrows by setting 'arrows=False' in the "
                                  "edge configuration.", UserWarning)
                else:
                    cb_e = edge_colormanager.create_cbar(edges, fig=hlpr.fig,
                                                         ax=hlpr.ax,
                                                         **edge_cbar_kwargs)

        return {'pos': pos,
                'nodes': nodes,
                'edges': edges,
                'node_labels': node_labels,
                'edge_labels': edge_labels,
                'cb_n': cb_n,
                'cb_e': cb_e}

    # .. Actual plotting routine starts here ..................................
    # Get the GraphGroup
    graph_group = data['graph_group']

    # Register external property data
    if register_property_maps:
        # Clear existing property maps in order to not have side effects if
        # plotting multiple times, e.g. in interactive mode. This is important
        # because the graph_group most probably is a reference.
        if clear_existing_property_maps:
            graph_group.property_maps.clear()

        # Can register now
        for tag in register_property_maps:
            try:
                pmap = data[tag]
            except KeyError as err:
                _available_tags = ', '.join(data.keys())
                raise ValueError(
                    f"No tag '{tag}' found in the data selected by the DAG! "
                    "Make sure the tag is named correctly and is selected to "
                    "be computed; adjust the 'compute_only' argument if "
                    "needed.\nThe following tags are available in the DAG "
                    f"results:  {_available_tags}"
                ) from err

            graph_group.register_property_map(tag, pmap)
            log.remark("Registered tag '%s' as property map of %s.",
                       tag, graph_group.logstr)

    if graph_creation is None:
        graph_creation = {}

    if graph_drawing is None:
        graph_drawing = {}

    # Perform single graph plot
    rv = prepare_and_plot(graph_group,
                          graph_creation_cfg=graph_creation,
                          graph_drawing_cfg=graph_drawing)

    # Hide the axes
    hlpr.ax.axis('off')

    # Prepare parameters and kwargs for the update routine
    suptitle_kwargs = suptitle_kwargs if suptitle_kwargs else {}
    positions = rv['pos']

    def update():
        """Animation generator for the draw_graph function.

        When the animation frames are given by different points in time, they
        can be specified by value (via ``sel``), by index (via ``isel``), or 
        ``from_property``. In the latter case, the time values are extracted
        from the ``time`` coordinates of the specified property data.

        The animation uses fixed node positions as the positioning models would
        arange the nodes very differently in each iteration, even for only
        small changes in the graph structure.

        The node positions and the colorbar(s) are obtained from the basic
        (single) plot configuration and are then fixed.
        """
        def parse_time_kwargs(kwargs):
            _TIME_KWARGS = ('from_property', 'sel', 'isel')

            # Check for unexpected entries
            if any([k not in _TIME_KWARGS for k in kwargs.keys()]):
                raise TypeError("Received invalid specifications for the "
                                "animation times: {}. Allowed entries: {}"
                                "".format(", ".join([k for k in kwargs
                                                     if k not in _TIME_KWARGS]),
                                          ", ".join(_TIME_KWARGS)))

            elif len(kwargs) > 1:
                raise TypeError("Received ambiguous time specifications. "
                                "Need _one_ of: {}"
                                "".format(", ".join(_TIME_KWARGS)))

            times = None
            time_idxs = None

            # Times can be extracted from any container stored in the graph
            # group or from any registered external data.
            if 'from_property' in kwargs:
                times = list(
                    graph_group._get_item_or_pmap(kwargs['from_property'])
                    .coords['time'].values
                )

            elif 'sel' in kwargs:
                times = kwargs['sel']

            elif 'isel' in kwargs:
                time_idxs = kwargs['isel']

            else:
                raise TypeError("Missing time specifications. Need _one_ of: {}"
                                "".format(", ".join(_TIME_KWARGS)))

            return times, time_idxs

        # Clear the axis. Colorbars are *not* removed.
        hlpr.ax.clear()
        hlpr.ax.axis('off')

        time_kwargs = graph_animation['times']

        times, time_idxs = parse_time_kwargs(time_kwargs)

        # Prepare graph creation config dict
        graph_creation_anim = copy.deepcopy(graph_creation)
        graph_creation_anim.pop('at_time_idx', None)
        graph_creation_anim.pop('at_time', None)

        # Prepare iterator and other time-dependent settings
        if times:
            time_iter = times
            time_key = 'at_time'
        else:
            time_iter = time_idxs
            time_key = 'at_time_idx'

        # Prepare the suptitle format string
        if 'title' not in suptitle_kwargs:
            if times:
                suptitle_kwargs['title'] = "time = {value:d}"
            else:
                suptitle_kwargs['title'] = "time idx = {value:d}"

        # Iterate over the selected times (can be time value _or_ index)
        for time in time_iter:
            graph_creation_anim[time_key] = time

            # Perform plot for the current time. Colorbars are suppressed and
            # the positions of the basic draw_graph function call are used.
            rv = prepare_and_plot(graph_group,
                                  graph_creation_cfg=graph_creation_anim,
                                  graph_drawing_cfg=graph_drawing,
                                  positions=positions,
                                  suppress_cbars=True)

            # Apply the suptitle format string, then invoke the helper
            st_kwargs = copy.deepcopy(suptitle_kwargs)
            st_kwargs['title'] = st_kwargs['title'].format(value=(time))
            hlpr.invoke_helper('set_suptitle', **st_kwargs)

            # Let the writer grab the current frame
            yield

            # Remove nodes and edges again
            rv['nodes'].remove()

            if isinstance(rv['edges'], list):
                # for a list of FancyArrowPatches
                for i in range(len(rv['edges'])):
                    rv['edges'][i].remove()
            else:
                # for a LineCollection
                rv['edges'].remove()

            # Remove labels
            if rv['node_labels']:
                for label in rv['node_labels'].values():
                    label.remove()

            if rv['edge_labels']:
                for label in rv['edge_labels'].values():
                    label.remove()

    hlpr.register_animation_update(update)
