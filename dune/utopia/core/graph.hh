#ifndef UTOPIA_CORE_GRAPH_UTILS_HH
#define UTOPIA_CORE_GRAPH_UTILS_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
// #include <random>

namespace Utopia {
namespace Graph {

/// Create a random graph
/** This function uses the create_random_graph function from the boost graph library.
 * It uses the Erös-Rényi algorithm to generate the random graph.
 *
 * @tparam Graph The graph type
 * @tparam RNG The random number generator type
 * @param num_vertices The total number of vertices
 * @param num_edges The total number of edges
 * @param allow_parallel Allow parallel edges within the graph
 * @param self_edges Allows a vertex to be connected to itself
 * @param rng The random number generator
 * @return Graph The random graph
 */
template<typename Graph, typename RNG>
Graph create_random_graph(  const std::size_t num_vertices, 
                            const std::size_t num_edges,
                            const bool allow_parallel,
                            const bool self_edges,
                            RNG& rng)
{
    // Create an empty graph
    Graph g; 

    // Create a random graph using the Erdös-Rényi algorithm
    // The graph is undirected
    boost::generate_random_graph(g, num_vertices, num_edges , rng, allow_parallel, self_edges); 

    // Return graph
    return g;
}


/// Create a scale-free graph
/** This function generates a scale-free graph using the Barabási-Albert model.
 * 
 * @tparam Graph THe graph type
 * @tparam RNG The random number generator type
 * @param num_vertices The total number of vertices
 * @param mean_degree The mean degree
 * @param rng The random number generator
 * @return Graph The scale-free graph
 */
template <typename Graph, typename RNG>
Graph create_scale_free_graph(  const std::size_t num_vertices,
                                const std::size_t mean_degree,
                                RNG& rng)
{
    // Create empty graph
    Graph g;
    
    // Creating a link effectively adds two counts to the number of total degrees. 
    // To counteract counting twice, the mean degree is halfed
    auto m = mean_degree/2.;

    if (num_vertices < m){
        throw std::runtime_error("The desired mean degree is too high."
                                "There are not enough vertices to place all edges.");
    }
    else{
        // Define helper variables
        auto num_edges = 0;
        auto deg_ignore = 0;
        auto m0 = m*2.;

        
        // Create a small spawning network with m0+1 completely connected vertices.
        for (auto i = 0; i<m0+1; ++i){
            for (auto j = 0; j<m0+1; ++j){
                if (i!=j){
                    // Increase the number of edges only if an edge was added
                    if (boost::add_edge(i,j, g).second == true) ++num_edges;
                }
            }
        }

        // Add i times a vertex and connect it randomly but weighted to the existing vertices
        std::uniform_real_distribution<> rand(0, 1);
        for (auto i = 0; i<(num_vertices - m0 - 1); ++i){
            // Add a new vertex
            auto new_vertex = boost::add_vertex(g);
            auto edges_added = 0;

            // Add the desired number of edges
            for (auto edge = 0; edge<m; ++edge){
                // Keep track of the probability
                auto prob = 0.;
                // Calculate a random number
                auto rand_num = rand(rng);

                // Loop through every vertex and look if it can be connected
                for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
                    // Check whether the notes are already connected
                    if (!boost::edge(new_vertex, *v, g).second){
                        prob += boost::out_degree(*v, g) / ((2. * num_edges) - deg_ignore);
                    }

                    if (rand_num <= prob){
                        // create an edge between the two vertices
                        deg_ignore = boost::out_degree(*v, g);
                        boost::add_edge(new_vertex, *v, g);

                        ++edges_added;
                        break;
                    }
                }
            }
            num_edges+=edges_added;
        }
    }
    return g;
}


} // namespace Graph
} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_UTILS_HH
