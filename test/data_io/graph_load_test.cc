#define BOOST_TEST_MODULE graph load test

#include <variant>
#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>

#include "utopia/data_io/graph_load.hh"
#include "utopia/core/graph.hh"
#include "utopia/core/types.hh"


namespace d_io = Utopia::DataIO;

struct Vertex {};

/// The test graph types
using G_vec = boost::adjacency_list<
                    boost::vecS,         // edge cont
                    boost::vecS,         // vertex cont
                    boost::undirectedS,
                    Vertex>;

using G_list = boost::adjacency_list<
                    boost::listS,        // edge cont
                    boost::listS,        // vertex cont
                    boost::undirectedS,
                    Vertex>;

struct ConfigFixture {
    YAML::Node cfg = YAML::LoadFile("graph_load_test.yml");

};

// -- Tests -------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(g_vec_load, ConfigFixture) {
    std::vector<G_vec> vec_graphs;

    for (const auto& node : cfg){
        const auto format
                = node.second["load_from_file"]["format"].as<std::string>();
        const auto& load_cfg = node.second["load_from_file"];


        vec_graphs.push_back(d_io::GraphLoad::load_graph<G_vec>(
                                    d_io::get_abs_filepath(load_cfg), format));
    }

    for (const auto& graph : vec_graphs){
        BOOST_TEST(boost::num_vertices(graph) >= 6);
    }

    BOOST_TEST(boost::num_edges(vec_graphs[0]) == 13);
    BOOST_TEST(boost::num_edges(vec_graphs[1]) == 9);
    BOOST_TEST(boost::num_edges(vec_graphs[2]) == 13);
    BOOST_TEST(boost::num_edges(vec_graphs[3]) == 9);

    for (auto v : boost::make_iterator_range(
            boost::vertices(vec_graphs[0]).first,
            boost::vertices(vec_graphs[0]).second)) {

        BOOST_TEST(boost::degree(v, vec_graphs[0]) 
                    >= 1);
    }
}

BOOST_FIXTURE_TEST_CASE(g_list_load, ConfigFixture) {
    std::vector<G_vec> list_graphs;

    for (const auto& node : cfg){
        const auto format
                = node.second["load_from_file"]["format"].as<std::string>();
        const auto& load_cfg = node.second["load_from_file"];

        list_graphs.push_back(d_io::GraphLoad::load_graph<G_vec>(
                                    d_io::get_abs_filepath(load_cfg), format));
    }

    for (const auto& graph : list_graphs){
        BOOST_TEST(boost::num_vertices(graph) >= 6);
    }

    BOOST_TEST(boost::num_edges(list_graphs[0]) == 13);
    BOOST_TEST(boost::num_edges(list_graphs[1]) == 9);
    BOOST_TEST(boost::num_edges(list_graphs[2]) == 13);
    BOOST_TEST(boost::num_edges(list_graphs[3]) == 9);

    for (auto v : boost::make_iterator_range(
            boost::vertices(list_graphs[0]).first,
            boost::vertices(list_graphs[0]).second)) {

        BOOST_TEST(boost::degree(v, list_graphs[0]) 
                    >= 1);
    }
}
