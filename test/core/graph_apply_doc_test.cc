#define BOOST_TEST_MODULE apply rule on graphs doc test

#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/copy.hpp>

#include <utopia/core/graph/apply.hh>
#include <utopia/core/state.hh>
#include <utopia/core/graph/entity.hh>

namespace Utopia
{

// ++ Types +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Below, an example of the required graph types - doc reference line

// -- Vertex --------------------------------------------------------------
/// The vertex state 
struct VertexState {
    /// A vertex property
    unsigned v_prop = 0;

    // Add your vertex parameters here.
    // ...
};

/// The traits of a vertex are just the traits of a graph entity
using VertexTraits = Utopia::GraphEntityTraits<VertexState>;

/// A vertex is a graph entity with vertex traits
using Vertex = GraphEntity<VertexTraits>;

// -- Edge ----------------------------------------------------------------
/// The edge state
struct EdgeState {
    /// An edge property
    unsigned e_prop = 0;

    // Add your edge parameters here.
    // ...
};

/// The traits of an edge are just the traits of a graph entity
using EdgeTraits = Utopia::GraphEntityTraits<EdgeState>;

/// An edge is a graph entity with edge traits
using Edge = GraphEntity<EdgeTraits>;

// -- Graph ---------------------------------------------------------------
/// Declare a graph type with the formerly defined Vertex and Edge types
using Graph = boost::adjacency_list<
              boost::vecS,         // edge container
              boost::vecS,         // vertex container
              boost::undirectedS,
              Vertex,
              Edge>;

// End of the required graph types - doc reference line

// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
struct GraphFixture {
    using VertexDesc = typename boost::graph_traits<Graph>::vertex_descriptor;

    // Create a random number generator
    Utopia::DefaultRNG rng;

    const unsigned num_vertices;
    const unsigned num_edges;
    const unsigned v_prop_default;
    const unsigned e_prop_default;

    Graph g;

    GraphFixture() :
        num_vertices{10},
        num_edges{20},
        v_prop_default{1},
        e_prop_default{2},
        g{}
    {
        // Creatig a test graph
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{v_prop_default}), this->g);
        }

        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g, rng), 
                            boost::random_vertex(g, rng), 
                            Edge(EdgeState{e_prop_default}), g);
        }
    };
};


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOST_FIXTURE_TEST_CASE(Test_apply_rule_graph_doc_examples, GraphFixture)
{
    // Below: Start of apply_rule on graph entities examples - doc reference line 
    // -- Simple Examples -----------------------------------------------------
    // The full possibilities are described in the detailed example below
    
    // Set all vertices' v_prop to 42
    apply_rule<IterateOver::vertices, Update::async, Shuffle::off>(
        [](const auto vertex_desc, auto& g){
            auto& state = g[vertex_desc].state;
            state.v_prop = 42;
        },
        g 
    );

    // Set all neighbors' v_prop synchronously to the sum of all their 
    // neighbors' v_prop accumulated to the former v_prop.
    apply_rule<IterateOver::neighbors, Update::sync, Shuffle::off>(
        [](const auto neighbor_desc, auto& g){
            auto state = g[neighbor_desc].state;
            
            for (const auto next_neighbor 
                    : range<IterateOver::neighbors>(neighbor_desc, g)){
                state.v_prop += g[next_neighbor].state.v_prop;
            }

            return state;
        },
        boost::vertex(0, g), // Neighbors of vertex '0'
        g 
    );

    // -- Example with detailed explanation -----------------------------------
    apply_rule<                     // Apply a rule to graph entities
        IterateOver::vertices,      // Choose the entities that the rule 
                                    // should be applied to. Here: vertices.
                                    // All available options are:
                                    //      IterateOver::vertices
                                    //      IterateOver::edges
                                    //
                                    //      IterateOver::neighbors
                                    //      IterateOver::inv_neighbors (inverse)
                                    //      IterateOver::degree
                                    //      IterateOver::out_degree
                                    //      IterateOver::in_degree
                                    //
                                    // The last options require the
                                    // parent_vertex that works as a reference.

        Update::async,              // Apply the rule asynchronously, i.e.
                                    // sequentially. Other option: Update::sync
        
        Shuffle::off                // Randomize the order (Shuffle::on)
                                    // or not (Shuffle::off).
    >(
        []
        (const auto vertex_desc,    // In this example, iteration happens
                                    // over vertices; thus, the first argument
                                    // is the vertex descriptor.
                                    // The vertex descriptor is normally just a
                                    // literal type, so copying is actually
                                    // faster than taking it by const reference
                                    // NOTE: The cell- or agent-based
                                    // apply_rule expect the state as a const
                                    // reference.

        auto& g)                    // The rule function expects the graph
                                    // as second argument.
                                    // NOTE: It's IMPORTANT to pass this as
                                    // (non-const) reference, otherwise the
                                    // whole graph is copied!
        {
            // Get the state (by reference)
            auto& state = g[vertex_desc].state;     
            // WARNING: If Update::sync was selected, you should work on a COPY
            //          of the state. To achieve that, leave away the '&' and
            //          return the state at the end of the rule function.

            // Set a vertex property
            state.v_prop = 42;

            // ... can do more stuff here ...
            
            // For Update::sync, return the state. Optional for Update::async.
            // return state;
        },
        // boost::vertex(0, g),     // This is the parent_vertex argument.
                                    // The parent vertex that needs to be
                                    // given when IterateOver requires a 
                                    // reference vertex.
                                    // As an example, vertex 0 is given here.

        g                           // Finally, specify the graph that
                                    // contains the objects to iterate over.
                                    // It is passed as the second argument to
                                    // the rule function.
    );
    // End of apply_rule on graph entities examples - doc reference line 
}

}   // namespace Utopia
