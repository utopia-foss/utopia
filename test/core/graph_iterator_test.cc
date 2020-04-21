#define BOOST_TEST_MODULE graph iterator_pair Iteratetest

#include <cmath>
#include <tuple>
#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/random.hpp>

#include <utopia/core/graph/iterator.hh>

using namespace Utopia;


// -- Type definitions --------------------------------------------------------

/// A custom type for bundled vertex properties
struct Node {
    /// A test parameter
    double param;
};

/// A custom type for bundled edge properties
struct Edge {
    /// Some parameter
    double weight;
};

/// The BGL default adjacency_list type. Supports most iterator.hh tools
using DefaultGraph = boost::adjacency_list<>;

/// A test graph type that supports ALL iterator.hh tools
using FullFunctioningGraph =
    boost::adjacency_list<
        // edge and vertex container types
        boost::listS,
        boost::vecS,
        boost::undirectedS,
        // vertex and edge property types
        Node,
        Edge
    >;

/// A test graph type that uses different container types
using ExoticContainerTypesGraph =
    boost::adjacency_list<
        // edge and vertex container types
        boost::setS,
        boost::listS,
        boost::undirectedS,
        // vertex and edge property types
        Node,
        Edge
    >;

/// A boost::subgraph type. Supports all but inv_adjacent_vertices.
using UndirectedSubGraph =
    boost::subgraph<boost::adjacency_list<
        // Edge and Vertex container type
        boost::listS,
        boost::vecS,
        boost::undirectedS,
        // vertex and edge property types
        // for boost::subgraph, they need to supply an internal index
        boost::property<
            boost::vertex_index_t, std::size_t,
            Node
        >,
        boost::property<
            boost::edge_index_t, std::size_t,
            Edge
        >
    >>;

/// A boost::subgraph type. Supports all but inv_adjacent_vertices.
using DirectedSubGraph =
    boost::subgraph<boost::adjacency_list<
        // Edge and Vertex container type
        boost::listS,
        boost::vecS,
        boost::directedS,
        // vertex and edge property types
        // for boost::subgraph, they need to supply an internal index
        boost::property<
            boost::vertex_index_t, std::size_t,
            Node
        >,
        boost::property<
            boost::edge_index_t, std::size_t,
            Edge
        >
    >>;

/// The BGL default adjacency_matrix type.
using DefaultMatrix = boost::adjacency_matrix<>;

/// An adjacency matrix with custom node and edge properties
using MatrixWithProperties =
    boost::adjacency_matrix<boost::undirectedS, Node, Edge>;


/// The graph types to test
using graph_types =
    std::tuple<
        DefaultGraph,
        FullFunctioningGraph,
        ExoticContainerTypesGraph,
        UndirectedSubGraph,
        DirectedSubGraph,
        DefaultMatrix,
        MatrixWithProperties
    >;


// -- Fixtures and Helpers ----------------------------------------------------

/// A graph generator
template<class G>
G setup_graph () {
    std::mt19937 rng([](){
        std::random_device rd{};
        return rd();
    }());

    const unsigned num_vertices = 20;
    const unsigned num_edges = 50;

    // Need to distinguish between matrix-like and other graphs
    if constexpr (   std::is_same<G, DefaultMatrix>()
                  or std::is_same<G, MatrixWithProperties>())
    {
        // Generate a random graph with (unfixed) number of edges
        const auto p_edge = ((double) num_edges) / std::pow(num_vertices, 2);
        auto prob = std::uniform_real_distribution<>(0., 1.);

        G g(num_vertices);
        for (auto src : boost::make_iterator_range(boost::vertices(g))) {
            for (auto dst : boost::make_iterator_range(boost::vertices(g))) {
                if (prob(rng) < p_edge) {
                    boost::add_edge(src, dst, g);
                }
            }
        }

        return g;
    }
    else {
        // Generate a random graph with fixed number of edges
        constexpr bool allow_parallel = false;
        constexpr bool allow_self_edges = false;

        G g(0);
        boost::generate_random_graph(g,
                                     num_vertices,
                                     num_edges,
                                     rng,
                                     allow_parallel,
                                     allow_self_edges);
        return g;
    }
}


// -- Actual tests ------------------------------------------------------------

BOOST_AUTO_TEST_SUITE (test_graph_iteration)

/// Test retrieving over iterator pairs for all possible graph entities
BOOST_AUTO_TEST_CASE_TEMPLATE (get_iterator_pair, Graph, graph_types)
{
    // Gather some type information
    using VertexDesc = typename boost::graph_traits<Graph>::vertex_descriptor;
    constexpr bool is_directed =
        std::is_same<
            typename boost::graph_traits<Graph>::directed_category,
            boost::directed_tag
        >();

    const auto g = setup_graph<Graph>();
    BOOST_TEST(boost::num_vertices(g) > 0);
    BOOST_TEST(boost::num_edges(g) > 0);

    // .. Vertices
    {
        auto it = GraphUtils::iterator_pair<IterateOver::vertices>(g);
        for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
            BOOST_TEST(*it.first == *v);
            ++it.first;
        }
    }

    // .. Edges
    {
        auto it = GraphUtils::iterator_pair<IterateOver::edges>(g);
        for (auto [e, e_end] = boost::edges(g); e!=e_end; ++e){
            BOOST_TEST(*it.first == *e);
            ++it.first;
        }
    }

    // .. Neighbors
    //    Need some descriptor for that
    const VertexDesc v = boost::vertex(2, g);
    {
        auto it = GraphUtils::iterator_pair<IterateOver::neighbors>(v, g);
        for (auto [nb, nb_end] = boost::adjacent_vertices(v, g);
             nb!=nb_end;
             ++nb)
        {
            BOOST_TEST(*it.first == *nb);
            ++it.first;
        }
    }

    // .. inverse neighbors; not supported for directedS, subgraphs, matrices
    if constexpr (    not is_directed
                  and not std::is_same<Graph, UndirectedSubGraph>()
                  and not std::is_same<Graph, DirectedSubGraph>()
                  and not std::is_same<Graph, DefaultMatrix>()
                  and not std::is_same<Graph, MatrixWithProperties>()
                  )
    {
        auto it = GraphUtils::iterator_pair<IterateOver::neighbors>(v, g);
        for (auto [nb, nb_end] = boost::inv_adjacent_vertices(v, g);
             nb!=nb_end;
             ++nb)
        {
            BOOST_TEST(*it.first == *nb);
            ++it.first;
        }
    }

    // .. in edges; not supported for directedS
    if constexpr (not is_directed)
    {
        auto it = GraphUtils::iterator_pair<IterateOver::in_edges>(v, g);
        for (auto [e, e_end] = boost::in_edges(v, g); e!=e_end; ++e){
            BOOST_TEST(*it.first == *e);
            ++it.first;
        }
    }

    // .. out edges
    {
        auto it = GraphUtils::iterator_pair<IterateOver::out_edges>(v, g);
        for (auto [e, e_end] = boost::out_edges(v, g); e!=e_end; ++e){
            BOOST_TEST(*it.first == *e);
            ++it.first;
        }
    }
}


/// Test range iteration for all possible graph entities
BOOST_AUTO_TEST_CASE_TEMPLATE (get_range, Graph, graph_types)
{
    // Gather some type information
    using VertexDesc = typename boost::graph_traits<Graph>::vertex_descriptor;
    constexpr bool is_directed =
        std::is_same<
            typename boost::graph_traits<Graph>::directed_category,
            boost::directed_tag
        >();

    const auto g = setup_graph<Graph>();
    BOOST_TEST(boost::num_vertices(g) > 0);
    BOOST_TEST(boost::num_edges(g) > 0);

    // .. Vertices
    {
        auto it = boost::vertices(g);
        for (auto v : range<IterateOver::vertices>(g)){
            BOOST_TEST(*it.first == v);
            ++it.first;
        }
    }

    // .. Edges
    {
        auto it = boost::edges(g);
        for (auto e : range<IterateOver::edges>(g)){
            BOOST_TEST(*it.first == e);
            ++it.first;
        }
    }

    // .. Neighbors
    //    Need some descriptor for that
    const VertexDesc v = boost::vertex(2, g);
    {
        auto it = boost::adjacent_vertices(v, g);
        for (auto nb : range<IterateOver::neighbors>(v, g)) {
            BOOST_TEST(*it.first == nb);
            ++it.first;
        }
    }

    // .. inverse neighbors; not supported for directedS, subgraphs, matrices
    if constexpr (    not is_directed
                  and not std::is_same<Graph, UndirectedSubGraph>()
                  and not std::is_same<Graph, DirectedSubGraph>()
                  and not std::is_same<Graph, DefaultMatrix>()
                  and not std::is_same<Graph, MatrixWithProperties>()
                  )
    {
        auto it = boost::inv_adjacent_vertices(v, g);
        for (auto nb : range<IterateOver::inv_neighbors>(v, g)) {
            BOOST_TEST(*it.first == nb);
            ++it.first;
        }
    }

    // .. in edges; not supported for directedS
    if constexpr (not is_directed)
    {
        auto it = boost::in_edges(v, g);
        for (auto e : range<IterateOver::in_edges>(v, g)){
            BOOST_TEST(*it.first == e);
            ++it.first;
        }
    }

    // .. out edges
    {
        auto it = boost::out_edges(v, g);
        for (auto e : range<IterateOver::out_edges>(v, g)){
            BOOST_TEST(*it.first == e);
            ++it.first;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // "test_iterator"
