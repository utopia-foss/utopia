Graph Plots
===========

.. automodule:: utopya.plot_funcs.dag.graph
   :noindex:

For detailed descriptions of the networkx plot functions that are used here, refer to the `networkx docs <https://networkx.github.io/documentation/stable/reference/drawing.html>`_.

.. contents::
    :local:
    :depth: 2

----

Draw Graph
----------

The ``draw_graph`` plot function combines and extends the following four networkx plotting utilities:

* `plotting nodes <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_nodes.html>`_
* `plotting edges <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edges.html>`_
* `plotting node-labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_labels.html>`_
* `plotting edge-labels <https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx_edge_labels.html>`_

It preserves the full configurability of the above functions while offering additional features, e.g. plotting directly from a :py:class:`~utopya.datagroup.GraphGroup`, automatically mapping data to layout properties, or creating graph animations.

Exemplary plot configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Check out an exemplary single graph plot configuration for the ``CopyMeGraph`` model:

.. literalinclude:: ../../../../python/utopya/test/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_plot_cfg
    :end-before:  ### End ---- graph_plot_cfg


ColorManager configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

With the :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager` integrated in ``draw_graph`` you can conveniently configure the colormap, norm, and colorbar. The examples below are meant to give an overview of its configuration possibilities. In the examples, the *node*-specific arguments are used. For configuring *edge*-colors, replace ``cmap``, ``vmin``, and ``vmax`` by ``edge_cmap``, ``edge_vmin``, and ``edge_vmax``. For more details, see :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`.

A customized discrete colormap (e.g., for visualizing categorical properties) can be created via the ``cmap.from_values`` argument. The matching colorbar labels can be defined via ``colorbar.labels`` (If the categories are ``[0, 1, 2, ...]`` one could also use the shortcut syntax, see :py:class:`~utopya.plot_funcs._mpl_helpers.ColorManager`):

.. literalinclude:: ../../../../python/utopya/test/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_coloring_discrete
    :end-before:  ### End ---- graph_coloring_discrete
    :dedent: 6

For a nonlinear color-mapping, adjust the ``cmap_norm``:

.. literalinclude:: ../../../../python/utopya/test/cfg/plots/graph_plot_cfg.yml
    :language: yaml
    :start-after: ### Start -- graph_coloring_lognorm
    :end-before:  ### End ---- graph_coloring_lognorm
    :dedent: 6

.. hint::

    When using the *BoundaryNorm* together with one of the pre-registered colormaps
    (e.g., *viridis*), use the
    `lut <https://matplotlib.org/api/cm_api.html#matplotlib.cm.get_cmap>`_
    argument to resample the colormap to have *lut* entries in the lookup table.
    Set ``lut = <BoundaryNorm.ncolors>`` to use the full colormap range.


.. autofunction:: utopya.plot_funcs.dag.graph.draw_graph
    :noindex:
