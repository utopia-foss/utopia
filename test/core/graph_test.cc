#define BOOST_TEST_MODULE graph test

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>

#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>

using namespace Utopia::Graph;


// -- Types -------------------------------------------------------------------

struct Vertex {};

/// The test graph types
using G_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::undirectedS,
                    Vertex>;             // vertex struct

using G_dir_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::bidirectionalS,
                    Vertex>;             // vertex struct

using G_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::undirectedS,
                    Vertex>;             // vertex struct

using G_dir_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::bidirectionalS,
                    Vertex>;             // vertex struct

// -- FIXTURES ----------------------------------------------------------------

// .. Erdös-Rènyi graph fixtures ----------------------------------------------
template<class G>
struct ErdosRenyiGraphFixture {
    // Create a random number generator together with a copy
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    unsigned num_vertices = 10;

    unsigned mean_degree = 2;

    // Create test graphs
    G g = create_ErdosRenyi_graph<G>(num_vertices, mean_degree, 
                                        false, false, rng); 
};

using ErdosRenyiDirGraphsFixtures = boost::mpl::vector<
    ErdosRenyiGraphFixture<G_dir_vec>,
    ErdosRenyiGraphFixture<G_dir_list>
>;

using ErdosRenyiUndirGraphsFixtures = boost::mpl::vector<
    ErdosRenyiGraphFixture<G_vec>,
    ErdosRenyiGraphFixture<G_list>
>;

// .. Watts-Strogatz graph fixtures -------------------------------------------
template<class G>
struct WattsStrogatzGraphFixture {
    // Create a random number generator together with a copy
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const unsigned num_vertices = 100;
    const unsigned mean_degree = 2;
    const double p_rewire = 0.6;

    // Create test graph
    G g = create_WattsStrogatz_graph<G>(num_vertices,
                                           mean_degree,
                                           p_rewire,
                                           rng); 
};

using WattsStrogatzDirGraphsFixtures = boost::mpl::vector<
    WattsStrogatzGraphFixture<G_dir_vec>,
    WattsStrogatzGraphFixture<G_dir_list>
>;

using WattsStrogatzUndirGraphsFixtures = boost::mpl::vector<
    WattsStrogatzGraphFixture<G_vec>,
    WattsStrogatzGraphFixture<G_list>
>;

// .. Barabási-Albert graph fixtures ------------------------------------------
template<class G, bool create_parallel_edges>
struct BarabasiAlbertGraphFixture {
    // Create a random number generator together with a copy
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const unsigned num_vertices = 200;
    const unsigned mean_degree = 8;

    // Create test graph
    G g = create_BarabasiAlbert_graph<G>(num_vertices,
                                           mean_degree,
                                           create_parallel_edges,
                                           rng); 
};

using BarabasiAlbertUndirGraphsFixtures = boost::mpl::vector<
    BarabasiAlbertGraphFixture<G_vec, true>,
    BarabasiAlbertGraphFixture<G_vec, false>,
    BarabasiAlbertGraphFixture<G_list, true>,
    BarabasiAlbertGraphFixture<G_list, false>
>;


// .. Bollobas-Riordan graph fixtures ------------------------------------------
template<class G>
struct BollobasRiordanGraphFixture {
    // Create a random number generator together with a copy
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const std::size_t num_vertices = 200;
    const double alpha = 0.2;
    const double beta = 0.8;
    const double gamma = 0.;
    const double del_in = 0.;
    const double del_out = 0.5;

    // Create test graph
    G g = create_BollobasRiordan_graph<G>(num_vertices,
                                                      alpha,
                                                      beta,
                                                      gamma,
                                                      del_in,
                                                      del_out,
                                                      rng);
};

using BollobasRiordanDirGraphsFixtures = boost::mpl::vector<
    BollobasRiordanGraphFixture<G_dir_vec>,
    BollobasRiordanGraphFixture<G_dir_list>
>;


// -- Tests -------------------------------------------------------------------

/// Test the function that creates a random graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_ErdosRenyi_graph_directed, G, 
    ErdosRenyiDirGraphsFixtures, G)
{
    // Assert that the number of vertices and edges is correct
    // In the undirected case, the number of edges is two times the added 
    // edges because both edge pairs (i,j) and (j,i) count.
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));
    BOOST_TEST(G::num_vertices * G::mean_degree == boost::num_edges(G::g));

    // Assert that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);    
}


/// Test the function that creates a random graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_ErdosRenyi_graph_undirected, G, 
    ErdosRenyiUndirGraphsFixtures, G)
{
    // Assert that the number of vertices and edges is correct
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));
    BOOST_TEST(G::num_vertices * G::mean_degree /2 == boost::num_edges(G::g));

    // Assert that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);    
}

/// Test the function that creates a small-world graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_WattsStrogatzUndirected_graph, G, 
    WattsStrogatzUndirGraphsFixtures, G)
{
    // Assert that the number of vertices and edges is correct
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));
    BOOST_TEST(G::num_vertices * G::mean_degree /2 == boost::num_edges(G::g));

    // Check that at least one vertex does not have connectivity mean_degree 
    // any more
    bool at_least_one_rewired = false;
    for (auto [v, v_end] = boost::vertices(G::g); v!=v_end; ++v){
        if (boost::out_degree(*v, G::g) != G::mean_degree){
            at_least_one_rewired = true;
            break;
        }
    }
    BOOST_TEST(at_least_one_rewired == true);

    // Assert that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);
}

/// Test the function that creates a small-world graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_WattsStrogatzDirected_graph, G, 
    WattsStrogatzDirGraphsFixtures, G)
{
    // Assert that the number of vertices and edges is correct
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));
    BOOST_TEST(G::num_vertices * G::mean_degree == boost::num_edges(G::g));

    // Check that at least one vertex does not have connectivity mean_degree 
    // any more
    bool at_least_one_rewired = false;
    for (auto [v, v_end] = boost::vertices(G::g); v!=v_end; ++v){
        if (boost::out_degree(*v, G::g) != G::mean_degree / 2){
            at_least_one_rewired = true;
            break;
        }
    }
    BOOST_TEST(at_least_one_rewired == true);

    // Assert that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);
}

/// Test the function that creates a scale-free Barabási-Albert graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_BarabasiAlbertUndirected_graph, G, 
    BarabasiAlbertUndirGraphsFixtures, G)
{
    // Assert that the number of vertices and edges is correct
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));
    BOOST_TEST(G::num_vertices * G::mean_degree / 2 == boost::num_edges(G::g));

    // Check that at least one vertex has more than 10 edges
    bool at_least_one_more_than_ten_edges = false;
    for (auto [v, v_end] = boost::vertices(G::g); v!=v_end; ++v){
        if (boost::out_degree(*v, G::g) > 10){
            at_least_one_more_than_ten_edges = true;
            break;
        }
    }
    BOOST_TEST(at_least_one_more_than_ten_edges == true);

    // Assert that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);
}

BOOST_AUTO_TEST_CASE(test_create_BarabasiAlbert_failing_high_degree)
{
    // Create a random number generator 
    Utopia::DefaultRNG rng;

    // Set graph properties
    const unsigned num_vertices = 5;
    const unsigned mean_degree = 6;

    // Try to create a graph and test that it fails
    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_vec>(num_vertices,
                                           mean_degree,
                                           true, // creating parallel edges
                                           rng),
                                           std::invalid_argument);

    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_vec>(num_vertices,
                                           mean_degree,
                                           false, // not creating parallel edges
                                           rng),
                                           std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(test_create_BarabasiAlbert_failing_odd_mean_degree)
{
    // Create a random number generator 
    Utopia::DefaultRNG rng;

    // Set graph properties
    const unsigned num_vertices = 5;
    const unsigned mean_degree = 5;

    // Try to create a graph and test that it fails
    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_vec>(num_vertices,
                                           mean_degree,
                                           true, // creating parallel edges
                                           rng),
                                           std::invalid_argument);

    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_vec>(num_vertices,
                                           mean_degree,
                                           false, // not creating parallel edges
                                           rng),
                                           std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(test_create_BarabasiAlbert_failing_due_to_directed_graph)
{
    // Create a random number generator 
    Utopia::DefaultRNG rng;

    // Set graph properties
    const unsigned num_vertices = 5;
    const unsigned mean_degree = 6;

    // Try to create a graph and test that it fails
    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_dir_vec>(num_vertices,
                                           mean_degree,
                                           true, // creating parallel edges
                                           rng),
                                           std::runtime_error);

    BOOST_CHECK_THROW(create_BarabasiAlbert_graph<G_dir_vec>(num_vertices,
                                           mean_degree,
                                           false, // not creating parallel edges
                                           rng),
                                           std::runtime_error);
}

/// Test the function that creates a directed scale-free graph
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_create_BollobasRiordan_graph, G, 
    BollobasRiordanDirGraphsFixtures, G)
{
    // Assert that the number of vertices is correct
    BOOST_TEST(G::num_vertices == boost::num_vertices(G::g));

    // BOOST_TEST that only tree vertices (the initial network)
    // have an in-degree unequal Zero.
    auto count = 0;
    for (auto [v, v_end] = boost::vertices(G::g); v!=v_end; ++v){
        if (boost::in_degree(*v, G::g) > 0) {
            ++count;
        }
    }
    BOOST_TEST(count == 3);

    // Check that at least one vertex has more than 10 in-edges
    bool at_least_one_more_than_ten_edges = false;
    for (auto [v, v_end] = boost::vertices(G::g); v!=v_end; ++v){
        if (boost::in_degree(*v, G::g) > 10){
            at_least_one_more_than_ten_edges = true;
            break;
        }
    }
    BOOST_TEST(at_least_one_more_than_ten_edges == true);

    // BOOST_TEST that the state of the rng has changed.
    BOOST_TEST(G::rng!=G::rng_copy);

}

BOOST_AUTO_TEST_CASE(test_create_BollobasRiordan_failing_due_to_undirected_graph)
{
// Create a random number generator together with a copy
    Utopia::DefaultRNG rng;

    // Set graph properties
    const std::size_t num_vertices = 200;
    const double alpha = 0.2;
    const double beta = 0.8;
    const double gamma = 0.;
    const double del_in = 0.;
    const double del_out = 0.5;

    // Try to create an undirected test graph and catch the error
    BOOST_CHECK_THROW(create_BollobasRiordan_graph<G_vec>(num_vertices,
                                                      alpha,
                                                      beta,
                                                      gamma,
                                                      del_in,
                                                      del_out,
                                                      rng),
                                                      std::runtime_error);
}
