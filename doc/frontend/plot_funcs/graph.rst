Graph Plots
===========

.. automodule:: utopya.plot_funcs.dag.graph

For detailed descriptions of the networkx plot functions that are used here, have a look at the `networkx docs <https://networkx.github.io/documentation/stable/reference/drawing.html>`_.

----

Draw Graph
----------

.. autofunction:: utopya.plot_funcs.dag.graph.draw_graph

An extended overview of the ``draw_graph`` configuration:

.. code-block:: yaml

    ---
    my_graph_plot:
      based_on: .dag.graph

      select:
        graph_group: path/to/your/GraphGroup
        some_ext_node_prop: path/to/external/node/property/data

      # It is also possible to load external property data. The dag tags (from the dag
      # selection above) listed in 'register_property_maps' will be registered in the
      # graph group, and hence be available as property data.
      register_property_maps: ['some_ext_node_prop']
    
      # The 'graph_cfg' dict contains all configurations used for the creation, drawing,
      # and animation of the graph.
      graph_cfg:
        graph_creation:                     # The graph_creation configurations are passed to
          at_time: 42                       # 'GraphGroup.create_graph'. Here, you can, e.g.,
          node_props:                       # set node and edge properties.
            - 'foo'
            - 'bar'
            - 'some_ext_node_prop'
          edge_props: ['baz']
        graph_drawing:                      # 'graph_drawing' contains all layout configs.
          positions:                        
            model: spring                   # Here, you can specify the node positioning
                                            # model to be used. The spring model reduces
                                            # overall edge lengths.
            k: 1                            # The spring model can mainly be tweaked by
            iterations: 100                 # changing the optimal edge length ('k') and the
                                            # maximum number of iterations taken to determine
                                            # the node positions.
          
          # The 'select' interface allows to only plot a subgraph which is created from a
          # specified set of nodes.
          select:
            nodes: !listgen [0, 42]         # The nodes can *either* be specified directly,
            radial: True                    # *or* by enabling radial selection. For that,
            center: 0                       # the center node and the radius are required.
            radius: 3

          # The four entries below configure the layout of nodes, edge, and labels. After
          # doing property mapping, they are passed to the matching networkx plot functions.
          # Have a look at the networkx documentation of the respective plot functions for
          # a complete list of kwargs that can be specified:
          # https://networkx.github.io/documentation/stable/reference/drawing.html
          nodes:                            # Passed to 'draw_networkx_nodes'
            node_color: orange
            node_size:                      # Here, the 'foo' property is mapped to the size
                                            # of the nodes.
              from_property: foo            # Use 'from_property' to specify the property to
                                            # map from. Additional to the stored node
                                            # properties this can be set to 'degree',
                                            # 'in_degree', or 'out_degree'.
              scale_to_interval: [100, 400] # Additionally, the property values can be scaled
                                            # linearly to a given interval (the default node
                                            # size is 300).
          edges:                            # Passed to 'draw_networkx_edges'
            edge_color:                     # The 'baz' property is mapped to the edge color.
              from_property: baz

          # Node and edge labels are disabled by default. They can be enabled using the
          # 'enabled' key. If enabled, the default labels for the nodes and edges are their
          # indices and index pairs, respectively.
          node_labels:                      # Passed to 'draw_networkx_labels'
            enabled: True
            labels:                         # The 'labels' kwarg is a dict of labels keyed by
              0: label0                     # node. Only the labels in this dictionary are
              1: label1                     # plotted.
              2: label2
            show_only: [0, 1]               # 'show_only' allows to modify the 'labels' dict
                                            # such that *only* the nodes in the given list
                                            # are labeled. All entries in 'labels' for which
                                            # the key is not in 'show_only' are removed.
                                            # Nodes that are in 'show_only' but are not a key
                                            # in 'labels' are labeled by their index.
          edge_labels:                      # Passed to 'draw_networkx_edge_labels'
            enabled: True
            edge_labels:                    # Why is this kwarg called 'edge_labels' for the
              [0,1]: label01                # edges and 'labels' for the nodes? We certainly
              [1,3]: label13                # don't know...

        # The 'graph_animation' specs are only taken into account if an animation is done.
        graph_animation:
          times:                            # Each time step represents one animation frame.
                                            # The times can be specified in 3 different ways:
            from_property: 'foo'            # Option 1: Extract all times from the time
                                            # coords of the given node/edge property.
            sel: [0, 10, 20, ..., 100]      # Option 2: Select the times explicitly
            isel: [0, 1, 2, ..., 10]        # Option 3: Select the times by index.

.. note::

    Graph animations are only possible for fixed node positions.
    When performing an animation, the node positions and colorbar(s) are fixed
    to the ones generated given the specified ``graph_cfg``.

.. note::

    When performing an animation, the values in ``graph_animation[times]`` are shown as suptitle by default. As the actual times are only shown when using ``from_property`` or ``sel``, we recommend to use either of those for specifying the animation times.