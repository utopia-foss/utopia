#define BOOST_TEST_MODULE apply rule on graphs test

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

struct VertexState {
    unsigned v_prop = 0;
};

struct EdgeState {
    unsigned e_prop = 0;
};


/// The traits of a vertex are just the traits of a graph entity
using VertexTraits = Utopia::GraphEntityTraits<VertexState, Update::manual>;

/// The traits of an edge are just the traits of a graph entity
using EdgeTraits = Utopia::GraphEntityTraits<EdgeState, Update::manual>;

/// A vertex is a graph entity with vertex traits
using Vertex = GraphEntity<VertexTraits>;

/// An edge is a graph entity with edge traits
using Edge = GraphEntity<EdgeTraits>;

/// The test graph types
using G_undir_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::undirectedS,
                    Vertex,
                    Edge>;

using G_dir_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::bidirectionalS,
                    Vertex,
                    Edge>;

using G_undir_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::undirectedS,
                    Vertex,
                    Edge>;

using G_dir_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::bidirectionalS,
                    Vertex,
                    Edge>;

// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template<class G>
struct GraphFixture {
    // Create a random number generator
    Utopia::DefaultRNG rng, rng_ref;

    const unsigned num_vertices;
    const unsigned num_edges;

    G g, g_ref;

    GraphFixture() :
        num_vertices{10},
        num_edges{20},
        g{},
        g_ref{}
    {
        // Creatig a test graph
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{10}), this->g);
        }

        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g, rng), 
                            boost::random_vertex(g, rng), 
                            Edge(EdgeState{20}), g);
        }

        // Creating an equal reference graph     
        // NOTE: Using boost::copy_graph() does not compile.
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{10}), this->g_ref);
        }

        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g_ref, rng_ref), 
                            boost::random_vertex(g_ref, rng_ref), 
                            Edge(EdgeState{20}), g_ref);
        }
    };
};

using GraphFixtures = boost::mpl::vector<
    GraphFixture<G_dir_vec>,
    GraphFixture<G_dir_list>,
    GraphFixture<G_undir_list>,
    GraphFixture<G_undir_vec>
>;


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_noshuffle, G, 
    GraphFixtures, G)
{
    // Set the vertex property to a counter value that increments with each
    // assignment.
    auto counter = 0u;
    apply_rule<IterateOver::vertices, Update::async, Shuffle::off>(
        [this, &counter](auto v){
            auto& state = this->G::g[v].state;
            state.v_prop = counter;
            ++counter;
            return state;
        },
        G::g
    );
    
    // Set the properties manually in the reference graph
    counter = 0;
    for (auto [it, it_end] = boost::vertices(G::g_ref); it!=it_end; ++it){
        G::g_ref[*it].state.v_prop = counter;
        ++counter;
    }

    // Test that both manually applying the rule leads to the same result
    // as with the apply_rule interface
    for (auto i = 0u; i < boost::num_vertices(G::g); ++i){
        BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
        BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop == i);
    }
}



BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_shuffle, G, 
    GraphFixtures, G)
{
    apply_rule<IterateOver::vertices, Update::async, Shuffle::on>(
        [this](auto v){
            auto& state = this->G::g[v].state;
            state.v_prop = 1;
            return state;
        },
        G::g
    );
}

}   // namespace Utopia
