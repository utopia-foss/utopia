.. _faq_graph:

Graphs
======

This part of the FAQ relates to the usage of the `Boost Graph Library <https://www.boost.org/doc/libs/1_74_0/libs/graph/doc/index.html>`_ in Utopia.

.. contents::
   :local:
   :depth: 2

----

.. _create_graphs:

Creating Graph Objects
----------------------
How do I attach custom objects to the vertices and edges of my graph?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Oftentimes it is desirable to attach custom objects to vertices and edges of your graph (or, more precisely: the ``boost::adjacency_list``), e.g. to store a custom vertex state. In the following example, the structs ``Vertex`` and ``Edge`` are applied to an ``adjacency_list``. If you access e.g. a vertex with a ``vertex_descriptor vd`` such as  ``g[vd]``, an object of type ``Vertex`` is returned, which holds the properties ``id`` and ``state``. Introducing properties like this provides an easy way to set up and access internal properties. For further information, see the `bundled property documentation <https://www.boost.org/doc/libs/1_62_0/libs/graph/doc/bundles.html>`_ .

.. code-block:: c++

    /// The vertex struct
    struct Vertex {
        std::size_t id;
        int state;
    };

    /// The edge struct
    struct Edge {
        double weight;
    };

    /// The containter types
    using EdgeCont = boost::vecS;
    using VertexCont = boost::vecS;

    /// The graph type
    using Graph = boost::adjacency_list<EdgeCont,
                                        VertexCont,
                                        boost::undirectedS,
                                        Vertex,
                                        Edge>;

    // Construct a graph
    Graph g;

If you want to use Utopia's full graph functionality, we strongly recommend
that you define your graph as described in the section on
:ref:`applying rules on graph entities <apply_rule_graph>`.

.. warning::

    When constructing the ``adjacency_list``, take care that the order of the
    template arguments specifying the vertex/edge container type and their
    struct is correct! For the container types, you **first** need to specify the
    edge container type and **then** the vertex container type, whereas for the structs you **first** need to specify the vertex struct and **then** the edge struct.
    If you're wondering why this is — we are, too!


Are there graph-generating functions available in Utopia?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Yes. Utopia contains a selection of graph creation algorithms. What is more, the
``Utopia::Graph::create_graph()`` function lets you easily switch between
different graph creation models.

Let's assume that we have defined a graph type as done in the previous section.
Let's further assume that we are inside a Utopia model class, such that we
have access to a model configuration (``this->_cfg``) and a random number
generator (``*this->_rng``). (Of course, you would need to adapt these two
options if you use the function in another situation.)
In this case, we can just write:

.. code-block:: c++

    Graph g = create_graph<Graph>(
                this->_cfg["create_graph"], // You model configuration file
                                            // requires a "create_graph"
                                            // entry.
                *this->_rng
                );

This function is configured through a configuration node. The corresponding
YAML file looks like this:

.. code-block:: YAML

    create_graph:
      # The model to use for generating the graph. Valid options are:
      # "regular"           Create a k-regular graph. Vertices are located on a
      #                     circle and connected symmetrically to their nearest
      #                     neighbors until the desired degree is reached.
      #                     If the degree is uneven, then they are additionally
      #                     connected to the vertex at the opposite location.
      # "ErdosRenyi"        Create a random graph using the Erdös-Rényi model
      # "WattsStrogatz"     Create a small-world graph using the Watts-Strogatz
      #                     model (for directed graphs, rewiring of in_edges)
      # "BarabasiAlbert"    Create a scale-free graph using the Barabási-Albert
      #                     model (for undirected graphs)
      # "BollobasRiordan"   Create a scale-free graph using the Bollobas-Riordan
      #                     model (for directed graphs)
      model: "ErdosRenyi"

      # The number of vertices
      num_vertices: 1000

      # The mean degree (equals degree in regular model;
      #                  not relevant in BollobasRiordan model)
      mean_degree: 4

      # Model-specific parameters
      ErdosRenyi:
        # Allow parallel edges
        parallel: false

        # Allow self edges
        self_edges: false

      WattsStrogatz:
        # Rewiring probability
        p_rewire: 0.2

      BarabasiAlbert:
        # Allow parallel edges
        parallel: false

      BollobasRiordan:
        # Graph generating parameters
        alpha: 0.2
        beta: 0.8
        gamma: 0.
        del_in: 0.
        del_out: 0.5



This of course is a highly documented configuration.
You only need to specify configuration options if the creation algorithm you set requires them, otherwise they will be just ignored.




.. _apply_rule_graph:

Apply Rule Interface
--------------------

Utopia provides an interface to easily apply a rule to entities of
a graph. The user just needs to define a lambda function that takes one
graph entity descriptor as input and call the ``apply_rule`` function.
This is best described through examples:

.. literalinclude:: ../../../test/core/graph_apply_doc_test.cc
    :language: c++
    :start-after: // DOC REFERENCE START: apply_rule on graph entities examples
    :end-before: // DOC REFERENCE END: apply_rule on graph entities examples
    :dedent: 4

.. note::

    You can find the whole working and tested example, including all the references,
    in the file ``utopia/test/core/apply_rule_graph_doc_test.cc``.


Prerequisits on Graph, Vertex, and Edge Type
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Note that this functionality can only be used if the vertices and edges of the
graph are derived from a  `Utopia::Entity <../doxygen/html/group___graph_entity.html>`_.
Your definition of the graph needs to look like this:


.. literalinclude:: ../../../test/core/graph_apply_doc_test.cc
    :language: c++
    :start-after: // Below, an example of the required graph types - doc reference line
    :end-before: // End of the required graph types - doc reference line


This graph structure is similar to, though slighly more sophisticated than, the one described above in the section on :ref:`Graph Creation <create_graphs>`. In this graph definition, the vertex and edge property access works as follows:

.. code-block:: c++

    // Get the vertex property v_prop
    g[vertex].state.v_prop;

    // Get the edge property e_prop
    g[edge].state.e_prop;



Can I modify the graph structure while looping over a graph entity?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In general you can do it but you should be really careful because the iterators you use to loop over the graph can easily be invalidated.
Have a look at the `boost graph documentation <https://www.boost.org/doc/libs/1_74_0/libs/graph/doc/adjacency_list.html>`_ for further information.

As a rule of thumb: If you want to change the graph structure ...

- use ``boost::listS`` as the appropriate list and edge containers and
- do *not* use the ``apply_rule`` functions because they can easily result in buggy behavior.



.. _save_graph_properties:

Save Node and Edge Properties
-----------------------------
How can values stored in vertex or edge objects be saved?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you use a ``boost::adjacency_list`` with custom properties, you might want to save these properties to HDF5 in order to process the data later (e.g. plot the graph structure). To do so, Utopia provides the ``save_graph_properties`` function. To save properties, you need to pass information on how to access them by providing a tuple of pairs containing a ``name`` and a lambda function.

In the following example, we want to save the properties ``id`` and ``state`` and provide two lambdas which extract these properties from an arbitrary ``Vertex v``. However, in general these lambdas can contain any calculation on ``Vertex v``.


.. code-block:: c++

    auto get_properties = std::make_tuple(
            std::make_tuple("id", [](auto& v){return v.id;}),
            std::make_tuple("state", [](auto& v){return v.state;})
    );


This tuple can be passed to the function ``save_graph_properties`` together with
an ``adjacency_list``, a parent ``HDFGroup``, and a ``label``. To determine if you
want to save a vertex or edge property, the vertex or edge type (e.g. ``Vertex``
or ``Edge``) is provided via a template argument. Please keep in mind that this
type has to match the one contained in the graph type of ``g``.
The ``label`` will be used to distinguish the saved data and should be unique.
If you write a graph at, for example, every time step, the ``label`` should encode the time at which the graph was written.


.. code-block:: c++

    save_graph_properties<Vertex>(graph, grp, "graph0", get_properties);

The example code will result in the following structure (assuming the graph
has 100 vertices):

::

    └┬ grp
       └┬ id
           └─ graph0         < ... shape(100,)
        ├ state
           └─ graph0         < ... shape(100,)

Supposing that you do not want to apply custom vertices or edges, or you want
to use functions that require a vertex or edge descriptor (e.g. ``boost::source`),
you can call ``save_graph_properties`` with a ``vertex_descriptor`` or
``edge_descriptor`` type as template argument. However, you have to adapt the
tuple to use a ``descriptor``-type.
The following example saves the ``id`` of the source vertex for each edge as well
as its ``weight`` (``edge_dsc`` is the ``GraphType::edge_descriptor``):

.. code-block:: c++

    auto get_properties_desc = std::make_tuple(
            std::make_tuple("source",
                            [](auto& g, auto& ed){return g[source(ed, g)].id;}),
            std::make_tuple("weight",
                            [](auto& g, auto& ed){return g[ed].weight;})
    );

    save_graph_properties<edge_dsc>(graph, grp, "graph0", get_properties_desc);


If you use a container without ordering to save vertices and/or edges in your
graph as e.g. a ``boost::setS``, the ordering might differ within multiple calls
of ``save_graph_properties``. Thus, if you want to be able to associate a
property with another one (e.g. saved edges and their corresponding weight),
make sure to call ``save_graph_properties`` only once, as the order is conserved
in a single call.
