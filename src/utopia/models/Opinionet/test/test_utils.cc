#define BOOST_TEST_MODULE utilities test

#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <utopia/core/model.hh>
#include <utopia/core/testtools.hh>

#include "../Opinionet.hh"
#include "../utils.hh"

namespace Utopia::Models::Opinionet::Utils{

// -- Type definitions --------------------------------------------------------
using Config = Utopia::DataIO::Config;

std::mt19937 rng{};
std::uniform_real_distribution<double> uniform_prob_distr;
Config cfg = YAML::LoadFile("test_config.yml")["test_utils"];

// -- Fixtures ----------------------------------------------------------------


// Test networks
struct TestNetworkU {
    using vertex = boost::graph_traits<NetworkUndirected>::vertex_descriptor;
    NetworkUndirected nw;
    TestNetworkU()
    :
    nw{}
    {
        const unsigned num_vertices = get_as<int>(
            "num_vertices", cfg["nw_params"]
        );
        const unsigned num_edges = get_as<int>("num_edges", cfg["nw_params"]);
        boost::generate_random_graph(
            nw, num_vertices, num_edges, rng, false, false
        );
    }

};

struct TestNetworkD {
    using vertex = boost::graph_traits<NetworkDirected>::vertex_descriptor;
    NetworkDirected nw;
    TestNetworkD()
    :
    nw{}
    {
        const unsigned num_vertices = get_as<int>(
            "num_vertices", cfg["nw_params"]
        );
        const unsigned num_edges = get_as<int>("num_edges", cfg["nw_params"]);
        boost::generate_random_graph(
            nw, num_vertices, num_edges, rng, false, false
        );
    }
};

struct TestNetworkD_small {
    using vertex = boost::graph_traits<NetworkDirected>::vertex_descriptor;
    NetworkDirected nw;
    vertex v1 = boost::add_vertex(nw);
    vertex v2 = boost::add_vertex(nw);
    vertex v3 = boost::add_vertex(nw);
    vertex v4 = boost::add_vertex(nw);

    TestNetworkD_small(){
        boost::add_edge(v1, v2, nw);
        boost::add_edge(v1, v3, nw);
        boost::add_edge(v4, v1, nw);
        boost::add_edge(v4, v2, nw);
    }
};

// -- Test get_rand functions -------------------------------------------------
BOOST_AUTO_TEST_CASE(test_get_rand)
{
    Config test_cfg = cfg["test_funcs"]["test_get_rand"];

    const std::vector<std::pair<int, int>> to_assert_int
            = get_as<std::vector<std::pair<int, int>>>("vals_int", test_cfg);
    for(const auto& val: to_assert_int) {
        const int rand = get_rand<int>(val, rng);
        BOOST_TEST(rand >= val.first);
        BOOST_TEST(rand <= val.second);
    }

    const std::vector<std::pair<double, double>> to_assert_double
            = get_as<std::vector<std::pair<double, double>>>("vals_double",
                                                                test_cfg);
    for(const auto& val: to_assert_int) {
        const double rand = get_rand<double>(val, rng);
        BOOST_TEST(rand >= val.first);
        BOOST_TEST(rand <= val.second);
    }

    const std::vector<std::pair<double, double>> to_assert_fail
            = get_as<std::vector<std::pair<double, double>>>("assert_fail",
                                                                test_cfg);
    for(const auto& val: to_assert_fail) {
        TestTools::check_exception<std::invalid_argument>(
            [&](){
            get_rand<double>(val, rng);
            },
            "Error, invalid parameter range! Upper limit has to be "
            "higher than the lower limit."  // expected error message
        );
    }
}

// -- Test network utility functions ------------------------------------------
// Network type check
BOOST_AUTO_TEST_CASE(test_is_directed)
{
    BOOST_TEST(is_directed<NetworkDirected>());
    BOOST_TEST(!is_directed<NetworkUndirected>());
}

// Test the get_rand_neighbor function
BOOST_FIXTURE_TEST_CASE(test_get_rand_neighbor, TestNetworkU)
{
    for (const auto v : range<IterateOver::vertices>(nw)) {
        if (boost::out_degree(v, nw) != 0) {
            vertex w = get_rand_neighbor(v, nw, rng);
            // Test edge exists
            BOOST_TEST(edge(v, w, nw).second);

            // Test edge selection by probability works on undirected networks
            vertex x = select_neighbor(v, nw, uniform_prob_distr, rng);
            BOOST_TEST(edge(v, x, nw).second);
        }
    }
}

// Test weights are normalized and positive.
BOOST_FIXTURE_TEST_CASE(test_set_and_normalize_weights,
                        TestNetworkD,
                        * boost::unit_test::tolerance(1e-12))
{
    const std::vector<double> weighting =
        get_as<std::vector<double>>("weighting",
            cfg["test_funcs"]["test_set_and_normalize_weights"]);

    std::pair<double, double> opinion_range = {-5., 5.};
    for(const auto& w: weighting) {
        for (const auto v : range<IterateOver::vertices>(nw)) {
            nw[v].opinion = get_rand<double>(opinion_range, rng);
            set_and_normalize_weights(v, nw, w);
        }
        for (const auto v : range<IterateOver::vertices>(nw)) {
            double sum_of_weights = 0;
            for (const auto e : range<IterateOver::out_edges>(v, nw)) {
                BOOST_TEST(nw[e].weight >= 0);
                sum_of_weights += nw[e].weight;
            }
            if (boost::out_degree(v, nw) != 0) {
                BOOST_TEST(sum_of_weights == 1.);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(test_select_neighbour,
                        TestNetworkD_small,
                        * boost::unit_test::tolerance(1.0))
{
    const std::vector<double> init_opinions =
        get_as<std::vector<double>>(
            "init_opinions",
            cfg["test_funcs"]["test_select_neighbour"]
        );

    const std::vector<double> weighting =
        get_as<std::vector<double>>("weighting",
            cfg["test_funcs"]["test_select_neighbour"]);

    for(const auto& w: weighting) {

        for (size_t i = 0; i<boost::num_vertices(nw); ++i) {
            nw[i].opinion = init_opinions.at(i);
        }

        for (const auto v : range<IterateOver::vertices>(nw)) {
            set_and_normalize_weights(v, nw, w);
        }

        const int num_steps = 100000;

        // Check interaction partner selection probability is same as ratio of
        // opinions
        int v2_selected = 0;
        int v3_selected = 0;
        for (int i=0; i<num_steps; ++i) {
            vertex w = select_neighbor(v1, nw, uniform_prob_distr, rng);
            if (w == v2) { ++v2_selected; }
            else if (w == v3) { ++v3_selected; }
        }

        BOOST_TEST((1.0*v2_selected)/v3_selected
                 == exp(-w*(fabs(nw[v2].opinion-nw[v1].opinion)
                         -fabs(nw[v3].opinion-nw[v1].opinion)))
        );

        // Repeat for vertex 4
        int v1_selected = 0;
        v2_selected = 0;
        for (int i=0; i<num_steps; ++i) {
            vertex w = select_neighbor(v4, nw, uniform_prob_distr, rng);
            if (w == v1) { ++v1_selected; }
            else if (w == v2) { ++v2_selected; }
        }

        BOOST_TEST((1.0*v1_selected)/v2_selected
                 == exp(-w*(fabs(nw[v1].opinion-nw[v4].opinion)
                         -fabs(nw[v2].opinion-nw[v4].opinion)))
        );
    }
}
} // namespace Utopia::Models::Opinionet::Utils
