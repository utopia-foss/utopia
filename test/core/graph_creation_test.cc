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
    std::vector<std::string> models {"regular",
                                    "ErdosRenyi", 
                                    "ErdosRenyiP", 
                                    "ErdosRenyiS", 
                                    "ErdosRenyiPS", 
                                    "WattsStrogatz", 
                                    "BarabasiAlbert",
                                    "BollobasRiordan"
                                    };
    // .. undirected graphs ...................................................
    // A map in which to store the graph for each model
    std::vector<Graph> g_vec;

    // Fill the map with undirected graphs for each model
    for (const auto& m : models){
        // Bollobas-Riordan scale-free graphs require directed graphs.
        if (m == "BollobasRiordan"){
            // undirected Graph should not work
            BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(cfg[m], rng),
                              std::runtime_error);
        }
        else{
            g_vec.push_back(Utopia::Graph::create_graph<Graph>(cfg[m], rng));
        }
    }

    // Check that all created graphs have 10 vertices and edges (mean_degree=2)
    for (const auto& g : g_vec){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 10);
    }

    // .. directed graphs ...................................................
    // A map in which to store the graph for each model
    std::vector<DiGraph> g_vec_dir;

    // Fill the map with directed graphs for each model
    for (const auto& m : models){
        // These graph models require undirected graphs
        if ((m == "BarabasiAlbert") or 
            (m == "regular")){
            // undirected Graph should not work
            BOOST_CHECK_THROW(Utopia::Graph::create_graph<DiGraph>(cfg[m], rng),
                              std::runtime_error);
        }
        else if (m == "BollobasRiordan") {
            auto g = Utopia::Graph::create_graph<DiGraph>(cfg[m], rng);
            BOOST_TEST(boost::num_vertices(g) == 10);
        }
        else{
            g_vec_dir.push_back(Utopia::Graph::create_graph<DiGraph>(cfg[m], 
                                                                     rng));
        }
    }

    // Check that all created graphs have 10 vertices and 20 edges (mean_degree=2)
    for (const auto& g : g_vec_dir){
        BOOST_TEST(boost::num_vertices(g) == 10);
        BOOST_TEST(boost::num_edges(g) == 20);
    }

    // .. failing graphs ......................................................
    BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(cfg["Fail"], rng),
                      std::invalid_argument);

    BOOST_CHECK_THROW(Utopia::Graph::create_graph<Graph>(cfg["missing_arg"], 
                                                         rng),
                      std::runtime_error);
}
