#include <dune/utopia/base.hh>
#include <dune/utopia/core/graph.hh>
#include <cassert>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>

using namespace Utopia::Graph;

// Create a test vertex struct
struct Vertex{
    int i;
};

// Define test graph type
using G = boost::adjacency_list<
                    boost::vecS,        // edge container
                    boost::vecS,        // vertex container
                    boost::undirectedS,
                    Vertex>;             // vertex struct


/// Test the function that creates a random graph
void test_create_random_graph(){
    // Create a random number generator
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const int num_vertices = 10;
    const int num_edges = 20;

    // Create test graph
    auto g = create_random_graph<G>(   num_vertices,
                                    num_edges,
                                    false,
                                    false,
                                    rng); 

    // Assert that the number of vertices and edges is correct
    assert(num_vertices == boost::num_vertices(g));
    assert(num_edges == boost::num_edges(g));

    // Assert that the state of the rng has changed.
    assert(rng!=rng_copy);
}


int main(int argc, char* argv[])
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);


        test_create_random_graph();

        return 0;
    }

    catch (...)
    {
        std::cerr << "Exeception occured!" << std::endl;
        return 1;
    }
}