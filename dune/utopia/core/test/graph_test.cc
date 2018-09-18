#include <dune/utopia/base.hh>
#include <dune/utopia/core/graph.hh>
#include <cassert>
#include <iostream>

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


/// Test the function that creates a small-world graph
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
    assert(num_vertices * mean_degree /2 == boost::num_edges(g));

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


/// Test the function that creates a scale-free graph
void test_create_scale_free_graph(){
    // Create a random number generator
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const int num_vertices = 200;
    const int mean_degree = 8;

    // Create test graph
    auto g = create_scale_free_graph<G>(    num_vertices,
                                            mean_degree,
                                            rng); 

    // Assert that the number of vertices and edges is correct
    assert(num_vertices == boost::num_vertices(g));
    assert(num_vertices * mean_degree / 2 == boost::num_edges(g));

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

    /// Test catching exceptions
    // Case: directed Graph

    using G_directed = boost::adjacency_list<
        boost::vecS,        // edge container
        boost::vecS,        // vertex container
        boost::directedS,
        Vertex>;             // vertex struct

    try
    {
        auto g_dir = create_scale_free_graph<G_directed> (num_vertices, mean_degree, rng);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception" << std::endl;
    }
    catch (...){
<<<<<<< HEAD
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
=======
        std::cerr   << "Caught unexpected exception in create_scale_free_graph function test." 
                    << std::endl;
        throw -1;
>>>>>>> 8c4cc2e2427658b4110e0d20164faa9d6980f13f
    }

    // Case: mean degree greater than number of vertices
    try
    {
        auto g_fail = create_scale_free_graph<G> (5, 6, rng);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception" << std::endl;
    }
    catch (...){
<<<<<<< HEAD
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
=======
        std::cerr   << "Caught unexpected exception in create_scale_free_graph function test." 
                    << std::endl;    
        throw -1;
>>>>>>> 8c4cc2e2427658b4110e0d20164faa9d6980f13f
    }

    // Case: mean degree is odd
    try
    {
        auto g_fail = create_scale_free_graph<G> (10, 5, rng);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception" << std::endl;
    }
    catch (...){
<<<<<<< HEAD
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
=======
        std::cerr   << "Caught unexpected exception in create_scale_free_graph function test." 
                    << std::endl;
        throw -1;
>>>>>>> 8c4cc2e2427658b4110e0d20164faa9d6980f13f
    }
}


/// Test the k-regular graph creation
void test_create_k_regular_graph(){
    // Create four different regular graph test cases
    const int num_vertices_even = 100;
    const int num_vertices_odd = 99;
    const int degree_odd = 3; 
    const int degree_even = 4;

    auto g_eo = create_k_regular_graph<G>(num_vertices_even, degree_odd);
    auto g_ee = create_k_regular_graph<G>(num_vertices_even, degree_even);
    auto g_oe = create_k_regular_graph<G>(num_vertices_odd, degree_even);
  
    // Check that all vertices have the same expected degree
    for (auto [v, v_end] = boost::vertices(g_eo); v!=v_end; ++v){
        assert (boost::num_vertices(g_eo) == num_vertices_even);
        assert (boost::out_degree(*v, g_eo) == degree_odd);
    }

    // Check that all vertices have the same expected degree
    for (auto [v, v_end] = boost::vertices(g_ee); v!=v_end; ++v){
        assert (boost::num_vertices(g_ee) == num_vertices_even);
        assert (boost::out_degree(*v, g_ee) == degree_even);
    }

    // Check that all vertices have the same expected degree
    for (auto [v, v_end] = boost::vertices(g_oe); v!=v_end; ++v){
        assert (boost::num_vertices(g_oe) == num_vertices_odd);
        assert (boost::out_degree(*v, g_oe) == degree_even);
    }

    try{
        auto g_oo = create_k_regular_graph<G>(num_vertices_odd, degree_odd);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception." << std::endl;
    }
    catch (...){
<<<<<<< HEAD
        throw std::runtime_error("Caught an unexpected exception in "
                                 "create_k_regular_graph function test.");
=======
        std::cerr << "Caught an unexpected exception in create_k_regular_graph"
                     "function test." << std::endl;
        throw -1;
>>>>>>> 8c4cc2e2427658b4110e0d20164faa9d6980f13f
    }
}

int main(int argc, char* argv[])
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);

        test_create_k_regular_graph();
        test_create_random_graph();
        test_create_small_world_graph();
        test_create_scale_free_graph();
        
        return 0;
    }
<<<<<<< HEAD
    catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
=======
    catch (const int i){
        std::cerr << "Caught all expected exceptions" << std::endl;
        return 0;
>>>>>>> 8c4cc2e2427658b4110e0d20164faa9d6980f13f
    }
    catch (...)
    {
        std::cerr << "Unexpected exceptions occured in the graph test!" << std::endl;
        return 1;
    }
}