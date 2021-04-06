#define BOOST_TEST_MODULE graph load test

#include <variant>
#include <type_traits>
#include <string>
#include <sstream>

#include <boost/test/included/unit_test.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include <utopia/data_io/graph_load.hh>
#include <utopia/core/graph.hh>
#include <utopia/core/types.hh>
#include <utopia/core/testtools.hh>

using namespace Utopia;
using namespace Utopia::DataIO;
using namespace Utopia::TestTools;

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure () : BaseInfrastructure<>("graph_load_test.yml") {}
};

// The Container structure used in Utopia (does not work to load into just yet)
struct VertexState {};
using VertexTraits = Utopia::GraphEntityTraits<VertexState>;
using Vertex = GraphEntity<VertexTraits>;

// Currently, the reading into bundled properties only works if the struct
// (EdgeState) is placed bare and naked into the boost::adj_list template (see
// below). The Utopia Edge::State::weight resp. g[e].state.weight syntax does
// not work, if you want to be able to load properties into the bundle.
struct EdgeState {
    double weight = 2.;
    int some_int = 1;
};



struct GraphProperties {};

// The test graph types. Adj. matrices are not mutable and cannot be read to
// They can be copied to, though, which must be done if a loading into AM is
// desired.
using G_vec_dir = boost::adjacency_list<
                    boost::vecS,         // edge cont
                    boost::vecS,         // vertex cont
                    boost::directedS,
                    Vertex, // Following line has `EdgeState`, not `Edge`, to
                            // allow for loading into edge bundle properties.
                    boost::property<boost::edge_weight_t, double, EdgeState>,
                    GraphProperties>;

using G_list_dir = boost::adjacency_list<
                    boost::listS,        // edge cont
                    boost::listS,        // vertex cont
                    boost::directedS,
                    Vertex,
                    boost::property<boost::edge_weight_t, double, EdgeState>,
                    GraphProperties>;

using G_vec_undir = boost::adjacency_list<
                    boost::vecS,         // edge cont
                    boost::vecS,         // vertex cont
                    boost::undirectedS,
                    Vertex,
                    boost::property<boost::edge_weight_t, double, EdgeState>,
                    GraphProperties>;

using G_list_undir = boost::adjacency_list<
                    boost::listS,        // edge cont
                    boost::listS,        // vertex cont
                    boost::undirectedS,
                    Vertex,
                    boost::property<boost::edge_weight_t, double, EdgeState>,
                    GraphProperties>;

using directed_graph_types = std::tuple<G_vec_dir, G_list_dir>;

using undirected_graph_types = std::tuple<G_vec_undir, G_list_undir>;


BOOST_FIXTURE_TEST_SUITE(with_cfg, Infrastructure)


BOOST_AUTO_TEST_CASE_TEMPLATE(test_load_directed_graphs,
                                Graph, directed_graph_types) {

    test_config_callable(
        [](const auto& cfg){

            // Get the Configs
            const auto cfg_lff = get_as<Config>("load_from_file", cfg);
            const auto cfg_test = get_as<Config>("params_for_test", cfg);
            const auto exp_cfg = get_as<Config>("expected", cfg);

            // Load test parameters
            const auto store_weights
                        = get_as<std::string>("store_weights", cfg_test);
            const auto store_some_int
                    = get_as<std::string>("store_some_int", cfg_test, "");

            // Load expected values
            const auto exp_num_vertices = get_as<int>("num_vertices", exp_cfg);
            const auto exp_num_edges = get_as<int>("num_edges", exp_cfg);
            const auto exp_built_in_weights
                        = get_as<double>("sum_built_in_weights", exp_cfg);
            const auto exp_bundle_weights
                        = get_as<double>("sum_bundle_weights", exp_cfg);
            const auto exp_bundle_some_int
                        = get_as<double>("sum_bundle_some_int", exp_cfg, 4);

            // Instantiate the (empty) graph
            Graph g(0);

            // Define dynamic property maps
            boost::dynamic_properties pmaps(boost::ignore_other_properties);
            // Add the built-in or bundle as a source for the weight pmap

            if (store_weights == "bundle") {
                // Remark for future improvement: It would be desirable for the
                // "bundle" case to work with the typical utopia graph entity
                // containers, if we find a way to translate the
                // `&EdgeState::weight` to it, that would be perfect.
                pmaps.property("weight", boost::get(&EdgeState::weight, g));
            } else if (store_weights == "built-in") {
                pmaps.property("weight", boost::get(boost::edge_weight, g));
            }
            if (store_some_int == "bundle") {
                pmaps.property("some_int",
                    boost::get(&EdgeState::some_int, g));
            } 

            // Load the graph
            g = GraphLoad::load_graph<Graph>(cfg_lff, pmaps);

            // Now sum over bundled weights, some_int and over built-in weight
            auto bundle_weight_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                // For standard utopia syntax, the following is to be replaced
                // by g[*e_it].state.weight, only that the above
                // &Edge::State::weight does not work as yet.
                bundle_weight_sum
                            += g[*e_it].weight;
            }

            auto bundle_some_int_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                // For standard utopia syntax, the following is to be replaced
                // by g[*e_it].state.some_int, only that the above
                // &Edge::State::some_int does not work as yet.
                bundle_some_int_sum
                            += g[*e_it].some_int;
            }

            auto built_in_weight_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                built_in_weight_sum
                            += boost::get(boost::edge_weight_t(), g, *e_it);
            }

            // 5 Tests
            BOOST_TEST(boost::num_vertices(g) == exp_num_vertices);

            BOOST_TEST(boost::num_edges(g) == exp_num_edges);

            BOOST_TEST(built_in_weight_sum == exp_built_in_weights);

            BOOST_TEST(bundle_weight_sum == exp_bundle_weights);

            BOOST_TEST(bundle_some_int_sum == exp_bundle_some_int);

        },
        // The YAML mapping that holds all _directed_ test cases
        cfg["directed_cases"], "directed_cases", {__LINE__, __FILE__}
    );        
};


BOOST_AUTO_TEST_CASE_TEMPLATE(test_load_undirected_graphs,
                                Graph, undirected_graph_types) {

    test_config_callable(
        [](const auto& cfg){

            // Get the Configs
            const auto cfg_lff = get_as<Config>("load_from_file", cfg);
            const auto cfg_test = get_as<Config>("params_for_test", cfg);
            const auto exp_cfg = get_as<Config>("expected", cfg);

            // Load test parameters
            const auto store_weights
                        = get_as<std::string>("store_weights", cfg_test);
            const auto store_some_int
                    = get_as<std::string>("store_some_int", cfg_test, "");

            // Load expected values
            const auto exp_num_vertices = get_as<int>("num_vertices", exp_cfg);
            const auto exp_num_edges = get_as<int>("num_edges", exp_cfg);
            const auto exp_built_in_weights
                        = get_as<double>("sum_built_in_weights", exp_cfg);
            const auto exp_bundle_weights
                        = get_as<double>("sum_bundle_weights", exp_cfg);
            const auto exp_bundle_some_int
                        = get_as<int>("sum_bundle_some_int", exp_cfg, 4);

            // Instantiate the (empty) graph
            Graph g(0);

            // Define dynamic property maps
            boost::dynamic_properties pmaps(boost::ignore_other_properties);
            // Add the built-in or bundle as a source for the weight pmap

            if (store_weights == "bundle") {
                // Remark for future improvement: It would be desirable for the
                // "bundle" case to work with the typical utopia graph entity
                // containers, if we find a way to translate the
                // `&EdgeState::weight` to it, that would be perfect.
                pmaps.property("weight", boost::get(&EdgeState::weight, g));
            } else if (store_weights == "built-in") {
                pmaps.property("weight", boost::get(boost::edge_weight, g));
            }
            if (store_some_int == "bundle") {
                pmaps.property("some_int",
                    boost::get(&EdgeState::some_int, g));
            }

            // Load the graph
            g = GraphLoad::load_graph<Graph>(cfg_lff, pmaps);

            // Now sum over bundled weights, some_int and over built-in weight
            auto bundle_weight_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                // For standard utopia syntax, the following is to be replaced
                // by g[*e_it].state.weight, only that the above
                // &Edge::State::weight does not work as yet.
                bundle_weight_sum
                            += g[*e_it].weight;
            }

            auto bundle_some_int_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                // For standard utopia syntax, the following is to be replaced
                // by g[*e_it].state.some_int, only that the above
                // &Edge::State::some_int does not work as yet.
                bundle_some_int_sum
                            += g[*e_it].some_int;
            }
            
            auto built_in_weight_sum = 0.;
            for(auto [e_it, e_end] = boost::edges(g); e_it != e_end; ++e_it) {
                built_in_weight_sum
                            += boost::get(boost::edge_weight_t(), g, *e_it);
            }

            // 5 Tests
            BOOST_TEST(boost::num_vertices(g) == exp_num_vertices);

            BOOST_TEST(boost::num_edges(g) == exp_num_edges);

            BOOST_TEST(built_in_weight_sum == exp_built_in_weights);

            BOOST_TEST(bundle_weight_sum == exp_bundle_weights);

            BOOST_TEST(bundle_some_int_sum == exp_bundle_some_int);

        },
        // The YAML mapping that holds all _undirected_ test cases
        cfg["undirected_cases"], "undirected_cases", {__LINE__, __FILE__}
    );        
};


BOOST_AUTO_TEST_SUITE_END() // with cfg
