#define BOOST_TEST_MODULE apply rule on graphs test

#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>

#include <utopia/core/apply.hh>
#include <utopia/core/state.hh>
#include <utopia/core/graph_entity.hh>

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
    Utopia::DefaultRNG rng;

    const unsigned num_vertices;
    const unsigned num_edges;

    G g;

    GraphFixture() :
        num_vertices{10},
        num_edges{20},
        g{}
    {
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{10}), this->g);
        }

        // Create num_edges random edges
        std::uniform_int_distribution<unsigned> dist(0, num_vertices - 1);
        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g, rng), 
                            boost::random_vertex(g, rng), 
                            Edge(EdgeState{20}), g);
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
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule, G, 
    GraphFixtures, G)
{

}

}   // namespace Utopia
