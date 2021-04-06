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
      # "load_from_file"    Create a graph from a given graphml or dot file
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

      load_from_file:
        base_dir: "~/Utopia/network-files"
        filename: "my_airlines_network.xml"
        format:"graphml" # or "graphviz"/"dot" (the same)



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

Loading a Graph from a File
-----------------------------
Can I also use real-world networks as a basis for the graph?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes you can! Via the ``create_graph`` config interface you can access the graph loading mechanism and do your simulations on airline networks, social networks, and anything that you are able to find out there. The usage is as simple as the following example:

.. code-block:: YAML

    create_graph:
      model: "load_from_file"
      load_from_file:
        base_dir: "~/Utopia/network-files"
        filename: "my_airlines_network.xml"
        format: "graphml" # or "graphviz"/"gv"/"dot"

.. warning::

    The loader only supports loading to ``boost``'s ``adjacency_list``, not to an ``adjacency_matrix``, as this is a bit more difficult.

    A workaround could be to copy the graph via `boosts copy graph functionality <https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/copy_graph.html>`_ .

The data that you load may need some curation before it works, though. As we are only loading the **graph's topology, without node/edge/graph attributes**, you might have to get rid of some data attributes, that can cause the loader to crash.

As far as we experienced it, attributes of the type ``vector_float`` or ``vector_string`` will cause the loader to break down. To avoid that, delete the declaration and every occurrence of the malicious attribute in your GraphML file. To make it illustrative, in the following GraphML file, you could use the regex ``^      <data key="key2.*\n`` to do a find and replace (by nothing) to remove the entire lines of the problematic ``key2``:


.. code-block:: XML

  <?xml version="1.0" encoding="UTF-8"?>

  <graphml>
    <key id="key1" for="node" attr.name="user_name" attr.type="string" />
    <key id="key2" for="node" attr.name="full_name" attr.type="vector_string" />

    <graph id="G" edgedefault="undirected" parse.nodeids="canonical" parse.edgeids="canonical" parse.order="nodesfirst">

      <node id="n1">
        <data key="key1">pia</data>
        <data key="key2">Iuto, Pia</data>
      </node>

      <node id="n2">
        <data key="key1">tro</data>
        <data key="key2">Dan, Tro</data>
      </node>
      ...
    </graph>
  </graphml>


.. note::

    If you encounter a format other than GraphML or Graphviz/DOT, be aware that there are a number of command line converters that might work right away: Try typing ``mm2gv``, ``gxl2gv``, or ``gml2gv``, to convert from the `MatrixMarket <https://math.nist.gov/MatrixMarket/formats.html#MMformat>`_, `GrapheXchangeLanguage <https://en.wikipedia.org/wiki/GXL>`_, and the `Graph Modeling Language <https://en.wikipedia.org/wiki/Graph_Modelling_Language>`_, respectively. If that doesn't help, check if `the Gephi Graph Exploration software <https://gephi.org/>`_ can maybe read the format, or otherwise pass it to python's `networkx <https://networkx.org/documentation/stable/reference/readwrite/index.html>`_  possibly by writing your own script. Both Gephi and networkx should be able to export to one of the accepted formats.

.. hint::

    One large database of real-world networks, that are available for download in the GraphML format, is `Netzschleuder <https://networks.skewed.de/>`_, another database, that uses the *MatrixMarket* ``.mtx`` format is `Network Repository <http://networkrepository.com>`_.

How do I load edge weights (or some other property) into utopia?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to load edge weights, or some other *edge* property, that is stored in GraphViz like

.. code-block:: DOT

  digraph "Directed Weighted Test GraphViz Graph" {
    1 -> 0  [weight=1, some_int=5];
    0 -> 2  [weight=1.5, some_int=5];
    1 -> 3  [weight=.5, some_int=4];
    1 -> 3  [weight=2.5];
    4;
  }

or in GraphML, similarly

.. code-block:: XML

  ...
  <graphml>

    <key id="key1" for="edge" attr.name="weight" attr.type="double" />
    <key id="key2" for="node" attr.name="some_int" attr.type="int" />

    <graph id="G" edgedefault="undirected">

      <node id="n1"> </node>
      <node id="n2"> </node>

      <edge id="e0" source="n1" target="n0">
        <data key="key1">2.</data>
        <data key="key2">5</data>
      </edge>

    </graph>
  </graphml>

then you need to *pass a* `dynamic property map <https://www.boost.org/doc/libs/1_75_0/libs/property_map/doc/dynamic_property_map.html>`_ *to the create graph algorithm*. This can be done in two ways: either to ``boost``'s built-in ``boost:edge_weight``, or to a bundled property, for example a ``struct EdgeState``:


.. code-block:: c++

  #include <boost/property_map/dynamic_property_map.hpp>
  #include <boost/graph/adjacency_list.hpp>
  #include "utopia/core/graph.hh"
  #include "utopia/data_io/graph_load.hh"

  struct EdgeState {
    double weight = 2.;
    int some_int = 1;
  };
  using GraphType = boost::adjacency_list<
                    boost::vecS,         // edge cont
                    boost::vecS,         // vertex cont
                    boost::directedS,
                    boost::no_property,  // no vertex properties
                    boost::property<boost::edge_weight_t, double, EdgeState>
                    >;
  GraphType g(0);

  // Now you need to define dynamic property maps
  boost::dynamic_properties pmaps(boost::ignore_other_properties);
  // Like this, it would simply ignore all properties in the file.
  // So now add a reference to the built-in or bundled weight as a source
  // for the weight pmap. To load to the bundle's `weight`, use:

  pmaps.property("weight", boost::get(&EdgeState::weight, g));
  // To load to boost's built_in `edge_weight`, use:
  // pmaps.property("weight", boost::get(boost::edge_weight, g));
  // You can also, if you want, load more edge properties, like:

  pmaps.property("some_int", boost::get(&EdgeState::some_int, g));

  // Now load the graph, either via `create_graph`, or `load_graph`
  g = Graph::create_graph<GraphType>(_cfg["create_graph"], *_rng, pmaps)
  // g = GraphLoad::load_graph<GraphType>(_cfg_cg["load_from_file"], pmaps);
  // where you need to pass the corresponding config nodes.

Now your graph should be loaded with the edge properties of your desire. If you find out how to include *vertex* properties, please let us know.

.. note::

    Make sure you load undirected graphs into undirected boost graphs and directed ones into directed ones. Conversions can be made both on the side of the file, and on boost's side, but for the reading process it must be coherent.

.. warning::

    The loading into bundled edge properties does not work with the typical utopia graph entity specifications as yet, so ``using EdgeTraits = Utopia::GraphEntityTraits<EdgeState>;`` ``using Edge = GraphEntity<EdgeTraits>;``, limits you to ``boost``'s built-in properties, `which are quite a few actually <https://www.boost.org/doc/libs/1_75_0/libs/graph/doc/property.html>`_.
