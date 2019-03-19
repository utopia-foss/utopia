#ifndef UTOPIA_CORE_GRAPH_HH
#define UTOPIA_CORE_GRAPH_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/graph/random.hpp>

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
    boost::generate_random_graph(g, num_vertices, num_edges , rng, allow_parallel, self_edges); 

    // Return graph
    return g;
}


/// Create a scale-free graph
/** This function generates a scale-free graph using the Barabási-Albert model.
 *  Multi-edges and self-loops are not allowed.
 *
 * @tparam Graph The graph type
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
    
    // Check for cases in which the algorithm does not work
    if (num_vertices < mean_degree){
        throw std::runtime_error("The desired mean degree is too high."
                                "There are not enough vertices to place all edges.");
    }
    else if (mean_degree % 2){
        throw std::runtime_error("The mean degree needs to be even!");
    }
    else if (boost::is_directed(g)){
        throw std::runtime_error("The scale-free generator algorithm currently "
                                "only works for undirected graphs. "
                                "The provided graph is directed.");
    }
    else{
        // Define helper variables
        auto num_edges = 0;
        auto deg_ignore = 0;

        // Create initial spawning network that is fully connected
        for (std::size_t i = 0; i<mean_degree+1; ++i){
            for (std::size_t j = 0; j<i; ++j){
                // Increase the number of edges only if an edge was added
                if (boost::add_edge(i, j, g).second == true) ++num_edges;
            }
        }

        // Keep account whether an edge has been added or not
        bool edge_added = false;

        // Add i times a vertex and connect it randomly but weighted 
        // to the existing vertices
        std::uniform_real_distribution<> rand(0, 1);
        for (std::size_t i = 0; i<(num_vertices - mean_degree - 1); ++i){
            // Add a new vertex
            auto new_vertex = boost::add_vertex(g);
            auto edges_added = 0;

            // Add the desired number of edges
            for (std::size_t edge = 0; edge<mean_degree/2; ++edge){
                // Keep track of the probability
                auto prob = 0.;
                // Calculate a random number
                auto rand_num = rand(rng);

                // Loop through every vertex and look if it can be connected
                for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v)
                {
                    // Until now, no edge has been added. Reset edge_added.
                    edge_added = false;
                    // accumulate the probability fractions
                    prob += boost::out_degree(*v, g) 
                            / ((2. * num_edges) - deg_ignore);

                    if (rand_num <= prob){
                        // Check whether the nodes are already connected
                        if (not boost::edge(new_vertex, *v, g).second){
                            // create an edge between the two vertices
                            deg_ignore = boost::out_degree(*v, g);
                            boost::add_edge(new_vertex, *v, g);

                            // Increase the number of added edges and keep
                            // track that an edge has been added
                            ++edges_added;
                            edge_added = true;
                            break;
                        }
                    }
                }

                // If no edge has been attached in one loop through the vertices
                // try again to attach an edge with another random number
                if (not edge_added){
                    --edge;
                }
            }
            num_edges+=edges_added;
        }
    }
    return g;
}

/// Create a directed scale-free graph
/** This function generates a scale-free graph using the model from
 *  Bollobás et al. Multi-edges and self-loops are not allowed.
 *
 * @tparam Graph The graph type
 * @tparam RNG The random number generator type
 * @param n The total number of vertices
 * @param alpha The probability for option 'A'
 * @param beta The probability for option 'B'
 * @param gamma The probability for option 'C'
 * @param del_in The unnormalized attraction of new vertices
 * @param del_out The unnormalized connectivity of new vertices
 * @param rng The random number generator
 * @return Graph The directed scale-free graph
 */
template <typename Graph, typename RNG>
Graph create_scale_free_directed_graph( const std::size_t num_vertices,
                                        const double alpha,
                                        const double beta,
                                        const double gamma,
                                        const double del_in,
                                        const double del_out,
                                        RNG& rng)
{
    // Create three-cycle as spawning network.
    Graph g(3);
    boost::add_edge(0, 1, g);
    boost::add_edge(1, 2, g);
    boost::add_edge(2, 0, g);
    
    // Check for cases in which the algorithm does not work.
    if (alpha + beta + gamma != 1.){
        throw std::runtime_error("The probabilities alpha, beta and gamma"
                                " have to add up to 1!");
    }
    else if (not boost::is_directed(g)){
        throw std::runtime_error("This algorithm only works for directed"
                                " graphs!");
    }

    std::uniform_real_distribution<> rand(0, 1);

    // Define helper variables
    const auto num_edges = boost::num_edges(g);
    auto norm_in = 0.;
    auto norm_out = 0.;
    bool skip;

    // In each step, add one edge to the graph. A new vertex may or may not be
    // added to the graph. In each step, do option 'A', 'B' or 'C' with the
    // respective probability fractions 'alpha', 'beta' and 'gamma'.
    while (boost::num_vertices(g) < num_vertices) {
        skip = false;
        auto v = boost::vertex(0, g);
        auto w = boost::vertex(0, g);

        // Update the normalization for in- and out-degree probabilities
        norm_in = num_edges + del_in * boost::num_vertices(g);
        norm_out = num_edges + del_out * boost::num_vertices(g);
        const auto rand_num = rand(rng);
        
        if (rand_num < alpha) {
            // option 'A'
            // Add new vertex v and add edge (v,w) with w drawn from the
            // discrete in-degree probablility distribution of already existing
            // vertices.
            auto prob_sum = 0.;
            const auto r = rand(rng);

            for (auto [p, p_end] = boost::vertices(g); p!=p_end; ++p) {
                
                prob_sum += (boost::in_degree(*p, g) + del_in) / norm_in;
                if (r < prob_sum) {
                    w = *p;
                    break;
                }
            }
            v = boost::add_vertex(g);
        }

        else if (rand_num < alpha + beta) {
            // option 'B'
            // Add edge (v,w) with v(w) drawn from the discrete out-degree
            // (in-degree) probablility distribution of already existing
            // vertices.
            auto prob_sum_in = 0.;
            auto prob_sum_out = 0.;
            auto r_in = rand(rng);
            auto r_out = rand(rng);

            // Find the source of the new edge.
            for (auto [p, p_end] = boost::vertices(g); p!=p_end; ++p) {
                
                prob_sum_out += (boost::out_degree(*p, g) + del_out)/norm_out;
                if (r_out < prob_sum_out) {
                    v = *p;
                    break;
                }
            }
            // Find the target of the new edge.
            for (auto [p, p_end] = boost::vertices(g); p!=p_end; ++p) {
                
                prob_sum_in += (boost::in_degree(*p, g) + del_in)/norm_in;
                if (r_in < prob_sum_in) {
                    if (v!=*p and (not boost::edge(v,*p, g).second)) {
                        w = *p;
                        break;
                    }
                    else {
                        // Do not allow multi-edges or self-loops.
                        skip = true;
                        break;
                    }
                }
            }
        }

        else {
            // option 'C'
            // Add new vertex w and add edge (v,w) with v drawn from the
            // discrete out-degree probablility distribution of already
            // existing vertices.
            auto prob_sum = 0.;
            const auto r = rand(rng);

            for (auto [p, p_end] = boost::vertices(g); p!=p_end; ++p) {
                
                prob_sum += (boost::out_degree(*p, g) + del_out)/norm_out;
                if (r < prob_sum) {
                    v = *p;
                    break;
                }
            }
            w = boost::add_vertex(g);
        }
        if (not skip) {
            ++num_edges;
            boost::add_edge(v, w, g);
        }
    }
    return g;
}


/// Create a small-world graph
/** This function creates a small-world graph using the Watts-Strogatz model.
 * @tparam Graph The graph type
 * @tparam RNG The random number generator type
 * @param num_vertices The total number of vertices
 * @param mean_degree The mean degree
 * @param p_rewire The rewiring probability
 * @param RNG The random number generator
 * @return Graph The small-world graph
 */
template <typename Graph, typename RNG>
Graph create_small_world_graph( const std::size_t num_vertices,
                                const std::size_t mean_degree,
                                const double p_rewire,
                                RNG& rng)
{
    // Create an empty graph
    Graph g;

    // Define a small-world generator
    using SWGen = boost::small_world_iterator<RNG, Graph>;

    // Create a small-world graph
    g = Graph(  SWGen(rng, num_vertices, mean_degree, p_rewire),
                SWGen(), 
                num_vertices);

    // Return the graph
    return g;
}


/// Cycles a vertex index
/** Cycles the index of a vertex such that if the vertex index exceeds 
 * the number of vertices it is projected on the interval [0, num_vertices]
 * 
 * @param vertex The vertex index
 * @param num_vertices The number of vertices
 * 
 * @return The cycled vertex index
 */
int cycled_index(const long int vertex, const long int num_vertices){
    if (vertex <= num_vertices){
        if (vertex >= 0){
            return vertex;
        }
        else {
            return cycled_index(vertex + num_vertices, num_vertices);
        }
    }
    else{
        return cycled_index(vertex % num_vertices, num_vertices);
    }
}


/// Create a k-regular graph (a circular graph)
/** @brief  Create a regular graph with degree k.
 * Creates a regular graph arranged on a circle where vertices are connected 
 * to their k/2 next neighbors on both sides for the case that k is even.
 * If k is uneven an additional connection is added to the opposite lying vertex. 
 * In this case, the total number of vertices n has to be even otherwise the code 
 * returns an error.
 * 
 * @tparam Graph The graph type
 * 
 * @param num_vertices The number of vertices
 * @param degree The degree of every vertex
 */
template<typename Graph>
Graph create_k_regular_graph(   const long int num_vertices, 
                                const long int degree) {
    // Create a graph
    Graph g(num_vertices);

    // Case of uneven degree
    if (degree % 2 == 1)
    {
        // Case of uneven number of vertices
        if (num_vertices % 2 == 1){
            throw std::runtime_error("If the degree is uneven, the number"
                                    "of vertices cannot be uneven too!");
        }
        // Case of even number of vertices
        else
        {
            // Imagine vertices arranged on a circle.
            // For every node add connections to the next degree/2 next 
            // neighbors in both directions.
            // Additionally add a node to the opposite neighbor on the circle
            for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
                for (auto nb = -degree/2; nb <= degree/2; ++nb){
                    if (nb != 0){
                        // Calculate the source and target of the new edge
                        auto source = cycled_index(*v, num_vertices);
                        auto target = cycled_index(*v + nb, num_vertices)
                                            % num_vertices;
                        
                        // If the edge does not exist yet, create it
                        if (!edge(source, target, g).second){
                            boost::add_edge(source, target, g);
                        }
                    }
                    else if (nb == 0){
                        // Calculate source and target of the edge
                        auto source = cycled_index(*v, num_vertices);
                        auto target = cycled_index(*v + num_vertices / 2, num_vertices) 
                                            % num_vertices;

                        // If the edge does not exist yet, create it
                        if (!boost::edge(source, target, g).second){
                            boost::add_edge(source, target, g);
                        }
                    }
                }
            }
        }
    }
    // Case of even degree
    else
    {
        for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
            for (auto nb = -degree/2; nb <= degree/2; ++nb){
                if (nb != 0){
                    // Calculate source and target of the new edge
                    auto source = *v;
                    auto target = cycled_index(*v +nb, num_vertices) % num_vertices;

                    // Add the new edge if it not yet exists
                    if (!boost::edge(source, target, g).second){
                        boost::add_edge(source, target, g); 
                    }
                }
            }
        }
    }
    return g;
}


} // namespace Graph
} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_HH
