#define BOOST_TEST_MODULE apply rule on graphs test

#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>

#include <utopia/core/apply.hh>
#include <utopia/core/state.hh>
#include <utopia/core/cell.hh>

namespace Utopia
{

// ++ Types +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct VertexState {
    unsigned v_prop = 0;
};

struct EdgeState {
    unsigned e_prop = 0;
};


/// Specify the traits of vertices and edges
/** Vertices and edges need to be defined as Utopia::Cells. The cell struct
 *  contains all relevant traits that are required to provide 
 *  the full functionality required to use Utopia features.
 */
using VertexTraits = Utopia::CellTraits<VertexState, Update::manual>;
using EdgeTraits = Utopia::CellTraits<EdgeState, Update::manual>;

using Vertex = Cell<VertexTraits>;
using Edge = Cell<EdgeTraits>;

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
            boost::add_vertex(Vertex(v, VertexState{10}), this->g);
        }

        // Create num_edges random edges
        std::uniform_int_distribution<unsigned> dist(0, num_vertices - 1);
        for (auto e = 0u; e < num_edges; ++e){
            // Sample two random vertices
            const auto v1 = boost::vertex(dist(rng), g);
            const auto v2 = boost::vertex(dist(rng), g);

            boost::add_edge(v1, v2, Edge(e, EdgeState{}), g);
        }


    };
};

using GraphFixtures = boost::mpl::vector<
    // GraphFixture<G_dir_vec>,
    GraphFixture<G_dir_list>,
    GraphFixture<G_undir_list>
    // GraphFixture<G_undir_vec>
>;


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule, G, 
    GraphFixtures, G)
{

}

}   // namespace Utopia
