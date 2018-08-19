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


/// Test the function that creates a random graph
void test_create_small_world_graph(){
    // Create a random number generator
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const int num_vertices = 100;
    const int mean_degree = 2;
    const double p_rewire = 0.6;

    // Create test graph
    auto g = create_small_world_graph<G>(   num_vertices,
                                    mean_degree,
                                    p_rewire,
                                    rng); 

    // Assert that the number of vertices and edges is correct
    assert(num_vertices == boost::num_vertices(g));
    assert(num_vertices * mean_degree == boost::num_edges(g));

    // Check that at least one vertex does not have connectivity mean_degree any more
    bool at_least_one_rewired = false;
    for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
        if (boost::out_degree(*v, g) != mean_degree){
            at_least_one_rewired = true;
            break;
        }
    }
    assert(at_least_one_rewired == true);

    // Assert that the state of the rng has changed.
    assert(rng!=rng_copy);
}


/// Test the function that creates a random graph
void test_create_scale_free_graph(){
    // Create a random number generator
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const int num_vertices = 200;
    const int mean_degree = 4;

    // Create test graph
    auto g = create_scale_free_graph<G>(    num_vertices,
                                            mean_degree,
                                            rng); 

    // Assert that the number of vertices and edges is correct
    assert(num_vertices == boost::num_vertices(g));
    assert(num_vertices * mean_degree == boost::num_edges(g));

    // Check that at least one vertex has more than 10 edges
    bool at_least_one_more_than_ten_edges = false;
    for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
        if (boost::out_degree(*v, g) > 10){
            at_least_one_more_than_ten_edges = true;
            break;
        }
    }
    assert(at_least_one_more_than_ten_edges == true);

    // Assert that the state of the rng has changed.
    assert(rng!=rng_copy);
}




int main(int argc, char* argv[])
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);

        test_create_random_graph();
        test_create_small_world_graph();
        
        return 0;
    }

    catch (...)
    {
        std::cerr << "Exeception occured!" << std::endl;
        return 1;
    }
}