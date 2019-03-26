#include <cassert>
#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>

#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>

using namespace Utopia::Graph;


/// The test vertex struct
struct Vertex{
    int i;
};

/// The test graph types
using G = boost::adjacency_list<
                    boost::vecS,        // edge container
                    boost::vecS,        // vertex container
                    boost::undirectedS,
                    Vertex>;            // vertex struct

using G_directed = boost::adjacency_list<
                    boost::vecS,        // edge container
                    boost::vecS,        // vertex container
                    boost::bidirectionalS,
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
    auto g = create_random_graph<G>(num_vertices,
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
    try
    {
        auto g_dir = create_scale_free_graph<G_directed> (num_vertices, mean_degree, rng);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception" << std::endl;
    }
    catch (...){
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
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
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
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
        throw std::runtime_error("Caught unexpected exception in "
                                 "create_scale_free_graph function test.");
    }
}


/// Test the function that creates a directed scale-free graph
void test_create_scale_free_directed_graph(){
    // Create a random number generator
    Utopia::DefaultRNG rng;
    Utopia::DefaultRNG rng_copy = rng;

    // Set graph properties
    const std::size_t num_vertices = 200;
    const double alpha = 0.2;
    const double beta = 0.8;
    const double gamma = 0.;
    const double del_in = 0.;
    const double del_out = 0.5;

    // Create test graph
    auto g = create_scale_free_directed_graph<G_directed>(  num_vertices,
                                                            alpha,
                                                            beta,
                                                            gamma,
                                                            del_in,
                                                            del_out,
                                                            rng);

    // Assert that the number of vertices is correct
    assert(num_vertices == boost::num_vertices(g));

    // Assert that only tree vertices (the initial network)
    // have an in-degree unequal Zero.
    auto count = 0;
    for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
        if (boost::in_degree(*v, g) > 0) {
            ++count;
        }
    }
    assert(count == 3);

    // Check that at least one vertex has more than 10 in-edges
    bool at_least_one_more_than_ten_edges = false;
    for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
        if (boost::in_degree(*v, g) > 10){
            at_least_one_more_than_ten_edges = true;
            break;
        }
    }
    assert(at_least_one_more_than_ten_edges == true);

    // Assert that the state of the rng has changed.
    assert(rng!=rng_copy);

    /// Test catching exceptions
    // Case: undirected Graph
    try
    {
        auto g_dir = create_scale_free_directed_graph<G>(   num_vertices,
                                                            alpha,
                                                            beta,
                                                            gamma,
                                                            del_in,
                                                            del_out,
                                                            rng);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Caught expected exception" << std::endl;
    }
    catch (...){
        throw std::runtime_error("Caught unexpected exception in "
                        "create_directed_scale_free_graph function test.");
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
        throw std::runtime_error("Caught an unexpected exception in "
                                 "create_k_regular_graph function test.");
    }
}

int main()
{
    try
    {
        test_create_k_regular_graph();
        test_create_random_graph();
        test_create_small_world_graph();
        test_create_scale_free_graph();
        test_create_scale_free_directed_graph();
        
        return 0;
    }
    catch (const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unexpected exceptions occurred in the graph test!" << std::endl;
        return 1;
    }
}
