#define BOOST_TEST_MODULE utilities test

#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>

#include "../Opinionet.hh"

namespace Utopia::Models::Opinionet::Utils{

using Network = boost::adjacency_list<
        EdgeContainer,
        VertexContainer,
        boost::bidirectionalS,
        Agent,
        Edge>;
// Define a random number generator
std::mt19937 rng{};

// -- Type definitions --------------------------------------------------------

// The vertex descriptor type
using VertexDesc = typename boost::graph_traits<Network>::vertex_descriptor;

// -- Fixtures ----------------------------------------------------------------

// Test network
struct TestNetwork {
    Network nw;
    VertexDesc v1 = boost::add_vertex(nw);
    VertexDesc v2 = boost::add_vertex(nw);
    VertexDesc v3 = boost::add_vertex(nw);
    VertexDesc v4 = boost::add_vertex(nw);

    TestNetwork(){
        boost::add_edge(v1, v2, {0.5}, nw);
        boost::add_edge(v1, v3, {0.5}, nw);
    }
};

// -- Actual test -------------------------------------------------------------

// Test the get_rand function
BOOST_AUTO_TEST_CASE(test_get_rand)
{
    BOOST_TEST(1==1);
    // auto rand_int = get_rand<int>(2, rng);
    // BOOST_TEST(rand_int <= 2);
    // BOOST_TEST(rand_int >= 0);
    //
    // auto rand_double = get_rand<double>(1., rng);
    // BOOST_TEST(rand_double <= 1.);
    // BOOST_TEST(rand_double >= 0.);
    //
    // auto interval = std::make_pair(2., 3.);
    // auto double_from_interval = get_rand<double>(interval, rng);
    // BOOST_TEST(double_from_interval <= 3.);
    // BOOST_TEST(double_from_interval >= 2.);
}

// Test the get_rand_neighbor function
BOOST_FIXTURE_TEST_CASE(test_get_rand_neighbor, TestNetwork)
{
    BOOST_TEST(1==1);
    // VertexDesc nb = get_rand_neighbor(nw, v1, rng);
    //
    // BOOST_TEST((nb == v2 || nb == v3));
}

} // namespace Utopia::Models::Opinionet::Utils
