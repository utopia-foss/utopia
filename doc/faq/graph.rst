Utopia Graph FAQs
=================

This part of the FAQ relates to the usage of
`Boost Graph Library
<https://www.boost.org/doc/libs/1_69_0/libs/graph/doc/index.html>`_ Features in
Utopia.

.. contents::
   :local:
   :depth: 1

----

How do I attach custom objects to the vertices and edges of my graph?
---------------------------------------------------------------------

Often times, it is desired to attach custom objects to vertices and edges of
your graph (or, more precisely: the ``boost::adjacency_list``), e.g.
to store a custom vertex state.


In the following example the structs ``Vertex`` and ``Edge`` are applied to an
``adjacency_list``. If you access e.g. a vertex with a ``vertex_descriptor vd``
like ``g[vd]`` an object of type ``Vertex`` is returned which holds the properties
``id`` and ``state``.
Introducing properties like this provides an easy way to set up and access 
internal properties. For further information, see the `bundled property documentation 
<https://www.boost.org/doc/libs/1_62_0/libs/graph/doc/bundles.html>`_ .

.. code-block:: c++

    struct Vertex {
        std::size_t id;
        int state;
    };

    struct Edge {
        double weight;
    };

    boost::adjacency_list<boost::vecS,        // edge container
                          boost::vecS,        // vertex container
                          boost::undirectedS,
                          Vertex,             // vertex struct
                          Edge>               // edge struct
                          graph;

.. note::

    If you construct the `adjacency_list` take care that the ordering of the 
    template arguments specifying the vertex/edge container type and their 
    struct is correct! For the container types you first have to specify the 
    edge container type and then the vertex container type whereas for the structs
    you first need to specify the vertex struct and then the edge struct.
    You wonder why this is? Actually, we too... :thinking:

How can values stored in vertex or edge objects be saved?
-------------------------------------

If you use a ``boost::adjacency_list`` with custom properties you might want to
save these properties to HDF5 in order to process the data later (e.g. plot
the graph structure).

In order to do so, Utopia provides the ``save_graph_properties`` function.
To save properties you have to pass the information how to access the
information. Therefore you can provide a tuple of pairs containing a ``name`` and
a lambda function. In the following example we want to save the properties ``id``
and ``state`` and provide two lambdas which extract these properties from an
arbitrary ``Vertex v``.
However, in general these lambdas can contain any calculation on ``Vertex v``.


.. code-block:: c++

    auto get_properties = std::make_tuple(
            std::make_tuple("id", [](auto& v){return v.id;}),
            std::make_tuple("state", [](auto& v){return v.state;})
    );


This tuple can be passed to the function ``save_graph_properties`` together with
an ``adjacency_list``, a parent ``HDFGroup`` and a ``label``. To determine if you
want to save a vertex or edge property the vertex or edge type (e.g. ``Vertex``
or ``Edge``) is provided via a template argument. Please keep in mind that this
type has to match the one contained in the graph type of ``g``.
The ``label`` will be used to distinguish the saved data and should be unique.
If you write a graph for example every time step, the ``label`` should encode the
time the graph was written.


.. code-block:: c++

    save_graph_properties<Vertex>(graph, grp, "graph0", get_properties);

The example code will result in the following structure (the graph
has 100 vertices):

.. code-block::

    └┬grp
       └┬ id
           └─ graph0         < ... shape(100,)
        ├ state
           └─ graph0         < ... shape(100,)

Supposing that you do not want to apply custom vertices or edges, or you want
to use functions that require a vertex or edge descriptor (e.g. `boost::source`),
you can call ``save_graph_properties`` with a ``vertex_descriptor`` or
``edge_descriptor`` type as template argument. However, you have to adapt the
tuple to use a ``descriptor``-type.
The following example saves the ``id`` of the source vertex for each edge as well
as its ``weight``. (``edge_dsc`` is the ``GraphType::edge_descriptor``)

.. code-block:: c++

    auto get_properties_desc = std::make_tuple(
            std::make_tuple("source",
                            [](auto& g, auto& ed){return g[source(ed, g)].id;}),
            std::make_tuple("weight",
                            [](auto& g, auto& ed){return g[ed].weight;})
    );

    save_graph_properties<edge_dsc>(graph, grp, "graph0", get_properties_desc);


If you use a container without ordering to save vertices and/or edges in your
graph as e.g. a ``boost::setS`` the ordering might differ within multiple calls
of ``save_graph_properties``. Thus, if you want to be able to associate a
property with another one (e.g. saved edges and their corresponding weight)
make sure to call ``save_graph_properties`` only once as the order is conserved
in a single call.