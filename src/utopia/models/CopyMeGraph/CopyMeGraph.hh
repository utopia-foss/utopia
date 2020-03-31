#ifndef UTOPIA_MODELS_COPYMEGRAPH_HH
#define UTOPIA_MODELS_COPYMEGRAPH_HH
// TODO Adjust above include guard (and at bottom of file)

// standard library includes
#include <random>
#include <iterator>

// third-party library includes
#include <boost/graph/adjacency_list.hpp>
#include <boost/range.hpp>

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/graph.hh>
#include <utopia/data_io/graph_utils.hh>


namespace Utopia::Models::CopyMeGraph {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ----------------------------------------------------------------------------
// -- General Information on choosing the adequate graph for your model -------
//    (Delete after reading, thinking and making choices ... )
//
// GRAPH CHOICE: You should think about what kind of graph you want to have.
//       Often, it is a good choise to have the graph represented as an
//       adjacency list ( https://www.boost.org/doc/libs/1_60_0/libs/graph/doc/adjacency_list.html ). 
//       However, there are also other viable options such an adjacency_matrix.
//       If you are unsure, just have a look in the boost::graph documentation
//       (quick googling helps you ;) )
//
// CONTAINER CHOICE: Depending on what you want to do in your model, you should
//       think about the adequate choice of container to store the vertices 
//       and edges in. You find again helpful information in the boost graph
//       documentation (e.g. https://www.boost.org/doc/libs/master/libs/graph/doc/using_adjacency_list.html ).
//       As a simple and basic rule of thumb you should choose a vector
//       (boost::vecS) if you have a lot of random accesses to you vertices 
//       or edges and a list (boost::listS) if you manipulate your entities 
//       often e.g. if you want to add and remove a lot of vertices/edges.
//
// DATA WRITING CHOICE: You should also think about how the data will be
//       written. If the number of vertices and edges changes over time, it is
//       difficult to initialize appropriate datasets as the size is not known.
//       Luckily, there are solutions for both the static case (graph topology
//       is constant over time) and the dynamic case (changing topology). Below,
//       examples for both cases are shown, denoted by '*_static' and
//       '*_dynamic', respectively. Don't forget to delete the parts irrelevant
//       for your model. Also note that the 'dynamic' case refers to the case
//       of changing vertex number AND edge number. Possibly, a mixed solution
//       might work best in your case. Since the data writing is slower in the
//       dynamic case, it is recommended to stick with the static case wherever
//       possible.
// ----------------------------------------------------------------------------

// -- Vertex ------------------------------------------------------------------
/// The vertex state 
/** Here you should add your vertex properties as in the examples below.
 *  And do not forget to delete this comment otherwise everyone will now 
 *  that you do not read commments. ;)
 */
struct VertexState {
    /// A useful documentation string
    double some_state;

    /// Another useful documentation string, yeah
    int some_trait;

    /// Whether this vertex is very important
    bool is_a_vip_vertex;

    // Add your vertex parameters here.
    // ...
};

/// The traits of a vertex are just the traits of a graph entity
using VertexTraits = Utopia::GraphEntityTraits<VertexState>;

/// A vertex is a graph entity with vertex traits
using Vertex = GraphEntity<VertexTraits>;

/// The vertex container type
/** Here, you select in which container your vertices should be stored in.
 *  Common choices are `boost::vecS` or `boost::listS`. 
 */
using VertexContainer = boost::vecS;

// -- Edge --------------------------------------------------------------------
/// The edge state
/** Here you should add your edge properties as in the examples below.
 *  And again do not forget to delete this comment otherwise everyone will now 
 *  that you do not read commments. ;)
 */
struct EdgeState {
    /// Every parameter should have useful document :)
    double weight;

    // Add your edge parameters here.
    // ...
};

/// The traits of an edge are just the traits of a graph entity
using EdgeTraits = Utopia::GraphEntityTraits<EdgeState>;

/// An edge is a graph entity with edge traits
using Edge = GraphEntity<EdgeTraits>;

/// The edge container type
/** Here, you select in which container your edges should be stored in.
 *  Common choices are `boost::vecS` or `boost::listS`. 
 */
using EdgeContainer = boost::listS;


// -- Graph -------------------------------------------------------------------
/// The type of the graph
/** By providing the structs that contain all properties of a vertex or edge 
 *  respectively, you take advantage of boost::graph's bundled properties
 *  (google it, if not known). This fascilitates setting and accessing
 *  vertex and edge properties. :)
 *  
 *  \warning As you can see in the graph type declaration below, when you 
 *           declare a boost::adjacency_list you first have to specify the 
 *           container type of the edges and then the container type of the 
 *           vertices. However, when you specify the structs containing the 
 *           properties of graph entities the order is vise versa. Why is this 
 *           the case? We don't now... 
 *           But take care that you have the correct order...
 */
using GraphType =
    boost::adjacency_list<EdgeContainer,
                          VertexContainer,
                          boost::undirectedS, // -> undirected graph
                          Vertex,
                          Edge>;

/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The CopyMeGraph Model; a good start for a graph-based model
/** TODO Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
class CopyMeGraph:
    public Model<CopyMeGraph, ModelTypes>
{
public:
    /// The type of the Model base class of this derived class
    using Base = Model<CopyMeGraph, ModelTypes>;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;
    
    // .. Graph-related types and rule types ..................................
    /// Data type for a vertex descriptor
    using VertexDesc = 
        typename boost::graph_traits<GraphType>::vertex_descriptor;

    /// Data type for an edge descriptor
    using EdgeDesc =
        typename boost::graph_traits<GraphType>::edge_descriptor;

    /// Data type for a rule function operating on vertices returning void
    using VertexVoidRule =
        typename std::function<void(VertexDesc, GraphType&)>;

    /// Data type for a rule function operating on vertices returning a state
    using VertexStateRule = 
        typename std::function<VertexState(VertexDesc, GraphType&)>;

    /// Data type for a rule function operating on edges returning void
    using EdgeVoidRule =
        typename std::function<void(EdgeDesc, GraphType&)>;

    /// Data type for a rule function operating on edges returning a state
    using EdgeStateRule = 
        typename std::function<EdgeState(EdgeDesc, GraphType&)>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// A re-usable uniform real distribution to evaluate probabilities
    std::uniform_real_distribution<double> _prob_distr;
    
    /// The graph
    GraphType _g;

    /// Some parameter
    double _some_parameter;

    // More parameters here ...


    // .. Temporary objects ...................................................

    
    // .. Datagroups ..........................................................
    // Use datagroups to structure and organize your data. In this case, it
    // makes sense creating one datagroup for the graph and all data which is
    // directly related to it. If you have multiple graphs, create one group
    // for each graph.
    // 
    // NOTE They should be named '_dgrp_<groupname>', where <groupname> is the
    //      datagroup's actual name as set in its constructor.
    /// A datagroup for a static graph
    std::shared_ptr<DataGroup> _dgrp_g_static;

    /// A datagroup for a dynamic graph
    std::shared_ptr<DataGroup> _dgrp_g_dynamic;

    // .. Datasets ............................................................
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in its constructor.
    /// A dataset for storing all vertices' some_state
    std::shared_ptr<DataSet> _dset_some_state;

    /// A dataset for storing all vertices' some_trait
    std::shared_ptr<DataSet> _dset_some_trait;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the CopyMeGraph model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    CopyMeGraph (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the uniform real distribution to range [0., 1.]
        _prob_distr(0., 1.),

        // Now initialize the graph
        _g{initialize_graph()},

        // Initialize model parameters
        _some_parameter(get_as<double>("some_parameter", this->_cfg)),
        // ...

        // Datagroups
        // For saving graph-related data, it is recommended to use a so-called
        // graph group. This is a normal datagroup (opened with the open_group
        // method) but with additional attributes such as information about the
        // graph itself. This allows for later loading the data from the graph
        // group directly into a NetworkX graph object in Python.
        _dgrp_g_static(create_graph_group(_g, this->_hdfgrp, "g_static")),
        _dgrp_g_dynamic(create_graph_group(_g, this->_hdfgrp, "g_dynamic")),

        // Datasets
        // For setting up datasets that store Graph data, you can use the
        // Model::create_dset helper method which already takes care of using
        // the correct length into the time dimension (depending on the
        // num_steps and write_every parameters). Additionally, the parent
        // datagroup can be specified. The syntax is:
        //
        // this->create_dset("a_number", dgrp, {})      // 1D {#writes}
        // this->create_dset("a_vec", dgrp, {num_cols}) // 2D {#writes, #cols}
        //
        // This is also done here for storing two vertex properties in the
        // static graph group.
        _dset_some_state(this->create_dset("some_state", _dgrp_g_static,
                                           {boost::num_vertices(_g)})),
        _dset_some_trait(this->create_dset("some_trait", _dgrp_g_static,
                                           {boost::num_vertices(_g)}))
    {
        // Can do remaining initialization steps here ...

        // Mark the datasets as vertex properties and set the second dimension
        // name to 'vertex_idx'. The first dimension's name is 'time'.
        // Also, specify the coordinates for the vertex_idx dimension, which
        // are just the trivial coordinates 0, ..., N-1, where N is the number
        // of vertices.
        // NOTE The IDs of the vertices do not necessarily line up with the
        //      indices of the vertices when iterating over the graph. That's
        //      why the dimension is called vertex_idx, not vertex_id.
        _dset_some_state->add_attribute("is_vertex_property", true);
        _dset_some_state->add_attribute("dim_name__1", "vertex_idx");
        _dset_some_state->add_attribute("coords_mode__vertex_idx", "trivial");

        _dset_some_trait->add_attribute("is_vertex_property", true);
        _dset_some_trait->add_attribute("dim_name__1", "vertex_idx");
        _dset_some_state->add_attribute("coords_mode__vertex_idx", "trivial");

        // NOTE The initial state need and should NOT be written here. The
        //      write_data method is invoked first at time `write_start`.
        //      However, this is a good place to store data that is constant
        //      during the run and needs to be written at some point.

        // In the case of a static graph, the topology can be saved once using
        // the DataIO::save_graph function. It saves the graph structure to the
        // specified datagroup by writing the vertex id's in a dataset named
        // `_vertices` and the edges in a dataset named `_edges`.
        save_graph(_g, _dgrp_g_static);

        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);
    }


private:
    // .. Setup functions .....................................................

    /// Initialize the graph
    GraphType initialize_graph () {
        this->_log->debug("Create and initialize the graph ...");

        auto g = Graph::create_graph<GraphType>(this->_cfg["create_graph"], 
                                               *this->_rng);

        this->initialize_vertices(g);
        this->initialize_edges(g);

        return g;
    }
    
    void initialize_vertices (GraphType& g) {
        // Define a rule that acts on a vertex
        auto initialize_vertex = [this](const auto v, auto& g){
            g[v].state.some_state = get_as<double>("init_some_state", this->_cfg);
            g[v].state.some_trait = get_as<int>("init_some_trait", this->_cfg);
            
            // Every 13th vertex (on average) is a VIP vertex
            if (this->_prob_distr(*this->_rng) < (1./13.)) {
                g[v].state.is_a_vip_vertex = true;
            }
            else{
                g[v].state.is_a_vip_vertex = false;
            }
        };

        // Apply the rule to all vertices
        apply_rule<IterateOver::vertices, Update::async, Shuffle::off>
            (initialize_vertex, g);
    }

    void initialize_edges (GraphType& g) {
        // Define a rule that acts on an edge
        auto initialize_edge = [this](const auto e, auto& g){
            // Get the initial weight from the configuration
            g[e].state.weight = get_as<double>("init_weight", this->_cfg);

            // If set in the configuration randomize the weight by
            // multiplying a random number between drawn uniformaly from [0,1].
            if (get_as<bool>("init_random_weight", this->_cfg)){
                // Here you see, how to generate a random number using the
                // random number generated from the parent model.
                // Remember that the member variable is a shared pointer,
                // so you need to dereference it by writing '*' in front of it.
                g[e].state.weight *= _prob_distr(*this->_rng);
            }
        };

        // Apply the single edge initialization rule to all edges
        // NOTE that you should distinguish between in_edges and out_edges
        //      if your graph is directed.
        apply_rule<IterateOver::edges, Update::async, Shuffle::off>
            (initialize_edge, g);
    }

    // Can add additional setup functions here ...


    // .. Helper functions ....................................................
    
    /// Calculate the mean of all vertices' some_state
    double calc_some_state_mean () const {
        auto sum = 0.;
        for (const auto v : range<IterateOver::vertices>(_g)) {
            sum += _g[v].state.some_state;
        }
        return sum / boost::num_vertices(_g);
    }    


    // .. Rule functions ......................................................
    // Rule functions that can be applied to the graph's vertices or edges
    // NOTE The below are examples; delete and/or adjust them to your needs!
    //      Ideally, only define those rule functions as members that are used
    //      more than once.

    /// An interaction function of a single vertex with its neighbors
    VertexVoidRule some_interaction = [this](const VertexDesc v, GraphType& g){
        // Increase some_state by one
        g[v].state.some_state += 1;

        // Iterate over all neighbors of the current vertex
        for (const auto nb : range<IterateOver::neighbors>(v, g)) 
        {
            // Obvious thing to do is to increase some_trait by the sum of
            // some_traits's of the neighbor. Sure thing.
            g[v].state.some_trait += g[nb].state.some_trait;

            // Let's add a random number in range [-1, +1] as well
            g[v].state.some_trait 
                += (this->_prob_distr(*this->_rng) * 2. - 1.);
        }

        // Ahhh and obviously you need to divide some float by 
        // _some_parameter because that makes totally sense
        g[v].state.some_trait /= this->_some_parameter;
    };


    /// Some other rule function
    /** \warning This rule should be applied synchronously, so the vertex
     *           state may NOT be changed directly. Instead, the state needs
     *           to be copied and changes should be done to the copied state
     *           only.
     */
    VertexStateRule some_other_rule = [this](const VertexDesc v, GraphType& g){
        // COPY the state -- important for synchronous update
        auto state = g[v].state;

        // With a probability of 0.3 set the vertex's some_state to 0
        if (this->_prob_distr(*this->_rng) < 0.3) {
            state.some_state = 0;
        }

        return state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    /** Here you can add a detailed description what exactly happens in a 
     *  single iteration step
     */
    void perform_step () {
        // Apply the rule 'some_interaction' to all vertices sequentially
        apply_rule<IterateOver::vertices, Update::async, Shuffle::on>
            (some_interaction, _g, *this->_rng);

        // Apply 'some_other_rule' synchronously to all vertices
        apply_rule<IterateOver::vertices, Update::sync>
            (some_other_rule, _g);
    }


    /// Monitor model information
    /** Here, functions and values can be supplied to the monitor that are
     *  then available to the frontend. The monitor() function is _only_
     *  called if a certain emit interval has passed; thus, the performance
     *  hit is small.
     *  With this information, you can then define stop conditions on frontend 
     *  side, that can stop a simulation once a certain set of conditions is 
     *  fulfilled.
     */
    void monitor () {
        this->_monitor.set_entry("some_value", 42);
        this->_monitor.set_entry("state_mean", calc_some_state_mean());
    }


    /// Write data
    /** This function is called to write out data. The model configuration
     *  determines at which times data is written.
     *  See \ref Utopia::DataIO::Dataset::write
     */
    void write_data () {
        // .. Writing to the static graph group ...............................
        // To save data to the already created datasets, the write method can
        // be called directly using a lambda function to extract the data from
        // the graph.
        // 
        // Get the iterator pair of the vertices
        auto [v, v_end] = boost::vertices(_g);

        // Write out the some_state of all vertices
        _dset_some_state->write(v, v_end,
            [this](const auto v) { return this->_g[v].state.some_state; }
        );

        // Write out the some_trait of all vertices
        _dset_some_trait->write(v, v_end,
            [this](const auto v) { return this->_g[v].state.some_trait; }
        );

        // .. Writing to the dynamic graph group ..............................
        // To save data for dynamic graphs, it is recommended to use the
        // save_<vertex/edge>_properties functions. Since it writes all data in
        // a single iteration over the graph entities, it ensures that all data
        // written within one time step is aligned.
        // 
        // First, two 'adaptor tuples' are defined - one that accesses the data
        // via a vertex-descriptor and one that does it via an edge-descriptor.
        // NOTE In order to ensure correct ordering of the data, there must not
        //      be a second adaptor tuple using the same type.
        // The adaptor tuple for the vertex data contains 1D adaptors which are
        // tuples of the form: (adaptor_name, adaptor_lambda)
        const auto get_vertex_data = std::make_tuple(
            std::make_tuple("_vertices", [](auto vd, auto& g) {
                        return boost::get(boost::vertex_index_t(), g, vd);}),
            std::make_tuple("some_state", [](auto vd, auto& g) {
                        return g[vd].state.some_state;}),
            std::make_tuple("some_trait", [](auto vd, auto& g) {
                        return g[vd].state.some_trait;})
        );

        // N-dimensional data can also be written. Here, 2D adaptors are used
        // for the edges and a 1D adaptor for the weights. For more details,
        // checkout the documentation of the save_<vertex/edge>_properties
        // functions.
        const auto get_edge_data = std::make_tuple(
            std::make_tuple("_edges", "label",
                std::make_tuple("source", [](auto ed, auto& g) {
                            return boost::get(  boost::vertex_index_t(),
                                                g,
                                                boost::source(ed, g));}),
                std::make_tuple("target", [](auto ed, auto& g) {
                            return boost::get(  boost::vertex_index_t(),
                                                g,
                                                boost::target(ed, g));})),
            std::make_tuple("weights", [](auto ed, auto& g) {
                        return g[ed].state.weight;})
        );
        // NOTE When using an adaptor tuple for saving dynamic edges (vertices),
        //      all edge (vertex) properties should also be saved via the
        //      adaptor tuple to ensure correct ordering.

        // Then, the save_<vertex/edge>_properties functions can be called.
        // NOTE Using the current time as the name of the dataset(s) indexes the
        //      data and allows to later load the data directly into a NetworkX
        //      graph.
        save_vertex_properties(_g, _dgrp_g_dynamic,
                               std::to_string(get_time()),
                               get_vertex_data);
        save_edge_properties(_g, _dgrp_g_dynamic,
                             std::to_string(get_time()),
                             get_edge_data);

        // When writing in every time step, this will result in the following
        // data structure:
        // ...
        // └┬ graph_group
        //      └┬ _vertices
        //           └┬ 0            < ... shape(num_vertices,)
        //            ├ 1            < ... shape(num_vertices,)
        //            ├ ...
        //       ├ _edges
        //           └┬ 0            < ... shape(num_edges, 2)
        //            ├ 1            < ... shape(num_edges, 2)
        //            ├ ...
        //       ├ ...
        // 
        // By default, the coordinates 'vertex_idx' and 'edge_idx' are added.
    }


    // .. Getters and setters .................................................
    // Add public getters and setters here to interface with other models

};


} // namespace Utopia::Models::CopyMeGraph

#endif // UTOPIA_MODELS_COPYMEGRAPH_HH
