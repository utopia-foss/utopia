Graph Plots
===========

.. contents::
    :local:
    :depth: 2

----

Draw Graph
----------

The ``draw_graph`` plot function combines and extends the following four networkx plotting utilities:

* :py:func:`networkx.drawing.nx_pylab.draw_networkx_nodes`
* :py:func:`networkx.drawing.nx_pylab.draw_networkx_edges`
* :py:func:`networkx.drawing.nx_pylab.draw_networkx_labels`
* :py:func:`networkx.drawing.nx_pylab.draw_networkx_edge_labels`

It preserves the full configurability of the above functions while offering additional features, e.g. plotting directly from a :py:class:`~utopya.eval.groups.GraphGroup`, automatically mapping data to layout properties, or creating graph animations.

Exemplary plot configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Check out an exemplary single graph plot configuration for the ``CopyMeGraph`` model:

.. literalinclude:: ../../../_inc/utopya/tests/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_plot_cfg
    :end-before:  ### End ---- graph_plot_cfg


.. note::

    If you have all ``node_props`` or ``edge_props`` in *one* container (HDF5 dataset) respectively, there are two possibilities to make subselections in the containers:

    1. When you only use *either* node *or* edge properties, or you want to make the same subselection on *both* your node *and* your edge dataset, you can simply do a ``.sel: {property: my_prop}``, where ``property`` is your dataset dimension, and ``my_prop`` the coordinate, or you can do the respective ``.isel``

    2. When you need to perform different subselections on your node and edge datasets—say you want node betweenness centrality and edge weight—you need to specify them via the DAG's select interface, and the procedure needs to include the following steps:

    .. code-block:: yaml

        select:
            betw:
              path: network/vertex_metrics
              transform:
                - .sel: [!dag_prev , {property: betweenness}]
                - .squeeze: !dag_prev
                  kwargs: {drop: true}
            wei: network/edge_properties
                - .sel: [!dag_prev , {property: weight}]
                - .squeeze: !dag_prev
                  kwargs: {drop: true}
            graph_group: g_static
        register_property_maps:
            - betw
            - wei
        compute_only: [graph_group, betw, wei]
        # clear_existing_property_maps: false
        graph_creation:
            at_time_idx: 0
            edge_props: [wei]
            node_props: [betw]
            # sel: { time: 0 } # applied to both properties
        graph_drawing:
            edges:
              edge_color: k
              width:
                from_property: wei
                scale_to_interval: [.1, 2.]
            nodes:
              node_color:
                from_property: betw





ColorManager configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

With the :py:class:`~utopya.eval.plots._mpl.ColorManager` integrated in :py:func:`~utopya.eval.plots.graph.draw_graph` you can conveniently configure the colormap, norm, and colorbar.
The examples below are meant to give an overview of its configuration possibilities.
In the examples, the *node*-specific arguments are used.
For configuring *edge*-colors, replace ``cmap``, ``vmin``, and ``vmax`` by ``edge_cmap``, ``edge_vmin``, and ``edge_vmax``.
For more details, see :py:class:`~utopya.eval.plots._mpl.ColorManager`.

A customized discrete colormap (e.g., for visualizing categorical properties) can be created via the ``cmap.from_values`` argument.
The matching colorbar labels can be defined via ``colorbar.labels`` (If the categories are ``[0, 1, 2, ...]`` one could also use the shortcut syntax, see :py:class:`~utopya.eval.plots._mpl.ColorManager`):

.. literalinclude:: ../../../_inc/utopya/tests/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_coloring_discrete
    :end-before:  ### End ---- graph_coloring_discrete
    :dedent: 6

For a nonlinear color-mapping, adjust the ``cmap_norm``:

.. literalinclude:: ../../../_inc/utopya/tests/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_coloring_lognorm
    :end-before:  ### End ---- graph_coloring_lognorm
    :dedent: 6

.. hint::

    When using the *BoundaryNorm* together with one of the pre-registered colormaps
    (e.g., *viridis*), use the ``lut`` argument (see :py:func:`matplotlib.cm.get_cmap`) to resample the colormap to have *lut* entries in the lookup table.
    Set ``lut = <BoundaryNorm.ncolors>`` to use the full colormap range.


See :py:func:`~utopya.eval.plots.graph.draw_graph` and :py:mod:`utopya.eval.plots.graph` for detailed interface information.

For detailed descriptions of the networkx plot functions that are used here, refer to the `networkx docs <https://networkx.github.io/documentation/stable/reference/drawing.html>`_.
