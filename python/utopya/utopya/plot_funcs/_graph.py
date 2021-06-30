"""This module provides the GraphPlot class."""

import os
import copy
import logging
import warnings
from typing import Sequence, Union, Callable, Dict, Tuple, Any

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import networkx as nx

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

class GraphPlot:
    """The ``GraphPlot`` class provides an interface for visualizing a 
    ``networkx.Graph`` object or a graph created from a
    :py:class:`~utopya.datagroup.GraphGroup` via matplotlib.
    """
    def __init__(
        self,
        g: nx.Graph,
        *,
        fig=None,
        ax=None,
        select: dict=None,
        positions: dict=None,
        nodes: dict=None,
        edges: dict=None,
        node_labels: dict=None,
        edge_labels: dict=None,
        mark_nodes: dict=None,
        mark_edges: dict=None
    ):
        """Initializes a ``GraphPlot``, which provides drawing utilities for a
        fixed graph.

        The drawing kwargs are stored and used when calling
        :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.

        A ``GraphPlot`` can also be initialized from a
        :py:class:`~utopya.datagroup.GraphGroup` via the
        :py:meth:`~utopya.plot_funcs._graph.GraphPlot.from_group` classmethod.

        If drawing multiple times from the same ``GraphPlot`` instance, be
        aware that it only keeps track of the nodes/edges/labels/colorbars that
        were last associated with it. Use
        :py:meth:`~utopya.plot_funcs._graph.GraphPlot.clear_plot` before
        re-drawing on the same axis.

        .. note::

            For some graph drawing kwargs it is possible to configure an
            automatized mapping from node/edge properties.
            This *property mapping* has the following syntax:

            .. code-block:: yaml

                some_layout_property:
                  from_property: my_node_or_edge_property
                  scale_to_interval: [low_lim, up_lim]

            The ``from_property`` specifies the node or edge property to be
            mapped from. If ``scale_to_interval`` is given, the layout property
            values are rescaled linearly the specified interval.
        
        Args:
            g (nx.Graph): The associated networkx graph
            fig (None, optional): The matplotlib figure used for drawing
            ax (None, optional): The matplotlib axis used for drawing

            select (dict, optional):
                Draw only a subgraph induced by a selection of nodes. Either
                select a list of nodes by passing the ``nodelist`` argument or
                do a radial node selection by specifying a ``center`` node and
                the ``radius``. The following arguments can be passed
                additionally:

                open_edges (bool, optional):
                    Whether to draw the edges for which only one of source and
                    destination is in the set of selected nodes. Disabled by
                    default.
                drop (bool, optional):
                    Whether to remove the non-selected nodes from the graph.
                    If False, *all* nodes are passed to the node positioning
                    model. Enabled by default.

            positions (dict, optional):
                Configuration for the node positioning. The following arguments
                are available:

                from_dict (dict, optional):
                    Node positions (2-tuples) keyed by node. If given, the
                    layouting algorithm given by the ``model`` argument will be
                    ignored.

                model (Union[str, Callable], optional):
                    The layout model that is used to calculate the node
                    positions (default=``spring``). Available
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

                    If the argument is a callable, it is invoked with the graph
                    as the first positional argument and is expected to return
                    networkx-compatible node positions, i.e. a mapping from
                    nodes to a 2-tuple denoting the position.
                further kwargs:
                    Passed on to the chosen layout model.

            nodes (dict, optional):
                Drawing configuration for the nodes. The following arguments
                are available for property mapping:
                ``node_size``, ``node_color``, ``alpha``.
                The following arguments are allowed:

                node_size (scalar or sequence of scalars, optional):
                    The node size (default=300). Available for property
                    mapping. Can be mapped directly from the nodes' ``degree``,
                    ``in_degree``, or ``out_degree`` by setting the
                    ``from_property`` argument accordingly.
                node_color (color or sequene of colors, optional):
                    Single color (string or RGB(A) tuple or numeric value)
                    or sequence of colors (default: '#1f78b4'). If numeric
                    values are specified they will be mapped to colors using
                    the cmap and vmin, vmax parameters. 

                    If mapped from property it may contain an additional
                    ``map_to_scalar``, which is a dict of numeric target values
                    keyed by property value. This allows to map from non-numeric
                    (e.g. categorical) properties.
                cmap (optional):
                    The colormap. Passed as ``cmap`` to
                    :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                cmap_norm (optional):
                    The norm used for the color mapping. Passed as ``norm`` to
                    :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                    Is overwritten, if a discrete colormap is specified in
                    ``cmap``.
                colorbar (dict, optional):
                    The node colorbar configuration. The following arguments
                    are allowed:

                    enabled (bool, optional):
                        Whether to plot a colorbar. Enabled by default if
                        ``node_color`` is mapped from property.
                    labels (dict, optional):
                        Colorbar tick-labels keyed by tick position (see
                        :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`).
                    tick_params (dict, optional):
                        Colorbar axis tick parameters
                    label (str, optional):
                        The axis label for the colorbar
                    label_kwargs (dict, optional):
                        Further keyword arguments to adjust the aesthetics of
                        the colorbar label
                    further kwargs:
                        Passed on to :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`.

                further kwargs:
                    Passed to `draw_networkx_nodes <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_nodes.html>`_
                    when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.
                    
            edges (dict, optional):
                Drawing configuration for the edges. The following arguments
                are available for property mapping: ``edge_color``, ``width``.
                
                The ``edge_color``, ``edge_cmap``, and ``colorbar`` arguments
                behave analogously for the edges as nodes.node_color,
                nodes.cmap, and nodes.colorbar for the nodes. Any further
                kwargs are (after applying property mapping), passed on to
                `draw_networkx_edges <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edges.html>`_
                when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.

                If arrows are to be drawn (i.e. for directed edges with
                arrows=True), only norms of type matplotlib.colors.Normalize
                are allowed.

            node_labels (dict, optional):
                Drawing configuration for the node labels. The following
                arguments are allowed:

                enabled (bool, optional):
                    Whether to draw node labels. Disabled by default. If
                    enabled, nodes are labeled by their index by default.
                show_only (list, optional):
                    If given, labels are drawn only for the nodes in this list.
                labels (dict, optional):
                    Custom text labels keyed by node. Available for property
                    mapping.
                format (str, optional):
                    If ``labels`` are mapped from property this format string
                    containing a ``label`` key is used for all node labels.
                decode (str, optional):
                    Decoding specifier which is applied to all property values
                    if ``format`` is used.
                further kwargs:
                    Passed on to `draw_networkx_labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_labels.html>`_
                    when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.

            edge_labels (dict, optional):
                Drawing configuration for the edge labels. The following
                arguments are allowed:

                enabled (bool, optional):
                    Whether to draw edge labels. Disabled by default. If
                    enabled, edges are labeled by their (source, destination)
                    pair by default.
                show_only (list, optional):
                    If given, labels are drawn only for the edges (2-tuples)
                    in this list.
                edge_labels (dict, optional):
                    Custom text labels keyed by edge (2-tuple). Available for
                    property mapping.
                format (str, optional):
                    If ``edge_labels`` are mapped from property this format
                    string containing a ``label`` key is used for all edge
                    labels.
                decode (str, optional):
                    Decoding specifier which is applied to all property values
                    if ``format`` is used.
                further kwargs:
                    Passed on to `draw_networkx_edge_labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edge_labels.html>`_
                    when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.

            mark_nodes (dict, optional):
                Mark specific nodes by changing their edgecolor. Either specify
                a ``color`` for a list of nodes (``nodelist``), or specify a
                ``colors`` dictionary of colors keyed by node. Updates an
                existing ``nodes.edgecolors`` entry.

            mark_edges(dict, optional):
                Mark specific edges by changing their color. Either specify a 
                ``color`` for a list of edges (``edgelist``), or specify a
                ``colors`` dictionary of colors keyed by edge (2-tuple).
                Updates an existing ``edges.edge_color`` entry.
        """
        # Set matplotlib figure and axis
        self.fig = fig if fig is not None else plt.gcf()
        self.ax = ax if ax is not None else self.fig.gca()

        self._g = g.copy()
        self._nodes_to_draw = None
        self._edges_to_draw = None
        self._nodes_to_shrink = None
        self.positions = None
        
        self._select_subgraph(**(select if select else {}))
        self.parse_positions(**(positions if positions else {}))

        # Drawing configurations
        # TODO With networkx v2.6 only FancyArrowPatches will be used for
        #      the edges (https://github.com/networkx/networkx/pull/4360).
        #      Then, there will be no simple way of showing edge colorbars.
        #      Remove the distinction between directed graphs (with arrows
        #      enabled) and undirected graphs at various places.
        self._node_colormanager = None
        self._edge_colormanager = None
        self._node_cbar_kwargs = {}
        self._edge_cbar_kwargs = {}
        self._show_node_cbar = False
        self._show_edge_cbar = False
        self._show_node_labels = False
        self._show_edge_labels = False
        self._node_kwargs = {}
        self._edge_kwargs = {}
        self._node_label_kwargs = {}
        self._edge_label_kwargs = {}

        self.parse_nodes(**(nodes if nodes else {}))
        self.parse_edges(**(edges if edges else {}))
        self.parse_node_labels(**(node_labels if node_labels else {}))
        self.parse_edge_labels(**(edge_labels if edge_labels else {}))
        self.mark_nodes(**(mark_nodes if mark_nodes else {}))
        self.mark_edges(**(mark_edges if mark_edges else {}))

        # matplotlib objects
        self._mpl_nodes = None
        self._mpl_edges = None
        self._mpl_node_labels = None
        self._mpl_edge_labels = None
        self._mpl_node_cbar = None
        self._mpl_edge_cbar = None

    # .........................................................................
    # Properties

    @property
    def g(self):
        """Get a deep copy of the graph associated with the GraphPlot instance.
        
        Returns:
            nx.Graph: The networkx graph object
        """
        return self._g.copy()

    # .........................................................................
    # Drawing methods

    def draw(
        self,
        *,
        fig=None,
        ax=None,
        positions: dict=None,
        nodes: dict=None,
        edges: dict=None,
        node_labels: dict=None,
        edge_labels: dict=None,
        mark_nodes: dict=None,
        mark_edges: dict=None,
        suppress_cbar: bool=False,
        update_colormapping: bool=True,
        **add_colorbars
    ):
        """
        Draws the graph associated with the ``GraphPlot`` using the current
        drawing configuration.
        
        The current drawing configuration may be temporarily updated for this
        plot. The respective arguments accept the same input as in
        :py:class:`~utopya.plot_funcs._graph.GraphPlot`.
        
        Args:
            fig (None, optional): matplotlib figure
            ax (None, optional): matplotlib axis
            positions (dict, optional): Position configuration. If given,
                the current positions are replaced. If using a node positioning
                model the positions are recalculated.
            nodes (dict, optional): Temporarily updates the node-kwargs
            edges (dict, optional): Temporarily updates the edge-kwargs
            node_labels (dict, optional): Temporarily updates the
                node-label-kwargs
            edge_labels (dict, optional): Temporarily updates the
                edge-label-kwargs
            mark_nodes (dict, optional): Temporarily mark nodes kwargs
            mark_edges (dict, optional): Temporarily mark edges kwargs
            suppress_cbar (bool, optional): Whether to suppress the drawing of
                colorbars
            update_colormapping (bool, optional): Whether to reconfigure the
                nodes' and edges'
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`
                (default=True). If True, the respective configuration entries
                are ignored. Set to False if doing repetitive plotting with
                fixed colormapping.
            **add_colorbars: Passed to
                :py:meth:`~utopya.plot_funcs._graph.GraphPlot.add_colorbars`
        """
        # Cache the current drawing configuration
        node_colormanager_cache = copy.deepcopy(self._node_colormanager)
        edge_colormanager_cache = copy.deepcopy(self._edge_colormanager)
        node_cbar_kwargs_cache = copy.deepcopy(self._node_cbar_kwargs)
        edge_cbar_kwargs_cache = copy.deepcopy(self._edge_cbar_kwargs)
        show_node_cbar_cache = self._show_node_cbar
        show_edge_cbar_cache = self._show_edge_cbar
        show_node_labels_cache = self._show_node_labels
        show_edge_labels_cache = self._show_edge_labels
        node_kwargs_cache = copy.deepcopy(self._node_kwargs)
        edge_kwargs_cache = copy.deepcopy(self._edge_kwargs)
        node_label_kwargs_cache = copy.deepcopy(self._node_label_kwargs)
        edge_label_kwargs_cache = copy.deepcopy(self._edge_label_kwargs)

        fig = fig if fig is not None else self.fig
        ax = ax if ax is not None else self.ax

        if positions is not None:
            self.parse_positions(**positions)

        self.parse_nodes(
            update_colormapping=update_colormapping,
            **(nodes if nodes else {}),
        )
        self.parse_edges(
            update_colormapping=update_colormapping,
            **(edges if edges else {}),
        )
        self.parse_node_labels(**(node_labels if node_labels else {}))
        self.parse_edge_labels(**(edge_labels if edge_labels else {}))
        self.mark_nodes(**(mark_nodes if mark_nodes else {}))
        self.mark_edges(**(mark_edges if mark_edges else {}))

        log.remark("Now drawing ...")

        # Draw nodes and edges
        self._mpl_nodes = nx.draw_networkx_nodes(
            self._g, pos=self.positions, ax=ax, **self._node_kwargs
        )
        self._mpl_edges = nx.draw_networkx_edges(
            self._g, pos=self.positions, ax=ax, **self._edge_kwargs
        )

        # NOTE networkx does not pass on the norms to the respective matplotlib
        #      functions. Hence, they need to be set manually. For the edges,
        #      the cmap also needs to be set manually. Can only be set for the
        #      edges if graph is undirected or `arrows=False`.
        self._mpl_nodes.set_norm(self._node_colormanager.norm)

        if not isinstance(self._mpl_edges, list):
            self._mpl_edges.set_norm(self._edge_colormanager.norm)
            self._mpl_edges.set_cmap(self._edge_colormanager.cmap)

        # Draw node labels and edge labels
        if self._show_node_labels:
            self._mpl_node_labels = nx.draw_networkx_labels(
                self._g,
                pos=self.positions,
                ax=ax,
                **self._node_label_kwargs,
            )
        if self._show_edge_labels:
            self._mpl_edge_labels = nx.draw_networkx_edge_labels(
                self._g,
                pos=self.positions,
                ax=ax,
                **self._edge_label_kwargs,
            )

        if not suppress_cbar:
            self.add_colorbars(
                show_node_cbar=self._show_node_cbar,
                show_edge_cbar=self._show_edge_cbar,
                fig=fig,
                ax=ax,
                **add_colorbars,
            )

        ax.axis("off")

        # Restore the previous drawing configuration
        self._node_colormanager = node_colormanager_cache
        self._edge_colormanager = edge_colormanager_cache
        self._node_cbar_kwargs = node_cbar_kwargs_cache
        self._edge_cbar_kwargs = edge_cbar_kwargs_cache
        self._show_node_cbar = show_node_cbar_cache
        self._show_edge_cbar = show_edge_cbar_cache
        self._show_node_labels = show_node_labels_cache
        self._show_edge_labels = show_edge_labels_cache
        self._node_kwargs = node_kwargs_cache
        self._edge_kwargs = edge_kwargs_cache
        self._node_label_kwargs = node_label_kwargs_cache
        self._edge_label_kwargs = edge_label_kwargs_cache

    def add_colorbars(
        self,
        *,
        show_node_cbar=True,
        show_edge_cbar=True,
        fig=None,
        ax=None,
        remove_previous=True,
        **update_cbar_kwargs
    ):
        """Adds colorbars for the drawn nodes and edges.
        
        Args:
            show_node_cbar (bool, optional): Whether to create a colorbar for
                the nodes
            show_edge_cbar (bool, optional): Whether to create a colorbar for
                the edges
            fig (None, optional): matplotlib figure
            ax (None, optional): matplotlib axis
            remove_previous (bool, optional): Whether the colorbars which are
                currently associated with the ``GraphPlot`` are removed.
                If False, the GraphPlot still loses track of the colorbars,
                they can not be removed via the GraphPlot afterwards.
            **update_cbar_kwargs: Update both node and edge colorbar kwargs,
                passed to :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`.
        """
        fig = fig if fig is not None else self.fig
        ax = ax if ax is not None else self.ax

        if show_node_cbar:
            if remove_previous and self._mpl_node_cbar:
                self._mpl_node_cbar.remove()

            self._mpl_node_cbar = self._node_colormanager.create_cbar(
                self._mpl_nodes,
                fig=fig,
                ax=ax,
                **self._node_cbar_kwargs,
                **update_cbar_kwargs,
            )

        if show_edge_cbar:
            if isinstance(self._mpl_edges, list):
                # When drawing arrows, draw_networkx_edges returns a list of
                # FancyArrowPatches which can not be used directly in
                # fig.colorbar, but needs conversion to a PatchCollection and
                # manual transfer of the chosen colormap and normalization ...
                edge_pc = mpl.collections.PatchCollection(self._mpl_edges)
                edge_pc.set_norm(self._edge_colormanager.norm)
                edge_pc.set_cmap(self._edge_colormanager.cmap)
            else:
                edge_pc = self._mpl_edges

            if remove_previous and self._mpl_edge_cbar:
                self._mpl_edge_cbar.remove()

            self._mpl_edge_cbar = self._edge_colormanager.create_cbar(
                edge_pc,
                fig=fig,
                ax=ax,
                **self._edge_cbar_kwargs,
                **update_cbar_kwargs,
            )

    def clear_plot(self, *, keep_colorbars: bool=False):
        """Removes all matplotlib objects associated with the GraphPlot from
        the respective axis. The GraphPlot loses track of all those objects,
        the respective class attributes are reset.
        
        Args:
            keep_colorbars (bool, optional): Whether to keep the node and edge
                colorbars. If True, the GraphPlot still loses track of the
                colorbars, they can not be removed via the GraphPlot
                afterwards.
        """
        # Remove colorbars. It is important to remove them before the
        # associated mappable.
        if not keep_colorbars:
            if self._mpl_node_cbar:
                try:
                    self._mpl_node_cbar.remove()
                except:
                    pass
            if self._mpl_edge_cbar:
                try:
                    self._mpl_edge_cbar.remove()
                except:
                    pass

        # Remove nodes and edges
        if self._mpl_nodes:
            self._mpl_nodes.remove()

        # Distinguish between FancyArrowPatch-list and LineCollection
        if isinstance(self._mpl_edges, list):
            for i in range(len(self._mpl_edges)):
                self._mpl_edges[i].remove()

        elif self._mpl_edges:
            self._mpl_edges.remove()

        # Remove labels
        if self._mpl_node_labels:
            for label in self._mpl_node_labels.values():
                label.remove()

        if self._mpl_edge_labels:
            for label in self._mpl_edge_labels.values():
                label.remove()

        # Reset class attributes
        self._mpl_node_cbar = None
        self._mpl_edge_cbar = None
        self._mpl_nodes = None
        self._mpl_edges = None
        self._mpl_node_labels = None
        self._mpl_edge_labels = None

        log.remark("Cleared GraphPlot.")

    # .........................................................................
    # Public helper methods

    @classmethod
    def from_group(
        cls,
        graph_group,
        *,
        graph_creation: dict=None,
        register_property_maps: dict=None,
        clear_existing_property_maps: bool=True,
        **init_kwargs
    ):
        """Initializes a ``GraphPlot`` from a
        :py:class:`~utopya.datagroup.GraphGroup`.

        Args:
            graph_group: The graph group
            graph_creation (dict, optional): Configuration of the graph
                creation. Passed to :py:meth:`~utopya.plot_funcs._graph.GraphPlot.create_graph_from_group`
            register_property_maps (dict, optional): Properties to be
                registered in the graph group before the graph creation keyed
                by name
            clear_existing_property_maps (bool, optional): Whether to clear any
                existing property maps from the graph group
            **init_kwargs: Passed to
                :py:class:`~utopya.plot_funcs._graph.GraphPlot`
        """
        g = cls.create_graph_from_group(
            graph_group,
            register_property_maps=register_property_maps,
            clear_existing_property_maps=clear_existing_property_maps,
            **(graph_creation if graph_creation is not None else {})
        )

        return cls(g=g, **init_kwargs)

    @staticmethod
    def create_graph_from_group(
        graph_group,
        *,
        register_property_maps: dict=None,
        clear_existing_property_maps: bool=True,
        **graph_creation
    ) -> nx.Graph:
        """Creates a ``networkx.Graph`` from a ``GraphGroup``. Additional
        property maps may be added to the group beforehand.

        Args:
            graph_group: The ``GraphGroup``.
            register_property_maps (dict, optional): Properties to be
                registered in the graph group before the graph creation keyed
                by name.
            clear_existing_property_maps (bool, optional): Whether to clear any
                existing property maps from the graph group.
            **graph_creation: Configuration of the graph creation. Passed on
                to :py:meth:`~utopya.datagroup.GraphGroup.create_graph`.

        Returns:
            nx.Graph: The created ``networkx.Graph`` object.
        """
        # Register external property data
        if register_property_maps:
            # Clear existing property maps in order to not have side effects if
            # plotting multiple times, e.g. in interactive mode. This is
            # important because the graph_group most probably is a reference.
            if clear_existing_property_maps:
                graph_group.property_maps.clear()

            # Can register now
            for tag, pmap in register_property_maps.items():
                graph_group.register_property_map(tag, pmap)

        g = graph_group.create_graph(**graph_creation)
        return g

    def parse_positions(
        self,
        *,
        from_dict: Dict[Any, Tuple[float, float]]=None,
        model: Union[str, Callable]=None,
        **kwargs
    ):
        """Parses the node positioning configuration. If a node positioning
        model is to be used, (re)calculates the positions.

        Args:
            from_dict (dict, optional): Explicit node positions (2-tuples keyed
                by node). If given, the ``model`` argument will be ignored.
            model (Union[str, Callable], optional): The model used for node
                positioning. If it is a string, it is looked up from the
                available networkx positioning models. If None, the spring
                model is used.
                If it is callable, it will be called with the graph as first
                positional argument.
            **kwargs: Passed to the node positioning routine

        Raises:
            ModuleNotFoundError: If a graphviz model was chosen but pygraphviz
                was not importable (via networkx)
        """
        # Set spring-layout as default if nothing else is specified
        if from_dict is None and model is None:
            model = "spring"

        if from_dict is not None:
            if model is not None:
                warnings.warn(
                    "Node positions were specified *both* via a positioning "
                    "model and explicitly via the `from_dict` argument. The "
                    "specified model will be ignored. To remove this warning, "
                    "set the graph_drawing.positions.model entry to None.",
                    UserWarning
                )
            self.positions = copy.deepcopy(from_dict)

        else:
            log.remark(
                "Calculating the node positions using a positioning model ..."
            )
            
            if callable(model):
                self.positions = model(self._g, **kwargs)

            elif model.startswith('graphviz_'):
                try:
                    # graphviz models
                    model = model[9:]
                    self.positions = nx.nx_pydot.graphviz_layout(
                        self._g, prog=model, **kwargs
                    )

                except ModuleNotFoundError as err:
                    raise ModuleNotFoundError(
                        "When trying to use the graphviz node positioning "
                        f"model '{model}': '{err}'"
                    ) from err

            # else: is a networkx positioning model
            else:
                self.positions = POSITIONING_MODELS_NETWORKX[model](
                    self._g, **kwargs
                )

    def parse_nodes(
        self,
        *,
        node_size=None,
        node_color=None,
        alpha=None,
        cmap=None,
        cmap_norm=None,
        vmin: float=None,
        vmax: float=None,
        edgecolors=None,
        colorbar: dict=None,
        update_colormapping: bool=True,
        **kwargs
    ):
        """Parses the node layout configuration and updates the node kwargs of
        the GraphPlot.

        The following arguments are available for property mapping:
        ``node_size``, ``node_color``, ``alpha``.
        
        Args:
            node_size (None, optional): Size of nodes (default=300). Available
                for property mapping. Can be mapped directly from the nodes'
                ``degree``, ``in_degree``, or ``out_degree`` by setting the
                ``from_property`` argument accordingly.
            node_color (None, optional): Single color (string or RGB(A) tuple
                or numeric value) or sequence of colors (default='#1f78b4').
                If numeric values are specified they will be mapped to colors
                using the cmap and vmin, vmax parameters.

                If mapped from property it may contain an additional
                ``map_to_scalar``, which is a dict of numeric target values
                keyed by property value. This allows to map from non-numeric
                (e.g. categorical) properties.
            alpha (None, optional): The node transparency
            cmap (None, optional): The colormap. Passed as ``cmap`` to
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
            cmap_norm (None, optional): The norm used for the color mapping.
                Passed as ``norm`` to
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                Is overwritten, if a discrete colormap is specified in
                ``cmap``.
            vmin (float, optional): Minimum for the colormap scaling
            vmax (float, optional): Maximum for the colormap scaling
            edgecolors (optional): Colors of node borders. The default is
                'none', i.e. no node border is drawn.
            colorbar (dict, optional): The node colorbar configuration.
                The following keys are available:

                enabled (bool, optional):
                    Whether to plot a colorbar. Enabled by default if
                    ``node_color`` is mapped from property.
                labels (dict, optional):
                    Colorbar tick-labels keyed by tick position (see
                    :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`).
                further kwargs: 
                    Passed on to :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`.

            update_colormapping (bool, optional): Whether to reconfigure the
                nodes' :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`
                (default=True). If False, the respective arguments are ignored.
                Set to False if doing repetitive plotting with fixed
                colormapping.
            **kwargs: Update the node kwargs. Passed to nx.draw_networkx_nodes
                when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`
        """
        if kwargs.pop("nodelist", None) is not None:
            warnings.warn(
                "The 'nodelist' argument will be ignored. To draw a subset of "
                "nodes use the 'select.nodelist' argument instead.",
                UserWarning
            )

        # Update node kwargs with simple kwargs. All kwargs that might need
        # extra treatment are caught explicitly and handled below.
        self._node_kwargs.update(kwargs)
        
        self._node_kwargs["nodelist"] = self._nodes_to_draw

        # Do the property mapping ...
        # Node sizes
        if isinstance(node_size, dict) and "from_property" in node_size:
            prop = node_size["from_property"]
            interval = node_size.get("scale_to_interval", None)

            try:
                _node_sizes = np.array(
                    [self._g.nodes[n][prop] for n in self._nodes_to_draw]
                )
            except KeyError as err:
                if prop == "degree":
                    _node_sizes =  node_sizes = np.array(
                        [self._g.degree[n] for n in self._nodes_to_draw]
                    )
                elif prop == "in_degree":
                    _node_sizes =  node_sizes = np.array(
                        [self._g.in_degree[n] for n in self._nodes_to_draw]
                    )
                elif prop == "out_degree":
                    _node_sizes =  node_sizes = np.array(
                        [self._g.out_degree[n] for n in self._nodes_to_draw]
                    )
                else:
                    raise ValueError(
                        f"No property {prop} found. Make sure the property "
                        "exists for all nodes to draw. Additional options: "
                        "degree, in_degree, out_degree"
                    ) from err

            # If there are nodes to be shrinked, set their size to zero
            to_shrink = np.isin(self._nodes_to_draw, self._nodes_to_shrink)
            _node_sizes[to_shrink] = 0
            _node_sizes[~to_shrink] = self._scale_to_interval(
                _node_sizes[~to_shrink], interval
            )
            self._node_kwargs["node_size"] = list(_node_sizes)

        elif node_size is not None:
            self._node_kwargs["node_size"] = node_size

        # Node colors
        if isinstance(node_color, dict) and "from_property" in node_color:
            prop = node_color["from_property"]
            interval = node_color.get("scale_to_interval", None)

            # If provided a mapping, map the property values to scalar values
            if "map_to_scalar" in node_color:
                map_to_scalar = np.vectorize(node_color["map_to_scalar"].get)
                _node_colors = list(map_to_scalar(_node_colors))

            else:
                _node_colors = self._scale_to_interval(
                    [self._g.nodes[n][prop] for n in self._nodes_to_draw],
                    interval,
                )

            self._node_kwargs["node_color"] = _node_colors
            self._show_node_cbar = True

        elif node_color is not None:
            self._node_kwargs["node_color"] = node_color

        # Node transparency
        if isinstance(alpha, dict) and "from_property" in alpha:
            prop = alpha["from_property"]
            interval = alpha.get("scale_to_interval", None)

            self._node_kwargs["alpha"] = self._scale_to_interval(
                [self._g.nodes[n][prop] for n in self._nodes_to_draw], interval
            )

        elif alpha is not None:
            self._node_kwargs["alpha"] = alpha

        # ... property mapping done.

        # If a single or no value was set for the node size, create an explicit
        # node size mapping if there are nodes to be shrinked to size zero.
        # If no size was given, use the default size 300.
        if self._nodes_to_shrink and not isinstance(
            self._node_kwargs.get("node_size", None), list
        ):
            _size = self._node_kwargs.get("node_size", 300)
            self._node_kwargs["node_size"] = [
                0 if n in self._nodes_to_shrink
                else _size
                for n in self._nodes_to_draw
            ]

        # Update the edgecolors of the nodes. Set the edgecolor to 'none'
        # (=transparent) if it has not been set yet.
        if edgecolors is not None:
            self._node_kwargs["edgecolors"] = edgecolors

        elif "edgecolors" not in self._node_kwargs:
            self._node_kwargs["edgecolors"] = "none"

        # For directed graphs (arrows enabled) individual FancyArrowPatches are
        # drawn. Make sure that the arrows end right at the node borders.
        if self._g.is_directed():
            self._configure_node_patch_sizes()

        # Get colorbar configuration and update the existing kwargs
        colorbar = colorbar if colorbar is not None else {}
        cbar_labels = colorbar.pop("labels", None)
        self._show_node_cbar = colorbar.pop("enabled", self._show_node_cbar)
        self._node_cbar_kwargs.update(colorbar)

        # (Re-)create the colormanager. Only do so if there is no colormanager
        # yet or if any kwargs are given.
        if update_colormapping and (
            self._node_colormanager is None
            or any(
                [
                    arg is not None 
                    for arg in (cmap, cmap_norm, vmin, vmax, cbar_labels)
                ]
            )
        ):
            # Replace all kwargs that are None with the current configuration.
            # That way the current configuration is not lost but updated.
            if vmin is None:
                vmin = self._node_kwargs.get("vmin", None)
            if vmax is None:
                vmax = self._node_kwargs.get("vmax", None)
            if self._node_colormanager is not None:
                if cmap is None:
                    cmap = self._node_colormanager.cmap
                if cmap_norm is None:
                    cmap_norm = self._node_colormanager.norm
                if cbar_labels is None:
                    cbar_labels = self._node_colormanager.labels

            # Set up the ColorManager
            self._node_colormanager = ColorManager(
                cmap=cmap,
                norm=cmap_norm,
                labels=cbar_labels,
                vmin=vmin,
                vmax=vmax,
            )
            self._node_kwargs["cmap"] = self._node_colormanager.cmap

    def parse_edges(
        self,
        *,
        width=None,
        edge_color=None,
        edge_cmap=None,
        cmap_norm=None,
        edge_vmin: float=None,
        edge_vmax: float=None,
        colorbar: dict=None,
        update_colormapping: bool=True,
        **kwargs
    ):
        """Parses the edge layout configuration and updates the edge kwargs of
        the GraphPlot.

        The following arguments are available for property mapping:
        ``width``, ``edge_color``.
        
        Args:
            width (None, optional): Line width of edges
            edge_color (None, optional): Single color (string or RGB(A) tuple
                or numeric value) or sequence of colors (default='k').
                If numeric values are specified they will be mapped to colors
                using the edge_cmap and edge_vmin, edge_vmax parameters.

                If mapped from property it may contain an additional
                ``map_to_scalar``, which is a dict of numeric target values
                keyed by property value. This allows to map from non-numeric
                (e.g. categorical) properties.
            edge_cmap (None, optional): The colormap. Passed as ``cmap`` to
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
            cmap_norm (None, optional): The norm used for the color mapping.
                Passed as ``norm`` to
                :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.
                Is overwritten, if a discrete colormap is specified in
                ``edge_cmap``.
                If arrows are to be drawn (i.e. for directed edges with
                arrows=True), only norms of type matplotlib.colors.Normalize
                are allowed.
            edge_vmin (float, optional): Minimum for the colormap scaling
            edge_vmax (float, optional): Maximum for the colormap scaling
            colorbar (dict, optional): The edge colorbar configuration.
                The following keys are available:

                enabled (bool, optional):
                    Whether to plot a colorbar. Enabled by default if
                    ``edge_color`` is mapped from property.
                labels (dict, optional):
                    Colorbar tick-labels keyed by tick position (see
                    :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`).
                further kwargs: 
                    Passed on to :py:meth:`~utopya.plot_funcs._mpl_helpers.ColorManager.create_cbar`.

            update_colormapping (bool, optional): Whether to reconfigure the
                edges' :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`
                (default=True). If False, the respective arguments are ignored.
                Set to False if doing repetitive plotting with fixed
                colormapping.
            **kwargs: Update the edge kwargs. Passed to nx.draw_networkx_edges
                when calling :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.

        Raises:
            TypeError: On norm type other than mpl.colors.Normalize if arrows
                are to be drawn.
        """
        if kwargs.pop("edgelist", None) is not None:
            warnings.warn(
                "The 'edgelist' argument will be ignored. Use the select "
                "configuration to specify a subgraph to be drawn.",
                UserWarning
            )
        self._edge_kwargs["edgelist"] = self._edges_to_draw

        # Update edge kwargs with simple kwargs. All kwargs that might need
        # extra treatment are caught explicitly and handled below.
        self._edge_kwargs.update(kwargs)

        # Do the property mapping ...
        # Edge width
        if isinstance(width, dict) and "from_property" in width:
            prop = width["from_property"]
            interval = width.get("scale_to_interval", None)

            self._edge_kwargs["width"] = self._scale_to_interval(
                [self._g.edges[n][prop] for n in self._edges_to_draw], interval
            )

        elif width is not None:
            self._edge_kwargs["width"] = width

        # Node colors
        if isinstance(edge_color, dict) and "from_property" in edge_color:
            prop = edge_color["from_property"]
            interval = edge_color.get("scale_to_interval", None)

            # If provided a mapping, map the property values to scalar values
            if "map_to_scalar" in edge_color:
                map_to_scalar = np.vectorize(edge_color["map_to_scalar"].get)
                _edge_colors = list(map_to_scalar(_edge_colors))

            else:
                _edge_colors = self._scale_to_interval(
                    [self._g.edges[n][prop] for n in self._edges_to_draw],
                    interval,
                )

            self._edge_kwargs["edge_color"] = _edge_colors
            self._show_edge_cbar = True

        elif edge_color is not None:
            self._edge_kwargs["edge_color"] = edge_color

        # ... property mapping done.

        # Get colorbar configuration and update the existing kwargs
        colorbar = colorbar if colorbar is not None else {}
        cbar_labels = colorbar.pop("labels", None)
        self._show_edge_cbar = colorbar.pop("enabled", self._show_edge_cbar)
        self._edge_cbar_kwargs.update(colorbar)
        
        # (Re-)create the colormanager. Only do so if there is no colormanager
        # yet or if any kwargs are given.
        if update_colormapping and (
            self._edge_colormanager is None
            or any(
                [
                    arg is not None 
                    for arg in (
                        edge_cmap, cmap_norm, edge_vmin, edge_vmax, cbar_labels
                    )
                ]
            )
        ):
            # Replace all kwargs that are None with the current configuration.
            # That way the current configuration is not lost but updated.
            if edge_vmin is None:
                edge_vmin = self._edge_kwargs.get("edge_vmin", None)
            if edge_vmax is None:
                edge_vmax = self._edge_kwargs.get("edge_vmax", None)
            if self._edge_colormanager is not None:
                if edge_cmap is None:
                    edge_cmap = self._edge_colormanager.cmap
                if cmap_norm is None:
                    cmap_norm = self._edge_colormanager.norm
                if cbar_labels is None:
                    cbar_labels = self._edge_colormanager.labels

            # Set up the ColorManager
            self._edge_colormanager = ColorManager(
                cmap=edge_cmap,
                norm=cmap_norm,
                labels=cbar_labels,
                vmin=edge_vmin,
                vmax=edge_vmax,
            )
            self._edge_kwargs["edge_cmap"] = self._edge_colormanager.cmap

        # NOTE In draw_networkx_edges, the Normalize norm is applied
        #      explicitly. Since the norm cannot be updated later (as edges
        #      with arrows are FancyArrowPatches), other norms than Normalize
        #      are forbidden here. When mapping the colors using the
        #      Colormanager a colorbar drawn from the edges mappable would
        #      still be wrong.
        if (
            isinstance(self._g, nx.DiGraph)
            and self._edge_kwargs.get("arrows", True)
            and type(self._edge_colormanager.norm) != mpl.colors.Normalize
        ):
            raise TypeError(
                "Received invalid norm type: "
                f"{type(self._edge_colormanager.norm)}. For directed edges"
                " with 'arrows=True', only the matplotlib.colors.Normalize"
                " base class is supported."
            )

    def parse_node_labels(
        self,
        *,
        enabled: bool=False,
        show_only: list=None,
        labels: dict=None,
        format: str="{label}",
        decode: str=None,
        **kwargs
    ):
        """Parses the node labels configuration and updates the node label
        kwargs of the GraphPlot.
        
        Args:
            enabled (bool, optional): Whether to draw node labels.
            show_only (list, optional): If given, labels are drawn only for the
                nodes in this list
            labels (dict, optional): Custom text labels keyed by node.
                Available for property mapping.
            format (str, optional): If ``labels`` are mapped from property this
                format string containing a ``label`` key is used for all node
                labels.
            decode (str, optional): Decoding specifier which is applied to all
                property values if ``format`` is used.
            **kwargs: Update the node label kwargs. Passed to
                nx.draw_networkx_labels when calling
                :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.
        """
        if enabled:
            self._show_node_labels = True

        # Update node label kwargs with simple kwargs. All kwargs that might
        # need extra treatment are caught explicitly and handled below.
        self._node_label_kwargs.update(kwargs)

        if not enabled and not labels:
            # Don't update the node_labels.labels -> can stop here
            return

        def to_show(n):
            if show_only is None:
                return n not in self._nodes_to_shrink
            else:
                return n in show_only and n not in self._nodes_to_shrink

        if labels:
            if "from_property" in labels:
                prop = labels["from_property"]
                _labels = {
                    n: format.format(
                        label=(
                            self._g.nodes[n][prop]
                            if decode is None
                            else self._g.nodes[n][prop].decode(decode)
                        )
                    )
                    for n in self._nodes_to_draw
                    if to_show(n)
                }

            else:
                _labels = copy.deepcopy(labels)

                if show_only is not None:
                    # show_only takes precedence over the provided node labels.
                    # Keep only those that are in show_only. nodes in show_only
                    # for which no label is given are labeled with their index.
                    for n in labels.keys():
                        if not to_show(n):
                            del _labels[n]

                    for n in show_only:
                        if (
                            n not in _labels.keys()
                            and n not in self._nodes_to_shrink
                        ):
                            _labels[n] = n

        elif enabled:
            # If enabled but no labels given, label nodes with their index.
            _labels = {n: n for n in self._nodes_to_draw if to_show(n)}

        self._node_label_kwargs["labels"] = _labels

    def parse_edge_labels(
        self,
        *,
        enabled: bool=False,
        show_only: list=None,
        edge_labels: dict=None,
        format: str="{label}",
        decode: str=None,
        **kwargs
    ):
        """Parses the edge labels configuration and updates the edge label
        kwargs of the GraphPlot.
        
        Args:
            enabled (bool, optional): Whether to draw edge labels.
            show_only (list, optional): If given, labels are drawn only for the
                edges (2-tuples) in this list
            edge_labels (dict, optional): Custom text labels keyed by edge
                (2-tuple). Available for property mapping.
            format (str, optional): If ``edge_labels`` are mapped from property
                this format string containing a ``label`` key is used for all
                edge labels.
            decode (str, optional): Decoding specifier which is applied to all
                property values if ``format`` is used.
            **kwargs: Update the edge label kwargs. Passed to
                nx.draw_networkx_edge_labels when calling
                :py:meth:`~utopya.plot_funcs._graph.GraphPlot.draw`.
        """
        # Catch a pitfall: There is no 'labels' argument for the edge labels
        # (as there is for the node labels), here it is named 'edge_labels'.
        if "labels" in kwargs:
            raise ValueError(
                "Received invalid 'labels' key in edge label configuration. "
                "For specifying an edge label dict, use the key 'edge_labels'."
            )

        if enabled:
            self._show_edge_labels = True

        # Update edge label kwargs with simple kwargs. All kwargs that might
        # need extra treatment are caught explicitly and handled below.
        self._edge_label_kwargs.update(kwargs)

        if not enabled and not edge_labels:
            # Don't update the edge_labels.labels -> can stop here
            return

        def to_show(e):
            return True if show_only is None else e[:2] in show_only

        if show_only is not None:
            show_only = [tuple(e) for e in show_only]

        if edge_labels:
            if "from_property" in edge_labels:
                prop = edge_labels["from_property"]
                _labels = {
                    e[:2]: format.format(
                        label=(
                            self._g.edges[e][prop]
                            if decode is None
                            else self._g.edges[e][prop].decode(decode)
                        )
                    )
                    for e in self._edges_to_draw
                    if to_show(e)
                }

            else:
                _labels = copy.deepcopy(edge_labels)

                if show_only is not None:
                    # show_only takes precedence over the provided edge labels.
                    # Keep only those that are in show_only. edges in show_only
                    # for which no label is given are labeled with their
                    # (source, destination) pair.
                    for e in edge_labels.keys():
                        if not to_show(e):
                            del _labels[e]

                    for e in show_only:
                        if e not in _labels.keys():
                            _labels[e] = e

        elif enabled:
            # If enabled but no labels given, label edges with their
            # (source, destination) pair.
            _labels = {e[:2]: e[:2] for e in self._edges_to_draw if to_show(e)}

        self._edge_label_kwargs["edge_labels"] = _labels

    def mark_nodes(
        self, *, nodelist: list=None, color=None, colors: dict=None
    ):
        """Mark specific nodes by changing their edgecolor.

        .. note::

            This function overwrites the ``edgecolors`` entry in the node
            kwargs. Thus it might overwrite an existing ``edgecolors`` entry
            specified via
            :py:meth:`~utopya.plot_funcs._graph.GraphPlot.parse_nodes`
            (and vice versa).
        
        Args:
            nodelist (list, optional): Nodes to mark with the color specified
                via ``color``
            color (None, optional): Single edgecolor to use for the nodes in
                ``nodelist``
            colors (dict, optional): Edgecolors keyed by node to mark. Must be
                None if ``nodelist`` is given.

        Raises:
            ValueError: On ambiguous or missing mark_nodes configuration
        """
        if nodelist is None and color is None and colors is None:
            return

        if colors is not None:
            if not (nodelist is None and color is None):
                raise ValueError(
                    "Received invalid 'mark_nodes' kwargs. Provide _either_ a "
                    "'colors' dict only _or_ an 'nodelist' together with a "
                    "single 'color'."
                )

        elif (nodelist is None) != (color is None):
            _missing_arg = "nodelist" if nodelist is None else "color"
            raise ValueError(
                f"Missing argument '{_missing_arg}' in 'mark_nodes' kwargs."
            )

        if self._node_kwargs.get("edgecolors", None) is None:
            # There is no edgecolors entry yet. Use the current node colors
            # as base colors. If there are none, use the default networkx node
            # color.
            base_color = self._node_kwargs.get("node_color", "none")
        else:
            # Use the existing edgecolors entry.
            base_color = self._node_kwargs["edgecolors"]

        # From the base color(s) create dict of colors keyed by node
        if mpl.colors.is_color_like(base_color):
            _colors = {n: base_color for n in self._nodes_to_draw}
        else:
            # ... must be a list of colors.
            # If base_color contains numeric values, they need to be
            # transformed via the specified colormap.
            if not mpl.colors.is_color_like(base_color[0]):
                base_color = self._node_colormanager.map_to_color(base_color)

            _colors = {
                n: base_color[i] for i, n in enumerate(self._nodes_to_draw)
            }

        # Update the color dict with the values from the mark configuration
        if nodelist:
            for n in nodelist:
                _colors[n] = color
        else:
            _colors.update(colors)

        # The color values are aligned with 'self._nodes_to_draw' since
        # dictionaries don't change their ordering.
        self._node_kwargs["edgecolors"] = list(_colors.values())

        # Reconfigure the node patch sizes which increase if a node boundary
        # is drawn.
        if self._g.is_directed():
            self._configure_node_patch_sizes()

    def mark_edges(
        self, *, edgelist: list=None, color=None, colors: dict=None
    ):
        """Mark specific edges by changing their color.

        .. note::

            This function overwrites the ``edge_color`` entry in the edge
            kwargs. Thus it might overwrite an existing ``edge_color`` entry
            specified via
            :py:meth:`~utopya.plot_funcs._graph.GraphPlot.parse_edges`
            (and vice versa).
        
        Args:
            edgelist (list, optional): Edges to mark with the color specified
                via ``color``
            color (None, optional): Single color to use for the edges in
                ``edgelist``
            colors (dict, optional): Colors keyed by edge (2-tuple) to mark.
                Must be None if ``edgelist`` is given.

        Raises:
            ValueError: On ambiguous or missing mark_edges configuration
        """
        if edgelist is None and color is None and colors is None:
            return

        if colors is not None:
            if not (edgelist is None and color is None):
                raise ValueError(
                    "Received invalid 'mark_edges' kwargs. Provide _either_ a "
                    "'colors' dict only _or_ an 'edgelist' together with a "
                    "single 'color'."
                )

        elif (edgelist is None) != (color is None):
            _missing_arg = "edgelist" if edgelist is None else "color"
            raise ValueError(
                f"Missing argument '{_missing_arg}' in 'mark_edges' kwargs."
            )

        # Create dict of colors keyed by node based on the current edge_color.
        # If there is none, use the default networkx edge color.
        base_color = self._edge_kwargs.get("edge_color", "k")
        
        if mpl.colors.is_color_like(base_color):
            _colors = {e[:2]: base_color for e in self._edges_to_draw}
        else:
            # ... must be a list of colors. Transform to color-like if needed.
            if not mpl.colors.is_color_like(base_color[0]):
                base_color = self._edge_colormanager.map_to_color(base_color)

            _colors = {
                e[:2]: base_color[i] for i, e in enumerate(self._edges_to_draw)
            }

        # Update the color dict with the values from the mark configuration
        if edgelist:
            for e in edgelist:
                e = tuple(e)
                if not isinstance(self._g, nx.DiGraph) and e not in _colors:
                    e = e[::-1]
                _colors[e] = color
        else:
            for e, c in colors.items():
                if not isinstance(self._g, nx.DiGraph) and e not in _colors:
                    e = e[::-1]
                _colors[e] = c

        self._edge_kwargs["edge_color"] = list(_colors.values())

    # .........................................................................
    # Private helper methods

    def _configure_node_patch_sizes(self):
        """In the edge drawing kwargs adjusts the node patch specifications
        ensuring that, in the case of a directed graph, the arrows end exactly
        at the node boundaries. Besides the ``node_size`` also accounts for
        node boundaries.
        """
        # NOTE This also implies that edges can only be drawn if both their
        #      source and destination are drawn (as long as everything is based
        #      on a single _nodes_to_draw list).
        self._edge_kwargs["nodelist"] = self._nodes_to_draw

        if "node_shape" in self._node_kwargs:
            self._edge_kwargs["node_shape"] = self._node_kwargs["node_shape"]

        patch_size = self._node_kwargs.get("node_size", 300)
        edgecolors = self._node_kwargs.get("edgecolors", "none")

        # If node boundaries are drawn the size of the node patch increases
        # depending on the boundary width. Adjust the patch size accordingly.
        # Only do the patch size configuration if 'edgecolors' is not 'none'
        # (=no patch boundary).
        if edgecolors != "none":
            lw = self._node_kwargs.get(
                "linewidths", mpl.rcParams['lines.linewidth']
            )
            
            if not isinstance(patch_size, (int, float)):
                patch_size = np.array(patch_size)

            if not isinstance(lw, (int, float)):
                lw = np.array(lw)

            # Adjust the patch size wherever a node boundary is to be drawn
            if mpl.colors.is_color_like(edgecolors):
                patch_size = (np.sqrt(patch_size) + lw)**2

            else:
                patch_size = np.where(
                    [ec == "none" for ec in edgecolors],
                    patch_size,
                    (np.sqrt(patch_size) + lw)**2,
                )

        self._edge_kwargs["node_size"] = patch_size

    @staticmethod
    def _scale_to_interval(data: list, interval=None) -> list:
        """Rescales the data linearly to the given interval. If no interval is
        given the data is returned as it is.
        
        Args:
            data (list): data that is rescaled linearly to the given interval
            interval (optional): The target interval
        
        Returns:
            list: rescaled data
        
        Raises:
            ValueError: On invalid interval specification
        """
        if interval is None:
            return data

        try:
            lim_low, lim_up = interval
        except ValueError as err:
            raise ValueError(
                "'interval' must be a 2-tuple or list of length 2! Received: "
                f"{interval}"
            ) from err

        data = np.array(data)
        max_val = np.max(data)
        min_val = np.min(data)

        if max_val > min_val:
            rescaled_data = (
                (data - min_val) / (max_val - min_val)
                * (lim_up - lim_low) + lim_low
            )
        else:
            # If all values are equal, set them to the mean of the interval
            rescaled_data = np.zeros_like(data) + (lim_up - lim_low) / 2.

        return list(rescaled_data)

    def _select_subgraph(self, nodelist: list=None, drop: bool=True, **kwargs):
        """Select a subgraph to draw. Sets the lists of nodes and edges to draw
        and the nodes to shrink. Either a list of nodes is selected or radial
        selection is done.
        
        Args:
            nodelist (list, optional): If given, select nodes from list
            drop (bool, optional): Whether to remove the non-selected nodes and
                edges from the graph
            **kwargs: Passed to the selection routine
        """

        def select_from_list(*, nodelist: list, open_edges: bool=False):
            """Given a list of nodes, selects all nodes and edges needed for
            the graph drawing. If ``open_edges=False``, only those edges are
            selected for which both ends are in ``nodes``.
            
            Args:
                nodelist (list): Nodes to be selected
                open_edges (bool, optional): Whether to draw loose edges
            
            Returns:
                Tuple containing list of selected nodes, list of selected
                edges, and list of nodes to be shrinked to size zero.
            """
            subgraph = nx.induced_subgraph(self._g, nodelist)

            if open_edges:
                # Create outer subgraph from given nodes and all neighbors
                node_selection = set(nodelist)
                outer_nodes = set()

                for n in nodelist:
                    outer_nodes.update(nx.all_neighbors(self._g, n))

                outer_nodes -= node_selection
                node_selection = node_selection.union(outer_nodes)
                subgraph_outer = nx.induced_subgraph(self._g, node_selection)

                # Set of nodes to shrink is the difference of the two node sets
                nodes_to_shrink = list(subgraph_outer.nodes - set(nodelist))

                # From the outer subgraph remove edges between outer nodes
                edges_to_plot = (
                    subgraph_outer.edges
                    - nx.induced_subgraph(self._g, outer_nodes).edges
                )

                return (
                    list(subgraph_outer.nodes),
                    list(edges_to_plot),
                    nodes_to_shrink
                )

            return list(subgraph.nodes), list(subgraph.edges), []

        def select_radial(
            *, center: int, radius: int, open_edges: bool=False
        ):
            """Selects all nodes around a given center within a given radius
            (measured in numbers of neighborhoods). If ``open_edges=False``, those
            edges are selected for which both ends are in the set of selected
            nodes.
            
            Args:
                center (int): Index of the central node
                radius (int): Selection radius
                open_edges (bool, optional): Whether to draw loose edges
            
            Returns:
                Tuple containing list of selected nodes, list of selected
                edges, and list of nodes to be shrinked to size zero.
            """
            # After num_nodes-1 iterations (below), all nodes would be selected
            if radius > self._g.number_of_nodes()-1:
                radius = self._g.number_of_nodes()-1

            # Identify nodes within the given radius around the central node.
            # Start by adding the central nodes and all its neighbors to a set.
            # Then, iteratively add all neighbors of the previously added nodes
            # to the set, until the given radius is reached.
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
                    nbs_new.update(nx.all_neighbors(self._g, n))

                nbs_prev = nbs_new - node_selection
                node_selection = node_selection.union(nbs_new)
                nbs_new.clear()

            if open_edges:
                # Create inner subgraph from all nodes within the given radius
                subgraph_inner = nx.induced_subgraph(self._g, node_selection)

                # Create outer subgraph from all nodes within radius=radius+1
                for n in nbs_prev:
                    nbs_new.update(nx.all_neighbors(self._g, n))

                outer_nodes = nbs_new - node_selection
                node_selection = node_selection.union(nbs_new)
                subgraph_outer = nx.induced_subgraph(self._g, node_selection)

                # Set of nodes to shrink is the difference of the two node sets
                nodes_to_shrink = list(
                    subgraph_outer.nodes - subgraph_inner.nodes
                )

                # From the outer subgraph remove edges between outer nodes
                edges_to_plot = (
                    subgraph_outer.edges
                    - nx.induced_subgraph(self._g, outer_nodes).edges
                )

                # Return the inner subgraph nodes and the outer subgraph edges
                return (
                    list(subgraph_outer.nodes),
                    list(edges_to_plot),
                    nodes_to_shrink
                )

            subgraph = nx.induced_subgraph(self._g, node_selection)
            return list(subgraph.nodes), list(subgraph.edges), []

        # Select the nodes and edges to be drawn. The selection is always based
        # on the original graph self._g.
        if nodelist is None and not kwargs:
            # If no selection was specified, select all nodes and edges
            nodes_to_draw, edges_to_draw, nodes_to_shrink = (
                list(self._g.nodes), list(self._g.edges), []
            )
        elif nodelist is not None:
            # Select from list of nodes
            nodes_to_draw, edges_to_draw, nodes_to_shrink = select_from_list(
                nodelist=nodelist, **kwargs
            )
        else:
            # Perform radial selection
            nodes_to_draw, edges_to_draw, nodes_to_shrink = select_radial(
                **kwargs
            )

        if drop:
            # Remove the nodes that are not selected. This also removes all
            # edges for which the source or destination was removed.
            nodes_to_remove = self._g.nodes - set(nodes_to_draw)
            if nodes_to_remove:
                self._g.remove_nodes_from(nodes_to_remove)

        self._nodes_to_draw = nodes_to_draw
        self._edges_to_draw = edges_to_draw
        self._nodes_to_shrink = nodes_to_shrink
