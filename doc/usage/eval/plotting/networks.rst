.. _plot_networks:

Networks
========

.. admonition:: Summary

    On this page, you will see how to

    * use ``.plot.graph`` to plot networks
    * plot data as the node size and/or color
    * plot data as the edge weights
    * highlight certain nodes and edges using ``mark_nodes`` and ``mark_edges``
    * adjust the plot style and coloring
    * animate your network plot (see also the article on :ref:`animations <plot_animations>`).

.. admonition:: Complete example: Static network
    :class: dropdown

    .. literalinclude:: ../../../_cfg/Opinionet/graph_plot/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- static_network
        :end-before: ### End --- static_network

.. admonition:: Complete example: Animated network
    :class: dropdown

    .. literalinclude:: ../../../_cfg/Opinionet/graph_plot/eval.yml
        :language: yaml
        :dedent: 0
        :start-after: ### Start --- animated_network
        :end-before: ### End --- animated_network

The ``.plot.graph`` plot function combines and extends the following four NetworkX plotting utilities:

* :py:func:`~networkx.drawing.nx_pylab.draw_networkx_nodes`
* :py:func:`~networkx.drawing.nx_pylab.draw_networkx_edges`
* :py:func:`~networkx.drawing.nx_pylab.draw_networkx_labels`
* :py:func:`~networkx.drawing.nx_pylab.draw_networkx_edge_labels`

It preserves the full configurability of the above functions while offering additional features, e.g. plotting directly from a :py:class:`~utopya.eval.groups.GraphGroup`, automatically mapping data to layout properties, or creating graph animations.


Example
^^^^^^^
Here is an example graph plot, using the  :ref:`Utopia Opinionet model <model_Opinionet>`:

.. image:: ../../../_static/_gen/Opinionet/graph_plot/static_network.pdf
    :width: 800
    :alt: Example graph plot

Let's go through the configuration step-by-step:

First, base the plot on ``.creator.universe`` (this is a universe plot) and ``.plot.graph`` (the default :py:func:`plot function for graphs <utopya.eval.plots.graph.draw_graph>`).
Then, select the :py:class:`~utopya.eval.groups.GraphGroup` from your model: in the case of Opinionet, this is the ``nw`` :py:class:`~utopya.eval.groups.GraphGroup`.
Lastly, specify the time at which you wish to show the network:

.. code-block:: yaml

  graph:
    based_on:
      - .creator.universe
      - .plot.graph

    select:
      graph_group: nw   # Adjust this to your own model's case

    graph_creation:
      at_time_idx: -1   # This can be any time

This is already enough to plot a simple graph:

.. image:: ../../../_static/_gen/Opinionet/graph_plot/graph_simple.pdf
    :width: 800
    :alt: Simple graph plot


Plotting data as node and edge properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Next, let's select the properties you wish to use for the graph plot.
In our case, we plot the network at the final time step of the model, and want both the user ``opinion`` property, as well as the ``edge_weight`` property to be shown in the plot.
Add the following entries to the ``graph_creation`` entry above:

.. code-block:: yaml

    graph:

      # Everything as above ...

      graph_creation:
        at_time_idx: -1 # as above
        node_props: [opinion]
        edge_props: [edge_weights]

If you only wish to plot node properties *or* edge properties, you can drop the irrelevant entry.
Now plot these properties on the graph using the ``graph_drawing`` key:

.. code-block:: yaml

  graph:

    # Everything as above ...

    graph_drawing:
      nodes:
        node_color:
          from_property: opinion
        node_size:
          from_property: degree
      edges:
        width:
          from_property: edge_weights

This plots the ``opinion`` as the node color and the node degree as its size, and the ``edge_weights`` as the edge width:

.. image:: ../../../_static/_gen/Opinionet/graph_plot/graph_with_props.pdf
    :width: 800
    :alt: Graph plot with some properties

Changing the layout and appearance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Let's modify the graph's appearance.
The arrows are a bit large, so let's reduce the ``arrowsize``.
The nodes in the above plot are a little small: let's scale them up.
To do this, add the following entry to ``nodes/node_size`` in the above configuration:

.. code-block:: yaml

    graph:

      # Everything as before ...

      graph_drawing:
        nodes:
          node_size:
            from_property: degree
            scale_to_interval: [1, 200]
        edges:
          # .. as above ..
          arrowsize: 4

This will scale the node size to a value between the interval you pass.
The same can be done for the edge widths.

NetworkX offers a large selection of `drawing models <https://networkx.org/documentation/stable/reference/drawing.html#see-also>`_ for visualising a graph, which you can control from the config use the ``positions`` key:

.. code-block:: yaml

    graph:
      graph_drawing:
        positions:
          model: spring
          k: 2

.. image:: ../../../_static/_gen/Opinionet/graph_plot/graph_pretty.pdf
    :width: 800
    :alt: Graph plot with positions according to spring model

The above example uses the :py:func:`~networkx.drawing.layout.spring_layout` with two passes (``k: 2``) to adjust the node distances.
You could also do

.. code-block:: yaml

  graph:
    graph_drawing:
      positions:
        model: circular

to place the nodes on a circle using :py:func:`~networkx.drawing.layout.circular_layout`.

.. admonition:: Fixing the seed

    If you don't fix a random seed, the network may look different every time you plot it, depending on the layout you choose.
    You can resolve this by setting the ``seed`` parameter.

    For example, when using the spring layout, do

    .. code-block:: yaml

      graph:
        graph_drawing:
          positions:
            model: spring
            k: 2
            seed: 42

.. hint::

    If you have `pygraphviz <https://github.com/pygraphviz/pygraphviz>`_ installed, the ``graphviz`` layouts become available.
    Simply specify ``model: graphviz_dot`` to use ``dot`` for layouting; the other `graphviz <https://graphviz.org>`_ layouting programs are also available.


Color settings
^^^^^^^^^^^^^^
Let us change the colormap used for the nodes, and add a label to the colorbar; in the ``graph_drawing/nodes`` entry above, add the following:

.. code-block:: yaml

    graph:
    # Everything as above ...
      graph_drawing:
        nodes:
          # Everything as above ...
          cmap:
            continuous: true
            from_values:
              0: crimson
              0.5: gold
              1: dodgerblue
          vmin: 0.
          vmax: 1.0

          colorbar:
            label: opinion $\sigma$

The ``cmap`` entry makes use of the
`ColorManager <https://dantro.readthedocs.io/en/latest/plotting/plot_functions.html#colormanager-integration>`_, and offers a wide range of capabilities, including using different norms; take a look at the :ref:`style section <colormaps>` for more details.

The colorbar is automatically shown whenever a property mapping was done for the node colors.
You can turn it off by setting

.. code-block:: yaml

    colorbar:
      enabled: false

You can also change location, size, or labels of the colorbar:

.. code-block:: yaml

    colorbar:
      labels:
        0: left
        0.5: center
        1: right
      shrink: .5
      aspect: 10
      orientation: horizontal

Taken together, all these changes generate a plot like this:

.. image:: ../../../_static/_gen/Opinionet/graph_plot/graph_colorbar.pdf
    :width: 800
    :alt: Styled-up graph plot with custom colormap and colorbar

.. note::

    If you have all ``node_props`` or ``edge_props`` in *one* container (HDF5 dataset) respectively, there are two possibilities to make subselections in the containers:

    1. When you only use *either* node *or* edge properties, or you want to make the same subselection on *both* your node *and* your edge dataset, you can simply do a ``.sel: {property: my_prop}``, where ``property`` is your dataset dimension, and ``my_prop`` the coordinate, or you can do the respective ``.isel``

    2. When you need to perform different subselections on your node and edge datasets—say you want node betweenness centrality and edge weight—you need to specify them via the DAG's select interface, and the procedure needs to include the following steps:

    .. code-block:: yaml

        select:
          graph_group: g_static
          betw:
            path: network/vertex_metrics
            transform:
              - .sel: [!dag_prev , {property: betweenness}]
              - .squeeze_with_drop
          wei:
            path: network/edge_properties
            transform:
              - .sel: [!dag_prev , {property: weight}]
              - .squeeze_with_drop

        compute_only: [graph_group, betw, wei]

        # Make the selected tags available as property maps
        register_property_maps:
          - betw
          - wei

        # clear_existing_property_maps: false

        graph_creation:
          at_time_idx: 0
          edge_props: [wei]
          node_props: [betw]
          # sel: {time: 0}    # applied to both properties

        graph_drawing:
          edges:
            edge_color: k
            width:
              from_property: wei
              scale_to_interval: [0.1, 2.0]
          nodes:
            node_color:
              from_property: betw

See :py:func:`~utopya.eval.plots.graph.draw_graph` and :py:mod:`utopya.eval.plots.graph` for detailed interface information.
For detailed descriptions of the networkx plot functions that are used here, refer to the `NetworkX docs <https://networkx.github.io/documentation/stable/reference/drawing.html>`_.


Add labels and highlights
^^^^^^^^^^^^^^^^^^^^^^^^^
You can add node and edge labels by adding the ``node_labels`` and ``edge_labels`` keys to the ``graph_drawing`` entry:

.. code-block:: yaml

    graph:

      # Everything as above ...

      graph_drawing:
        # Everything as above ...
        # Just add these two entries:
        node_labels:
          enabled: True
        edge_labels:
          enabled: True

Labels need to be explicity enabled.
Labelling *all* nodes and edges may crowd the plot, so let's only label some of the nodes:

.. code-block:: yaml

    graph:

      # Everything as above ...

      graph_drawing:
        # Everything as above ...
        # Just add these two entries:
        node_labels:
          enabled: True
          labels:
            0: 'node 0'
            1: 'node 1'
        edge_labels:
          enabled: True
          edge_labels:
            [0, 1]: "(0, 1)"
            [1, 3]: "(1, 3)"

We can also highlight certain nodes and edges, to create a plot that looks like this:

.. image:: ../../../_static/_gen/Opinionet/graph_plot/graph_highlighted.pdf
    :width: 800
    :alt: A graph with highlights

To do this, use the ``graph_drawing/mark_nodes`` and ``graph_drawing/mark_edges`` keys.
You can also reduce the transparency of the unmarked edges to make the marked ones more prominent:

.. code-block:: yaml

    graph_highlighted:

      # Everything as above ..

      graph_drawing:
        nodes:
          # As before ...

        # Reduce the transparency of the edge weights
        edges:
          edge_color: [0, 0, 0, 0.05]
          width:
            from_property: edge_weights
            scale_to_interval: [0, 1]

        # Add labels for the nodes in the path
        node_labels:
          enabled: True
          show_only: &nodelist !range [2, 12]  # [2, 3, .., 11]

        # Mark the nodes in the nodelist
        mark_nodes:
          nodelist: *nodelist
          color: crimson

        # Mark some edges (will be ignored if they do not exist)
        mark_edges:
          colors:
            [24, 11]: crimson
            [18, 15]: crimson
            [49, 15]: crimson
            # add more edges to mark here ...



Animations
^^^^^^^^^^

.. raw:: html

    <video width="800" src="../../../_static/_gen/Opinionet/graph_plot/animated_network.mp4" controls></video>


You can animate your network plot by also basing your plot on one of the base animation functions (e.g. ``.animation.ffmpeg``) and adding a ``graph_animation`` entry to your configuration:

.. code-block:: yaml

    animated_network:
      based_on:
        - .creator.universe
        - .plot.graph
        - .animation.ffmpeg  # Use the ffmpeg writer

      # Everything else as above.

      # Just add this entry to make the 'opinion' change over time:
      graph_animation:
        sel:
          time:
            from_property: opinion

We discuss animations in more detail on the :ref:`animations page <plot_animations>`, including how to increase the animation resolutions.
