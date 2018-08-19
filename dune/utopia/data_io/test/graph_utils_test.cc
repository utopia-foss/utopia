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


void test_save_graph()
{
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
    using Graph1 =  boost::adjacency_list<
                        boost::vecS,             // edge container
                        boost::listS,            // vertex container
                        boost::undirectedS,
                        // boost::property<boost::vertex_index_t, std::size_t>,
                        // boost::no_property,
                        // boost::property<std::size_t(), std::size_t>,
                        Vertex, Edge>;

    // using Graph_vv_u = boost::adjacency_list<
    //                     boost::vecS,            // edge container
    //                     boost::vecS,            // vertex container
    //                     boost::undirectedS,
    //                     Vertex, Edge>;

    // using Graph_ss_u = boost::adjacency_list<
    //                     boost::setS,            // edge container
    //                     boost::setS,            // vertex container
    //                     boost::undirectedS,
    //                     Vertex, Edge>;
    
    // using Graph_vv_d =  boost::adjacency_list<
    //                     boost::vecS,            // edge container
    //                     boost::vecS,            // vertex container
    //                     boost::directedS,
    //                     Vertex, Edge>;


    auto g1 = create_and_initialize_test_graph<Graph1>(10,3);


    using VertexIdMap = boost::property_map<Graph1, std::size_t Vertex::*>::type;
    VertexIdMap vertex_id = get(&Vertex::id, g1);

    // for (auto v : boost::make_iterator_range(vertices(g1))){
    //     std::cout << boost::get(boost::vertex_index, g1, v) << std::endl; 
    // }

    // Create a test HDFFile and a HDFGroup 
    auto hdf = Utopia::DataIO::HDFFile("graphtestfile.h5","a");
    auto grp = hdf.open_group("testgroup");

    Utopia::DataIO::save_graph<true>(g1, grp, "test1", vertex_id);
    Utopia::DataIO::save_graph<true>(g1, grp, "test1");

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
