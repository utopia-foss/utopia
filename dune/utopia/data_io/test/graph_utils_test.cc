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
GraphType create_test_graph(const int num_vertices, const int num_edges){
    GraphType g;

    std::mt19937 rng(42);

    // Add vertices
    for (int i = 0; i < num_vertices; i++){
        add_vertex(g);
    }

    // Randomly add edges
    for (int i = 0; i < num_edges; i++){
        auto v1 = random_vertex(g, rng);
        auto v2 = random_vertex(g, rng);

        add_edge(v1,v2,g);
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


    std::size_t id;
    // Create different graph types to be tested.
    using Graph1 =  boost::adjacency_list<
                        boost::listS,            // edge container
                        boost::vecS,            // vertex container
                        boost::undirectedS,
                        // boost::property<std::size_t(), std::size_t>,
                        Vertex, Edge>;

    using Graph_vv_u = boost::adjacency_list<
                        boost::vecS,            // edge container
                        boost::vecS,            // vertex container
                        boost::undirectedS,
                        Vertex, Edge>;

    using Graph_ss_u = boost::adjacency_list<
                        boost::setS,            // edge container
                        boost::setS,            // vertex container
                        boost::undirectedS,
                        Vertex, Edge>;
    
    using Graph_vv_d =  boost::adjacency_list<
                        boost::vecS,            // edge container
                        boost::vecS,            // vertex container
                        boost::directedS,
                        Vertex, Edge>;

    auto g1 = create_test_graph<Graph1>(10,3);

    for (auto v : boost::make_iterator_range(vertices(g1))){
        std::cout << boost::get(boost::vertex_index, g1, v) << std::endl; 
    }

    // Create a test HDFFile and a HDFGroup 
    auto hdf = Utopia::DataIO::HDFFile("graphtestfile.h5","a");
    auto grp = hdf.open_group("testgroup");

    // boost::BOOST_INSTALL_PROPERTY()



    // boost::static_property_map dp;
    // dp.property(boost::vertex_index_t(), boost::get(&Vertex::id, g1));

    Utopia::DataIO::save_graph<true, id>(g1, grp, "test1");
    // Utopia::DataIO::save_graph<true, id>(g1, grp, "test1");

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
