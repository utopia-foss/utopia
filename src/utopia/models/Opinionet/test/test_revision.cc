#define BOOST_TEST_MODULE revision test

#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/graph/random.hpp>

#include "../Opinionet.hh"
#include "../modes.hh"
#include "../revision.hh"
#include "../utils.hh"

namespace Utopia::Models::Opinionet::Revision{

/// -- Type definitions -------------------------------------------------------
std::mt19937 rng{};
std::uniform_real_distribution<double> prob_distr;

// -- Fixtures ----------------------------------------------------------------
struct TestNetworkD {
    using vertex = boost::graph_traits<NetworkDirected>::vertex_descriptor;
    NetworkDirected nw;
    vertex v1 = boost::add_vertex(nw);
    vertex v2 = boost::add_vertex(nw);
    vertex v3 = boost::add_vertex(nw);
    vertex v4 = boost::add_vertex(nw);
    vertex v5 = boost::add_vertex(nw);
    vertex v6 = boost::add_vertex(nw);

    const double weighting = 1.5;

    TestNetworkD(){
        boost::add_edge(v1, v2, nw);
        boost::add_edge(v1, v3, nw);
        boost::add_edge(v1, v4, nw);
        boost::add_edge(v1, v6, nw);
        int i = 2;
        for (const auto v : range<IterateOver::vertices>(nw)) {
            nw[v].opinion = i;
            ++i;
        }
        for (const auto v : range<IterateOver::vertices>(nw)) {
            Utils::set_and_normalize_weights(v, nw, weighting);
        }
    }
};


struct TestNetworkU {
    using vertex = boost::graph_traits<NetworkUndirected>::vertex_descriptor;
    NetworkUndirected nw;
    vertex v1 = boost::add_vertex(nw);
    vertex v2 = boost::add_vertex(nw);
    vertex v3 = boost::add_vertex(nw);
    vertex v4 = boost::add_vertex(nw);
    vertex v5 = boost::add_vertex(nw);
    vertex v6 = boost::add_vertex(nw);

    TestNetworkU(){
        boost::add_edge(v1, v2, nw);
        boost::add_edge(v1, v3, nw);
        boost::add_edge(v1, v4, nw);
        boost::add_edge(v1, v6, nw);
        int i = 0;
        for (const auto v : range<IterateOver::vertices>(nw)) {
            nw[v].opinion = i;
            ++i;
        }
    }
};

// -- Actual test -------------------------------------------------------------
// Test HegselmannKrause opinion update function (undirected network)
BOOST_FIXTURE_TEST_CASE(test_opinion_update_HK_u,
                        TestNetworkU) {
    const double tolerance = 4;
    const double susceptibility = 1;
    update_opinion_HK(v1, nw, susceptibility, tolerance);
    BOOST_TEST (nw[v1].opinion==2);
}

// Test HegselmannKrause opinion update function (directed network)
BOOST_FIXTURE_TEST_CASE(test_opinion_update_HK_d,
                        TestNetworkD,
                        * boost::unit_test::tolerance(0.0001))
{
    const double tolerance = 4;
    const double susceptibility = 1;
    update_opinion_HK(v1, nw, susceptibility, tolerance);
    BOOST_TEST (nw[v1].opinion==4.3296);
}

// Test rewiring (undirected network)
BOOST_FIXTURE_TEST_CASE(test_rewiring_u,
                        TestNetworkU) {
    const double tolerance = 3;
    const double weighting = 1;
    for (size_t i = 0; i<20; ++i) {
        rewire_random_edge(nw, tolerance, weighting, rng);
    }

    BOOST_TEST(edge(v1, v5, nw).second);
    BOOST_TEST(!edge(v1, v6, nw).second);
}

// Test rewiring (directed network)
BOOST_FIXTURE_TEST_CASE(test_rewiring_d,
                        TestNetworkD) {
    const double tolerance = 3;
    for (size_t i = 0; i<20; ++i) {
        rewire_random_edge(nw, tolerance, weighting, rng);
    }

    // Test rewiring
    BOOST_TEST(edge(v1, v5, nw).second);
    BOOST_TEST(!edge(v1, v6, nw).second);
}

// Test Deffuant opinion update function (continuous opinion space)
BOOST_FIXTURE_TEST_CASE(test_opinion_update_D_c,
                        TestNetworkD,
                        * boost::unit_test::tolerance(0.05))
{
    using modes::Opinion_space_type;

    TestNetworkD();
    const double tolerance = 2;
    const double susceptibility = 0.5;
    const int num_steps = 10000;
    const double init_opinion = nw[v1].opinion;

    //Check interaction probabilities equal to ratio of opinion differences
    int v2_selected = 0;
    int v3_selected = 0;
    const double ratio_of_interactions
        = exp(-weighting*(fabs(nw[v2].opinion-nw[v1].opinion)
                          -fabs(nw[v3].opinion-nw[v1].opinion)));

    for (int i = 0; i<num_steps; ++i) {
        update_opinion_Deffuant<Opinion_space_type::continuous>(
            v1, nw, susceptibility, tolerance, prob_distr, rng
        );
        if (nw[v1].opinion
          == init_opinion+susceptibility*(nw[v2].opinion-init_opinion)
        ) {
                ++v2_selected;
        }
        else if (nw[v1].opinion
               == init_opinion +susceptibility*(nw[v3].opinion-init_opinion)
        ) {
                ++v3_selected;
        }
        nw[v1].opinion = init_opinion;
    }

    BOOST_TEST((1. * v2_selected)/v3_selected
            == ratio_of_interactions);

}
// Test Deffuant opinion update function (discrete opinion space)
BOOST_FIXTURE_TEST_CASE(test_opinion_update_D_d,
                        TestNetworkU,
                        * boost::unit_test::tolerance(0.05))
{
    using modes::Opinion_space_type;

    TestNetworkU();
    const double tolerance = 5;
    const double susceptibility = 0.5;
    const int num_steps = 100000;
    const double init_opinion = nw[v1].opinion;

    // Check ratio of opinion flips is equal to susceptibility
    // Check ratio of flips to opinion of v2 is proportional to 1/out_degree
    int total_opinion_flips = 0;
    int opinion_flipped_to_v2 = 0;
    for (int i = 0; i<num_steps; ++i) {
        update_opinion_Deffuant<Opinion_space_type::discrete>(
            v1, nw, susceptibility, tolerance, prob_distr, rng
        );
        if (nw[v1].opinion == nw[v2].opinion) {
            ++opinion_flipped_to_v2;
        }
        if (nw[v1].opinion != init_opinion) {
            ++total_opinion_flips;
        }
        nw[v1].opinion = init_opinion;
    }

    BOOST_TEST(1.*total_opinion_flips == num_steps * susceptibility);
    BOOST_TEST(1.*opinion_flipped_to_v2
            == num_steps * susceptibility/boost::degree(v1, nw)
    );
}

} // namespace Utopia::Models::Opinionet::Revision
