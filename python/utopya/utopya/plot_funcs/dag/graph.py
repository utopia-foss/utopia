"""This module provides a generic function for graph plotting."""
import os
import copy
import logging
from typing import Sequence

import numpy as np
import matplotlib.pyplot as plt
import networkx as nx

from utopya.plotting import is_plot_func, PlotHelper

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
               graph_cfg: dict,
               register_property_maps: Sequence[str]=None,
               clear_existing_property_maps: bool=True,
               suptitle_kwargs: dict=None):
    """Draws a graph from a ``GraphGroup``.

    Data selection happens via the dantro data transformation framework. The
    graph group that is to be plotted needs to be selected via the DAG and
    tagged ``graph_group``. Additional property maps can also be made available
    for plotting, see ``register_property_map``argument.

    For more information on how to use the transformation framework, refer to
    the `dantro documentation <https://dantro.readthedocs.io/en/stable/plotting/plot_data_selection.html>`_.

    Args:
        hlpr (PlotHelper): The PlotHelper instance for this plot
        data (dict): Data from TransformationDAG selection
        graph_cfg (dict): Configuration for graph creation, layout and
            animation
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
    def select_from_list(g, *, nodes: list):
        """Given a list of selected nodes, selects all edges for which both
        ends are in the set of selected nodes.

        Args:
            g (nx.Graph): The graph
            nodes (list): The nodes to be selected

        Returns:
            Tuple containing a list of selected nodes and a list of selected
            edges.
        """
        subgraph = nx.induced_subgraph(g, nodes)

        return list(subgraph.nodes), list(subgraph.edges)

    def select_radial(g, *, center: int, radius: int):
        """Selects all nodes around a given center within a given radius
        (measured in numbers of neighborhoods). Selects all edges for which
        both ends are in the set of selected nodes.

        Args:
            g (nx.Graph): The graph
            center (int): index of central node
            radius (int): selection radius

        Returns:
            Tuple containing a list of selected nodes and a list of selected
            edges.
        """
        if radius > g.number_of_nodes()-1:
            radius = g.number_of_nodes()-1

        # Identify the nodes within the given radius around the central node.
        # Start by adding the central nodes and all of its neighbors to a set.
        # Then, iteratively add all neighbors of the previously added nodes to
        # the set, until the given radius is reached.
        # FIXME It might be worth testing the computational efficiency of this
        #       (also for large subgraphs) as each node is tried to be added to
        #       the set at least two times.
        node_selection = set([center])
        nbs_prev = set([center])
        nbs_new = set()

        for i in range(radius):
            for n in nbs_prev:
                nbs_new.update(nx.all_neighbors(g, n))

            node_selection = node_selection.union(nbs_new)
            nbs_prev = nbs_new.copy()
            nbs_new.clear()

        # Create a (read-only) GraphView of the subgraph induced by the
        # selected nodes, i.e., a graph containing the selected nodes N and
        # all edges that have both ends in N.
        subgraph = nx.induced_subgraph(g, node_selection)

        return list(subgraph.nodes), list(subgraph.edges)

    def scale_to_interval(data: list, interval: list=None):
        """Rescales the data linearly to the given interval. If not interval is
        given the data is returned as it is.

        Args:
            data (list): data that is rescaled linearly to the given interval
            interval (list, optional): The target interval

        Returns:
            list: rescaled data
        """
        if not interval:
            return data

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

    def parse_node_kwargs(g, *, nodelist: list, node_kwargs: dict):
        """Parses node kwargs which are then passed to draw_networkx_nodes.

        Args:
            g (networkx graph): The graph
            nodelist (list): List of nodes for which to do the property mapping
            node_kwargs (dict): node layout configuration

        Returns:
            (dict, bool): (parsed node_kwargs, whether to add a colorbar)
        """
        # Update the list of nodes to be shown
        node_kwargs['nodelist'] = nodelist

        show_cbar = False
        for plt_prop, g_prop in node_kwargs.items():

            if isinstance(g_prop, dict) and g_prop.get('from_property', False):

                prop = g_prop['from_property']
                interval = g_prop.get('scale_to_interval', None)

                if plt_prop == 'node_size' and prop == 'degree':
                    node_kwargs['node_size'] = scale_to_interval(
                            [g.degree[n] for n in nodelist], interval)

                elif plt_prop == 'node_size' and prop == 'in_degree':
                    node_kwargs['node_size'] = scale_to_interval(
                            [g.in_degree[n] for n in nodelist], interval)

                elif plt_prop == 'node_size' and prop == 'out_degree':
                    node_kwargs['node_size'] = scale_to_interval(
                            [g.out_degree[n] for n in nodelist], interval)

                elif plt_prop == 'node_size':
                    node_kwargs['node_size'] = scale_to_interval(
                            [g.nodes[n][prop] for n in nodelist], interval)

                elif plt_prop == 'node_color':
                    node_kwargs['node_color'] = scale_to_interval(
                            [g.nodes[n][prop] for n in nodelist], interval)
                    show_cbar = node_kwargs.pop('colorbar', True)  # FIXME

                elif plt_prop == 'alpha':
                    node_kwargs['alpha'] = scale_to_interval(
                            [g.nodes[n][prop] for n in nodelist], interval)

                else:
                    raise TypeError("'{}' can not be mapped to the '{}' "
                                    "property!".format(prop, plt_prop))

        return node_kwargs, show_cbar

    def parse_edge_kwargs(g, *, edgelist: list, edge_kwargs: dict):
        """Parses edge kwargs which are then passed to draw_networkx_edges.

        Args:
            g (networkx graph): The graph
            edgelist (list): List of edges for which to do the property mapping
            edge_kwargs (dict): edge layout configuration

        Returns:
            (dict, bool): (parsed edge_kwargs, whether to add a colorbar)
        """
        # Update the list of edges to be shown
        edge_kwargs['edgelist'] = edgelist

        show_cbar = False
        for plt_prop, g_prop in edge_kwargs.items():
            if isinstance(g_prop, dict) and g_prop.get('from_property', False):
                prop = g_prop['from_property']
                interval = g_prop.get('scale_to_interval', None)

                if plt_prop == 'edge_color':
                    edge_kwargs['edge_color'] = scale_to_interval(
                            [g.edges[e][prop] for e in edgelist], interval)
                    show_cbar = edge_kwargs.pop('colorbar', True)

                elif plt_prop == 'width':
                    edge_kwargs['width'] = scale_to_interval(
                            [g.edges[e][prop] for e in edgelist], interval)

                else:
                    raise TypeError("'{}' can not be mapped to the '{}' "
                                    "property!".format(prop, plt_prop))

        return edge_kwargs, show_cbar

    def parse_node_label_kwargs(g, *, nodelist: list, label_kwargs: dict):
        """Parses node label kwargs which are then passed to
        draw_networkx_lables.

        Args:
            g (networkx graph or list of nodes): The graph
            nodelist (list): List of nodes that will be drawn
            label_kwargs (dict): label layout configuration

        Returns:
            (dict, bool): (parsed label_kwargs, whether to show labels)
        """
        # Labels are disabled by default.
        if not label_kwargs.pop('enabled', False):
            return {}, False

        show_only_given = True if 'show_only' in label_kwargs else False
        show_only = label_kwargs.pop('show_only', nodelist)

        if label_kwargs.get('labels', False):
            if label_kwargs['labels'].get('from_property', False):
                prop = label_kwargs['labels']['from_property']
                label_kwargs['labels'] = {idx:l for idx,l in zip(show_only,
                        ["{:.1f}".format(g.nodes[n][prop]) for n in show_only
                         if n in nodelist])}

            else:
                # show_only takes precedence over the provided node labels
                if show_only_given:
                    # If some labels are given, keep only those that are in
                    # show_only. nodes in show_only for which no label was
                    # given are labeled with their index.
                    for n in list(label_kwargs['labels'].keys()):
                        if n not in show_only:
                            del label_kwargs['labels'][n]

                    for n in show_only:
                        if n not in label_kwargs['labels'].keys():
                            label_kwargs['labels'][n] = str(n)

        else:
            # If enabled but no labels given, label nodes with their index.
            label_kwargs['labels'] = {idx:l for idx,l in zip(nodelist,
                                        ["{}".format(n) for n in show_only
                                         if n in nodelist])}

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
        def make_tuple_conversions(cont):
            try:
                if isinstance(cont, list):
                    new_cont = [tuple(el) for el in cont]
                elif isinstance(cont, dict):
                    new_cont = {tuple(k):v for k,v in cont.items()}
                else:
                    raise TypeError("Retreived object of type '{}' in "
                                    "'make_tuple_conversions'. Expected one "
                                    "of: list, dict".format(type(cont)))

            except TypeError as err:
                raise TypeError("An error occurred when trying to convert "
                                "edge(s) to tuples: ", err) from err

            return new_cont

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

        show_only_given = True if 'show_only' in label_kwargs else False
        show_only = label_kwargs.pop('show_only', edgelist)

        # Convert edges to tuples
        show_only = make_tuple_conversions(show_only)

        # Now, keep only edges that are in the edgelist
        for e in show_only:
            if all([(e!=edge and e!=edge[:2]) for edge in edgelist]):
                show_only.remove(e)

        if label_kwargs.get('edge_labels', False):
            if label_kwargs['edge_labels'].get('from_property', False):

                # First check for the correct edge tuple length in show_only.
                # If the graph allows parallel edges, they must be 3-tuples.
                if (isinstance(g, nx.MultiGraph)
                    or isinstance(g, nx.MultiDiGraph)):
                    edge_tuple_length = 3
                else:
                    edge_tuple_length = 2

                if any([len(e)!=edge_tuple_length for e in show_only]):
                    raise TypeError("Received invalid edge(s) in 'show_only' "
                                    "for graph of type {}: {}. Expected "
                                    "tuples of size {}."
                                    "".format(type(g),
                                              ", ".join([str(e) for e in edges
                                                if len(e)!=edge_tuple_length]),
                                              edge_tuple_length))

                prop = label_kwargs['edge_labels']['from_property']
                label_kwargs['edge_labels'] = {e[:2]:l for e,l in zip(show_only,
                        ["{:.1f}".format(g.edges[e][prop]) for e in show_only])}

            else:
                # First, convert edges to tuples and check for invalid entries
                label_kwargs['edge_labels'] = make_tuple_conversions(
                                                label_kwargs['edge_labels'])

                if any([len(e)!=2 for e in
                        list(label_kwargs['edge_labels'].keys())]):
                    raise TypeError("Received invalid edge(s) in 'edge_labels':"
                                    " {}. Expected tuples of size 2."
                                    "".format(", ".join([str(e) for e in edges
                                                         if len(e)!=2])))

                # show_only takes precedence over the provided edge_labels
                if show_only_given:
                    # If some labels are given, keep only those that are in
                    # show_only. edges in show_only for which no label was
                    # given are labeled with their (source, destination) pair.
                    for e in list(label_kwargs['edge_labels'].keys()):
                        # Delete edge label entries that are not in show_only
                        if all(
                            [(e!=edge and e!=edge[:2]) for edge in show_only]
                        ):
                            del label_kwargs['edge_labels'][e]

                    for e in show_only:
                        if e[:2] not in label_kwargs['edge_labels'].keys():
                            label_kwargs['edge_labels'][e[:2]] = str(e[:2])

        else:
            # If enabled but no labels given, label edges with the respective
            # (source, destination) pairs.
            label_kwargs['edge_labels'] = {e[:2]:l for e,l in zip(edgelist,
                                    ["{}".format(e[:2]) for e in show_only])}

        return label_kwargs, True

    def prepare_and_plot(graph_group, *,
                         graph_cfg: dict,
                         positions: dict=None,
                         suppress_cbars=False):
        """Plots graph with given configuration.

        Args:
            graph_group (GraphGroup): The GraphGroup from which the networkx
                graph is created
            graph_cfg (dict): The configuration of graph creation and layout
            positions (dict, optional): dict assigning a position to each node
            suppress_cbars (bool, optional): Whether to suppress colorbars

        Returns:
            node positions, PatchCollections of drawn nodes and edges,
            colorbars
        """
        # Work on a copy such that the original configuration is not modified
        cfg = copy.deepcopy(graph_cfg)

        # Create the graph from the GraphGroup. The Graph object is created
        # once and not changed in the following.
        graph_creation_kwargs = cfg.get('graph_creation', {})
        g = graph_group.create_graph(**graph_creation_kwargs)

        # Get the configurations for the drawing of the graph
        graph_drawing_cfg = cfg.get('graph_drawing', {})
        pos_kwargs = graph_drawing_cfg.get('positions', dict(model='spring'))
        select = graph_drawing_cfg.get('select', {})
        node_kwargs = graph_drawing_cfg.get('nodes', {})
        edge_kwargs = graph_drawing_cfg.get('edges', {})
        node_label_kwargs = graph_drawing_cfg.get('node_labels', {})
        edge_label_kwargs = graph_drawing_cfg.get('edge_labels', {})

        # Do the node positioning
        pos = positions if positions else parse_positions(g, **pos_kwargs)

        # Select the nodes and edges to be shown
        if select.pop('radial', False):
            # Radial selection
            nodes_to_plot, edges_to_plot = select_radial(g, **select)

        elif select:
            # Selection from a list of nodes
            nodes_to_plot, edges_to_plot = select_from_list(g, **select)

        else:
            # If no selection was specified, select all nodes and edges in g
            nodes_to_plot, edges_to_plot = list(g.nodes), list(g.edges)

        # Now, parse all configuration dictionaries, e.g., do property mapping.
        node_kwargs, show_node_cbar = parse_node_kwargs(g,
                                                nodelist=nodes_to_plot,
                                                node_kwargs=node_kwargs)
        edge_kwargs, show_edge_cbar = parse_edge_kwargs(g,
                                                edgelist=edges_to_plot,
                                                edge_kwargs=edge_kwargs)
        node_label_kwargs, show_node_labels = parse_node_label_kwargs(g,
                                                nodelist=nodes_to_plot,
                                                label_kwargs=node_label_kwargs)
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
            if show_node_cbar:
                cb_n = hlpr.fig.colorbar(nodes, ax=hlpr.ax)

            if show_edge_cbar:
                # When drawing arrows, draw_networkx_edges returns a list of
                # FancyArrowPatches which can not be used in fig.colorbar.
                if isinstance(edges, list):
                    raise Warning("No colorbar can be shown for directed "
                                  "edges! To show the colorbar, hide the "
                                  "arrows by setting 'arrows=False' in the "
                                  "edge configuration.")
                else:
                    cb_e = hlpr.fig.colorbar(edges, ax=hlpr.ax)

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

    # Perform single graph plot
    rv = prepare_and_plot(graph_group, graph_cfg=graph_cfg)

    # Hide the axes
    hlpr.ax.axis('off')

    # Prepare parameters and kwargs for the update routine
    suptitle_kwargs = suptitle_kwargs if suptitle_kwargs else {}
    positions = rv['pos']

    def update():
        """Animation generator for the draw_graph function.

        When the animation frames are given by different points in time, they
        can be specified 'by_value', 'by_idx', or 'from_property'. In the case
        of the latter, the time values are extracted from the coordinatesp of
        the property data.

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

            if 'from_property' in kwargs:
                times = list(graph_group[kwargs['from_property']]
                             .coords['time'].values)

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

        graph_animation = graph_cfg['graph_animation']
        time_kwargs = graph_animation['times']

        times, time_idxs = parse_time_kwargs(time_kwargs)

        # Prepare graph parameter dict
        cfg_anim = copy.deepcopy(graph_cfg)
        cfg_anim['graph_creation'].pop('at_time_idx', None)
        cfg_anim['graph_creation'].pop('at_time', None)

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
            cfg_anim = copy.deepcopy(cfg_anim)
            cfg_anim['graph_creation'][time_key] = time

            # Perform plot for the current time. Colorbars are suppressed and
            # the positions of the basic draw_graph function call are used.
            rv = prepare_and_plot(graph_group,
                                  graph_cfg=cfg_anim,
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
