#define BOOST_TEST_MODULE graph creation test

#include <variant>
#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "utopia/core/graph.hh"
#include "utopia/core/types.hh"

// -- Fixtures ----------------------------------------------------------------

// .. Helper ..................................................................
struct GraphFixBase {
    /// The test vertex struct
    struct Vertex{
        int i;
    };

    // The vertex and edge container
    using vertex_cont = boost::vecS;
    using edge_cont = boost::vecS;
};

// .. Actual Fixtures .........................................................

struct GraphFix : GraphFixBase{
    using Graph = boost::adjacency_list<edge_cont, 
                                        vertex_cont, 
                                        boost::undirectedS, 
                                        Vertex>;
};


struct DiGraphFix : GraphFixBase{
    using DiGraph = boost::adjacency_list<edge_cont,
                                          vertex_cont,
                                          boost::bidirectionalS,
                                          Vertex>;
};

struct CreateGraphFix : GraphFix, DiGraphFix {
    YAML::Node cfg = YAML::LoadFile("graph_creation_test.yml");

    Utopia::DefaultRNG rng{};
};


// -- Tests -------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(create_graph, CreateGraphFix)
{
    // .. undirected graphs ...................................................
    // A map in which to store the graph for each model (for mean_degree>0)
    std::vector<Graph> g_vec;
    // A separate map for the graphs with mean degree zero
    std::vector<Graph> g_deg0_vec;

    // Fill the map with undirected graphs for each model
    for (const auto& model_map : cfg){
        // Unpack the returned key-value pairs
        const auto model = model_map.second["model"].as<std::string>();
        const auto& model_cfg = model_map.second;

        // Bollobas-Riordan scale-free graphs require directed graphs.
        if (model == "BollobasRiordan"){
            // undirected Graph should not work
            BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(model_cfg, rng),
                              std::runtime_error);
        }
        else if (model_cfg["mean_degree"].as<std::size_t>() == 0){
            g_deg0_vec.push_back(Utopia::Graph::create_graph<Graph>(model_cfg,
                                                                    rng));
        }
        else{
            g_vec.push_back(Utopia::Graph::create_graph<Graph>(model_cfg, rng));
        }
    }

    // Check that all created graphs with mean_degree>0 have 10 vertices and
    // 10 edges (mean_degree=2)
    for (const auto& g : g_vec){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 10);
    }

    // Check that all created graphs with mean_degree=0 have 10 vertices and
    // 0 edges
    for (const auto& g : g_deg0_vec){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 0);
    }

    // For regular graphs check that all vertices have the same correct number
    // of edges
    auto g_reg_undir = Utopia::Graph::create_graph<Graph>(cfg["regular"], rng);
    for (auto v : boost::make_iterator_range(
            boost::vertices(g_reg_undir).first,
            boost::vertices(g_reg_undir).second))
    {
        BOOST_TEST(boost::out_degree(v,g_reg_undir) 
                    == Utopia::get_as<unsigned>("mean_degree", cfg["regular"]));
    }
    

    // .. directed graphs ...................................................
    // A map in which to store the graph for each model (for mean_degree>0)
    std::vector<DiGraph> g_vec_dir;
    // A separate map for the graphs with mean degree zero
    std::vector<DiGraph> g_deg0_vec_dir;

    // Fill the map with directed graphs for each model
    // Fill the map with undirected graphs for each model
    for (const auto& model_map : cfg){
        // Unpack the returned key-value pairs
        const auto model = model_map.second["model"].as<std::string>();
        const auto& model_cfg = model_map.second;

        // These graph models require undirected graphs
        if (model == "BarabasiAlbert" or model == "BarabasiAlbertP"){
            // undirected Graph should not work
            BOOST_CHECK_THROW(Utopia::Graph::create_graph<DiGraph>(model_cfg, rng),
                              std::runtime_error);
        }
        else if (model == "BollobasRiordan") {
            auto g = Utopia::Graph::create_graph<DiGraph>(model_cfg, rng);
            BOOST_TEST(boost::num_vertices(g) == 10);
        }
        else if (model_cfg["mean_degree"].as<std::size_t>() == 0){
            g_deg0_vec_dir.push_back(Utopia::Graph::create_graph<DiGraph>(
                                                            model_cfg, rng));
        }
        else{
            g_vec_dir.push_back(Utopia::Graph::create_graph<DiGraph>(model_cfg, 
                                                                     rng));
        }
    }

    // Check that all created graphs with mean_degree>0 have 10 vertices and
    // 20 edges (mean_degree=2)
    for (const auto& g : g_vec_dir){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 20);
    }

    // Check that all created graphs with mean_degree=0 have 10 vertices and
    // 0 edges
    for (const auto& g : g_deg0_vec_dir){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 0);
    }

    // For regular graphs check that all vertices have the same correct number
    // of edges
    auto g_reg_dir = Utopia::Graph::create_graph<DiGraph>(cfg["regular"], rng);
    for (auto v : boost::make_iterator_range(
            boost::vertices(g_reg_dir).first,
            boost::vertices(g_reg_dir).second))
    {
        BOOST_TEST(boost::out_degree(v,g_reg_dir) 
                    == Utopia::get_as<unsigned>("mean_degree", cfg["regular"]));
    }

    // .. failing graphs ......................................................
    Utopia::DataIO::Config fail_cfg, missing_args_cfg;
    fail_cfg["model"] = "fail";
    missing_args_cfg["model"] = "regular";

    BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(fail_cfg, rng),
                      std::invalid_argument);

    BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(missing_args_cfg, 
                                                         rng),
                      std::runtime_error);
}
