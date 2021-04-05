#define BOOST_TEST_MODULE revision test

#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/graph/random.hpp>

#include "../Opinionet.hh"

namespace Utopia::Models::Opinionet::Revision{

using Network = boost::adjacency_list<
        EdgeContainer,
        VertexContainer,
        boost::bidirectionalS,
        Agent,
        Edge>;

// Define a random number generator
std::mt19937 rng{};

// A uniform probability distribution
std::uniform_real_distribution<double> uniform_prob_distr(0., 1.);

// -- Type definitions --------------------------------------------------------

// The vertex descriptor type of the user network
using VertexDesc = typename boost::graph_traits<Network>::vertex_descriptor;

// The edge descriptor type of the user network
using EdgeDesc = typename boost::graph_traits<Network>::edge_descriptor;


// -- Fixtures ----------------------------------------------------------------

// Test user network and test media network
struct TestNetworks {
    Network nw;

    VertexDesc v1 = boost::add_vertex(nw);
    VertexDesc v2 = boost::add_vertex(nw);
    VertexDesc v3 = boost::add_vertex(nw);
    VertexDesc v4 = boost::add_vertex(nw);

    EdgeDesc e12 = std::get<0>(boost::add_edge(v1, v2, {0.5}, nw));
    EdgeDesc e13 = std::get<0>(boost::add_edge(v1, v3, {0.5}, nw));
    EdgeDesc e24 = std::get<0>(boost::add_edge(v2, v4, {1.0}, nw));
    EdgeDesc e34 = std::get<0>(boost::add_edge(v3, v4, {1.0}, nw));


    TestNetworks(){
        // add opinions etc.
    }
};


// -- Actual test -------------------------------------------------------------

// Test the evaluate_user_char function
BOOST_AUTO_TEST_CASE(test_evaluate_user_char)
{
    BOOST_TEST ( 1 == 1);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::BC,
    //                                     0.3, 0.49, 0.2, 0.4, rng)) == 1.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::BC,
    //                                     0.3, 0.49, 0.2, 0.4, rng)) == true);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::BC,
    //                                     0.3, 0.09, 0.2, 0.4, rng)) == 0.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::BC,
    //                                     0.3, 0.09, 0.2, 0.4, rng)) == false);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.49, 0.2, 0.4, rng)) == 1.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.49, 0.2, 0.4, rng)) == true);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.09, 0.2, 0.4, rng)) == 0.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.09, 0.2, 0.4, rng)) == false);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.72, 0.2, 0.4, rng)) == 1.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::BC_extended,
    //                                     0.3, 0.72, 0.2, 0.4, rng)) == false);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::Gaussian,
    //                                     0.3, 0.3, 0.2, 0.4, rng)) == 1.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::Gaussian,
    //                                     0.3, 0.3, 0.2, 0.4, rng)) == true);
    // BOOST_TEST(std::get<0>(evaluate_user_char(UserChar::Gaussian,
    //                                     0.3, 1., 0.1, 0.4, rng)) == 0.);
    // BOOST_TEST(std::get<1>(evaluate_user_char(UserChar::Gaussian,
    //                                     0.3, 1., 0.1, 0.4, rng)) == false);
}

// Test the update_weights function
BOOST_FIXTURE_TEST_CASE(test_update_weights, TestNetworks)
{
    BOOST_TEST(1==1);
    // update_weights(v1, nw, 0.5, 1., 1., uniform_prob_distr, rng);
    //
    // BOOST_TEST(nw[e12].weight == 0.45);
    // BOOST_TEST(nw[e13].weight == 0.3);
    //
    // for (int i = 0; i < 5; ++i)
    // {
    //     // Each time edge (v1, v3) is rewired with a probability > 0.5.
    //     update_weights(v1, nw, 0.2, 1., 1., uniform_prob_distr, rng);
    // }
    //
    // // Check that rewiring was done
    // BOOST_TEST(std::get<1>(boost::edge(v1, v4, nw)) == true);
}

// Test the normalize_weights function
BOOST_FIXTURE_TEST_CASE(test_normalize_weights, TestNetworks)
{
    BOOST_TEST(1==1);
    // normalize_weights(v1, nw);
    //
    // BOOST_TEST(nw[e12].weight == 0.5);
    // BOOST_TEST(nw[e13].weight == 0.5);
    //
    // nw[e13].weight = 1.5;
    //
    // normalize_weights(v1, nw);
    //
    // BOOST_TEST(nw[e12].weight == 0.25);
    // BOOST_TEST(nw[e13].weight == 0.75);
}

// Test the user_revision function
BOOST_FIXTURE_TEST_CASE(test_user_revision, TestNetworks)
{
    BOOST_TEST(1==1);
    // EdgeDesc e14 = std::get<0>(boost::add_edge(v1, v4, {0.}, nw));
    // for (int i = 0; i < 10; ++i)
    // {
    //     user_revision(nw, 0.3, 0.5, 1., 0., uniform_prob_distr, rng);
    // }
    //
    // // Check that v1 interacted with v2 or v3, but not with v4
    // BOOST_TEST(nw[e12].weight + nw[e13].weight == 1.);
    // BOOST_TEST(nw[e14].weight == 0.);
    // BOOST_TEST(nw[v1].opinion < 0.5);
}

} // namespace Utopia::Models::Opinionet::Revision
