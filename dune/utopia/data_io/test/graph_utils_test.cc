#include <cassert>
#include <iostream>
#include <random>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/properties.hpp>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/utopia/data_io/graph_utils.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdffile.hh>


using namespace Utopia;


/// Vertex struct containing some properties
struct Vertex {
    int test_int;
    double test_double;
    std::size_t id;
};

/// Edge struct with property
struct Edge {
    float weight;
};


// Create different graph types to be tested.
using Graph_vertvecS_edgevecS_undir =  boost::adjacency_list<
                                            boost::vecS,        // edge container
                                            boost::vecS,        // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertlistS_edgelistS_undir =  boost::adjacency_list<
                                            boost::listS,       // edge container
                                            boost::listS,       // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertsetS_edgesetS_undir =  boost::adjacency_list<
                                            boost::setS,        // edge container
                                            boost::setS,        // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertvecS_edgevecS_dir =  boost::adjacency_list<
                                            boost::vecS,        // edge container
                                            boost::vecS,        // vertex container
                                            boost::directedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct


/// Creates a small test graph
template<typename GraphType>
GraphType create_and_initialize_test_graph(const int num_vertices, const int num_edges){
    GraphType g;

    std::mt19937 rng(42);

    // Add vertices and initialize
    for (int i = 0; i < num_vertices; i++){
        // Add vertex
        auto v = add_vertex(g);
        
        // Initialize vertex
        g[v].test_int = 42;
        g[v].test_double = 2.3;
        g[v].id = 10;
    }

    // Randomly add edges
    for (int i = 0; i < num_edges; i++){
        // Add random edge
        auto v1 = random_vertex(g, rng);
        auto v2 = random_vertex(g, rng);
        auto e = add_edge(v1,v2,g);
        
        // Initialize edge
        g[e.first].weight = 0.5;
    }

    // Return the graph
    return g;
}


/// Test the save graph functionality
void test_save_graph()
{
    // Create a test HDFFile and a HDFGroup 
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","a");
    auto grp = hdf.open_group("testgroup");

    // Test case 1: 
    //     vertex container: boost::vecS
    //     list container:   boost::vecS

    // Set up graph
    auto g_vvu = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_undir>(10,3);

    // Save the graph ...
    // ...with edges
    Utopia::DataIO::save_graph<true>(g_vvu, grp, "g");
    // ... without edges
    Utopia::DataIO::save_graph<false>(g_vvu, grp, "g_vvu_noedges");

    // Test case 2: 
    //     vertex container: boost::listS
    //     list container:   boost::listS

    // Set up graph
    auto g_llu = create_and_initialize_test_graph<Graph_vertlistS_edgelistS_undir>(10,3);

    // In the case of boost::listS, the graph does not store a boost::vertex_index internally.
    // Therefore, the save_graph function would not work.
    // To overcome this, the user needs to define an id in the Vertex struct itself.
    // It can be given to the save_data function through a property map, which 
    // needs to be defined.
    // NOTE: The user needs to adapt this id, such that it actually represents a unique id
    //       for every agent within the model dynamics!
    using VertexIdMapLLU = boost::property_map<Graph_vertlistS_edgelistS_undir, 
                                            std::size_t Vertex::*
                                            >::type;
    VertexIdMapLLU vertex_id_map_llu = get(&Vertex::id, g_llu);

    // Save the graph ...
    // ... with edges
    Utopia::DataIO::save_graph<true>(g_llu, grp, "g_llu", vertex_id_map_llu);
    // ... without edges
    Utopia::DataIO::save_graph<false>(g_llu, grp, "g_llu_noedges", vertex_id_map_llu);


    // Test case 3: 
    //     vertex container: boost::setS
    //     list container:   boost::setS

    // Set up graph
    auto g_ssu = create_and_initialize_test_graph<Graph_vertsetS_edgesetS_undir>(10,3);

    // In the case of boost::setS, the graph does not store a boost::vertex_index internally.
    // Therefore, the save_graph function would not work.
    // To overcome this, the user needs to define an id in the Vertex struct itself.
    // It can be given to the save_data function through a property map, which 
    // needs to be defined.
    // NOTE: The user needs to adapt this id, such that it actually represents a unique id
    //       for every agent within the model dynamics!
    using VertexIdMapSSU = boost::property_map<Graph_vertsetS_edgesetS_undir, 
                                            std::size_t Vertex::*
                                            >::type;
    VertexIdMapSSU vertex_id_map_ssu = get(&Vertex::id, g_ssu);

    // Save the graph ...
    // ... with edges
    Utopia::DataIO::save_graph<true>(g_ssu, grp, "g_ssu", vertex_id_map_ssu);
    // ... without edges
    Utopia::DataIO::save_graph<false>(g_ssu, grp, "g_ssu_noedges", vertex_id_map_ssu);


    // Test case 4: 
    //     vertex container: boost::vecS
    //     list container:   boost::vecS
    //     undirected graph

    // Set up graph
    auto g_vvd = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_dir>(10,3);

    // Save the graph ...
    // ... with edges 
    Utopia::DataIO::save_graph<true>(g_vvd, grp, "g_vvd");
    // ... without edges
    Utopia::DataIO::save_graph<false>(g_vvd, grp, "g_vvd_noedges");
}


int main(int argc, char** argv)
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);

        test_save_graph();

        std::cout << "Tests successful." << std::endl;
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
