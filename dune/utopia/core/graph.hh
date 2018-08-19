#ifndef UTOPIA_CORE_GRAPH_UTILS_HH
#define UTOPIA_CORE_GRAPH_UTILS_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>

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




} // namespace Graph
} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_UTILS_HH
