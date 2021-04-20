.. _impl_graph:

Graphs
======

Utopia makes use of the `Boost Graph Library <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/index.html>`_ to implement graph-related features. The following will help you create and use graphs in your model.

.. contents::
   :local:
   :depth: 2

----

.. _create_graphs:

Creating Graph Objects
----------------------

Utopia provides the ``create_graph`` functionality to easily construct a graph. Assuming we are inside a Utopia model class, such that we have access to a model configuration (``this->_cfg``) and a random number generator (``*this->_rng``), here is an example of how you could construct an undirected graph:

.. code-block:: c++

    #include <utopia/core/graph.hh>
    
    // ...
    
    /// The vertex container type
    using VertexContainer = boost::vecS;

    /// The edge container type
    using EdgeContainer = boost::vecS;

    /// The graph type (here: undirected)
    using Graph = boost::adjacency_list<
        EdgeContainer,
        VertexContainer,
        boost::undirectedS>;
    
    // ..
    
    /// Inside the model class: initialize its graph member
    Graph g = create_graph<Graph>(this->_cfg["my_graph"], *this->_rng);
    
Here ``cfg["my_graph"]`` points to the relevant configuration entry from the model configuration file. It could look like this:

.. code-block:: yaml

    my_graph:
 
      # graph model
      model: ErdosRenyi  # (a random graph)
      num_vertices: 200
      mean_degree: 4

      ErdosRenyi:
        parallel: False
        self_edges: False

The ``model`` key tells Utopia which graph generation algorithm you wish to use, for which there are several currently available options. Each model key requires its own parameters for the graph generation algorithm – see :ref:`below <graph_gen_functions>` for more details.

.. note::

    Depending on your needs, you will need to use either ``boost::undirectedS`` or ``boost::directedS``/``boost::bidirectionalS`` in the adjacency matrix of your graph.

.. hint::
    
    Boost allows you to use different container types for vertices and edges, e.g. ``boost::vecS`` or ``boost::listS``, each optimised for different purposes. See `the boost graph documentation entry <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/using_adjacency_list.html#sec:choosing-graph-type>`_ for more details.

.. _vertex_and_edge_propertymaps:

Attaching custom objects to graph vertices and edges
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Oftentimes it is desirable to attach custom objects to vertices and edges of your graph (or, more precisely: the ``boost::adjacency_list``), e.g. to store a custom vertex state. In the following example, the structs ``Vertex`` and ``Edge`` are applied to an ``adjacency_list``. If you access e.g. a vertex with a ``vertex_descriptor vd`` such as  ``g[vd]``, an object of type ``Vertex`` is returned, which holds the properties ``id`` and ``state``. Introducing properties like this provides an easy way to set up and access internal properties. For further information, see the `bundled property documentation <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/bundles.html>`_ .

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

.. _graph_gen_functions:

Graph-generating functions in Utopia
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Utopia contains a selection of graph creation algorithms. What is more, the
``Utopia::Graph::create_graph()`` function lets you easily switch between
different graph creation models.

Let's assume that we have defined a graph type as done in the previous section.
Let's again further assume that we are inside a Utopia model class, such that we
have access to a model configuration (``this->_cfg``) and a random number
generator (``*this->_rng``). (Of course, you would need to adapt these two
options if you use the function in another situation.)
In this case, we can just write:

.. code-block:: c++

    Graph g = create_graph<Graph>(
                this->_cfg["my_graph"],     // Your model configuration file
                                            // requires a "my_graph"
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

.. warning::

    When generating scale-free networks, you *must* use ``BarabasiAlbert`` for undirected and ``BollobasRiordan`` for directed graphs.

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
graph are derived from a  `Utopia::Entity <../../../doxygen/html/group___graph_entity.html>`_.
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



Modifying the graph structure while looping over a graph entity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When looping over a graph and simultaneously modifying its structure, you need to be really careful, because the iterators you use to loop over the graph can easily be invalidated.
Have a look at the `boost graph documentation <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/adjacency_list.html>`_ for further information.

As a rule of thumb: If you want to change the graph structure ...

- use ``boost::listS`` as the appropriate list and edge containers and
- do *not* use the ``apply_rule`` functions because they can easily result in buggy behavior.


Saving Graph Data
-----------------
Utopia provides an interface to save ``boost::adjacency_list``s or custom properties attached to it. In a first step, it is highly recommended to set up a separate ``HDFGroup`` for the graph data using the ``create_graph_group`` function. It automatically adds some metadata as group attributes that allows to conveniently load the data as a ``GraphGroup`` on the frontent side.

Saving a static graph
^^^^^^^^^^^^^^^^^^^^^
If you want to save *only* the topology of a graph, you can use the ``save_graph`` functionality to write the vertices and edges to HDF5:

.. literalinclude:: ../../../test/data_io/graph_utils_doc_test.cc
    :language: c++
    :start-after: // DOC REFERENCE START: save_graph example
    :end-before: // DOC REFERENCE END: save_graph example

.. hint::

    The ``save_graph`` function as used in the example above uses ``boost::vertex_index_t`` to retreive the vertex indices. However, custom vertex ids can be used by additionaly passing a PropertyMap of vertex ids. This would be needed, for example, when using a ``boost::listS`` VertexContainer.

.. note::

    If using a VertexContainer (EdgeContainer) type that does not provide a fixed internal ordering and if additionaly saving vertex (edge) properties, then the vertices and edges should be stored alongside the latter dynamically. For more details see :ref:`below <save_graph_properties>`


.. _save_graph_properties:

Saving vertex and edge properties dynamically
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
With the ``save_vertex_properties`` and ``save_edge_properties`` functions Utopia provides an interface for dynamic saving of graph data accessible via the vertex descriptors and edge descriptors, respectively.

When should you use this interface?
* Not all VertexContainer and EdgeContainer types provide a fixed internal ordering. For example ``boost::vecS`` does, whereas ``boost::listS`` does not. In case a fixed ordering is not given: If you want to save multiple properties or if you want to save property data alongside the graph topology, all of that data should be written using this dynamic interface. **Correct alignment** of the data is guaranteed since all data is written in a single loop over the graph.
* If in your model the number of vertices and edges changes over time it is difficult to initialize appropriate datasets as the size is not known. Thus, for **dynamic graphs** the dynamic interface should be used which circumvents this problem by creating a new dataset for each time step.

Let's look at an example: Let's assume we have a graph where each vertex holds a ``some_prop`` property and we want to save the graph topology and property data. For that we need to provide tuples of tuples, each of the latter containing a property *name* and the respective *adaptor*. The adaptor is a lambda function returning the property data given a vertex or edge descriptor and a reference to the graph. In order to write 2D data (here: for the edge data), multiple such adaptors must be passed (one for each coordinate) as well as the name of the second dimension.

.. literalinclude:: ../../../test/data_io/graph_utils_doc_test.cc
    :language: c++
    :start-after: // DOC REFERENCE START: setup_adaptor_tuples example
    :end-before: // DOC REFERENCE END: setup_adaptor_tuples example

Note that while here we simply extract the properties from the vertices (edges), in general the adaptors can contain any calculation on ``vd`` (``ed``).

These tuples can then be passed to the functions ``save_vertex_properties`` and ``save_edge_properties`` together with an ``adjacency_list``, a parent ``HDFGroup``, and a ``label``. The ``label`` will be used to distinguish the saved data and should be unique.
For example, if you write a graph at every time step the ``label`` should encode the time at which the graph was written.

.. literalinclude:: ../../../test/data_io/graph_utils_doc_test.cc
    :language: c++
    :start-after: // DOC REFERENCE START: save_properties example
    :end-before: // DOC REFERENCE END: save_properties example

.. note::

    In order to guarantee correct alignment of the data, which is needed if you want to be able to associate properties with one another, the ``save_vertex_properties`` and ``save_edge_properties`` functions must only be called once per model update step.

.. _loading_a_graph_from_a_file:

Loading a Graph from a File
-----------------------------
Using real-world networks as a basis for the graph
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Using the ``create_graph`` config interface you can access the graph loading mechanism and do your simulations on airline networks, social networks, and anything that you are able to find out there. The usage is as simple as the following example:

.. code-block:: YAML

    create_graph:
      model: "load_from_file"
      load_from_file:
        base_dir: "~/Utopia/network-files"
        filename: "my_airlines_network.xml"
        format: "graphml" # or "graphviz"/"gv"/"dot"

.. warning::

    The loader only supports loading to ``boost``'s ``adjacency_list``, not to an ``adjacency_matrix``, as this is a bit more difficult.

    A workaround could be to copy the graph via `boosts copy graph functionality <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/copy_graph.html>`_ .

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

Loading edge weights (or some other property) into utopia
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you want to load edge weights, or some other *edge* property which is stored in GraphViz via

.. code-block:: DOT

  digraph "Directed Weighted Test GraphViz Graph" {
    1 -> 0  [weight=1, some_int=5];
    0 -> 2  [weight=1.5, some_int=5];
    1 -> 3  [weight=.5, some_int=4];
    1 -> 3  [weight=2.5];
    4;
  }

or in GraphML via

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

then you need to *pass a* `dynamic property map <https://www.boost.org/doc/libs/1_76_0/libs/property_map/doc/dynamic_property_map.html>`_ *to the create graph algorithm*. This can be done in two ways: either to ``boost``'s built-in ``boost:edge_weight``, or to a bundled property, for example a ``struct EdgeState``:


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
                          boost::vecS,         // edge container
                          boost::vecS,         // vertex container
                          boost::directedS,
                          boost::no_property,  // no vertex properties
                          boost::property<boost::edge_weight_t,
                                          double, EdgeState>
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

Now your graph should be loaded with the edge properties of your desire. If you find out how to read *vertex* properties, from the file, please let us know.

.. note::

    Make sure you load undirected graphs into undirected boost graphs and directed ones into directed ones. Conversions can be made both on the side of the file, and on boost's side, but for the reading process it must be coherent.

.. note::

    Whenever you use more than just one vertex/edge property, or more than just the bundle, the properties need to be nested, as in ``boost::property<boost::edge_weight_t, double, EdgeState>``. See the `documentation page on bundled properties <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/bundles.html>`_ for more examples.

.. warning::

    The loading into bundled edge properties does not work with the typical utopia graph entity specifications as yet, so ``using EdgeTraits = Utopia::GraphEntityTraits<EdgeState>;`` ``using Edge = GraphEntity<EdgeTraits>;``, limits you to ``boost``'s built-in properties, `which are quite a few actually <https://www.boost.org/doc/libs/1_76_0/libs/graph/doc/property.html>`_.
