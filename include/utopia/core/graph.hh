#ifndef UTOPIA_CORE_GRAPH_HH
#define UTOPIA_CORE_GRAPH_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/graph/random.hpp>

#include "utopia/data_io/cfg_utils.hh"
#include "utopia/core/types.hh"

namespace Utopia {
namespace Graph {

// -- Helper Functions --------------------------------------------------------

/// Cycles a vertex index
/** \detail Cycles the index of a vertex such that if the vertex index exceeds 
 *          the number of vertices it is projected on the interval 
 *          [0, num_vertices]
 * 
 * @param vertex        The vertex index
 * @param num_vertices  The number of vertices
 * 
 * @return              The cycled vertex index
 */
constexpr int _cycled_index(int vertex, int num_vertices){
    if (vertex <= num_vertices){
        if (vertex >= 0){
            return vertex;
        }
        else {
            return _cycled_index(vertex + num_vertices, num_vertices);
        }
    }
    else{
        return _cycled_index(vertex % num_vertices, num_vertices);
    }
}

// -- Graph creation algorithms -----------------------------------------------

/// Create a Erdös-Rényi random graph
/** \detail This function uses the create_random_graph function from the boost 
 *          graph library. It uses the Erdös-Rényi algorithm to generate the 
 *          random graph. Sources and targets of edges are randomly selected
 *          with equal probability. Thus, every possible edge has the same 
 *          probability to be created.
 * 
 * \note The underlying boost::generate_random_graph function requires the
 *       total number of edges as input. In case of an undirected graph it is
 *       calculated through: num_edges = num_vertices * mean_degree / 2, in
 *       case of a directed graph through:
 *       num_edges = num_vertices * mean_degree. 
 *       If the integer division of the right hand side leaves a rest, the 
 *       mean degree will be slightly distorted. However, for large numbers of 
 *       num_vertices this effect is negligible.
 *
 * \tparam Graph            The graph type
 * \tparam RNG              The random number generator type
 * 
 * \param num_vertices      The total number of vertices
 * \param mean_degree       The mean degree (= mean in-degree
 *                          = mean out-degree for directed graphs)
 * \param allow_parallel    Allow parallel edges within the graph
 * \param self_edges        Allows a vertex to be connected to itself
 * \param rng               The random number generator
 * 
 * \return Graph            The random graph
 */
template<typename Graph, typename RNG>
Graph create_ErdosRenyi_graph(std::size_t num_vertices, 
                              std::size_t mean_degree,
                              bool allow_parallel,
                              bool self_edges,
                              RNG& rng)
{
    // Create an empty graph
    Graph g; 

    // Calculate the number of edges
    const std::size_t num_edges = [&](){
        if (boost::is_directed(g)) {
            return num_vertices * mean_degree;
        }
        else {
            return num_vertices * mean_degree / 2;
        }
    }(); // directly call the lambda function to initialize the variable


    // Create a random graph using the Erdös-Rényi algorithm
    boost::generate_random_graph(g, 
                                 num_vertices, 
                                 num_edges, 
                                 rng, 
                                 allow_parallel, 
                                 self_edges); 

    // Return graph
    return g;
}


/// Create a Barabási-Albert scale-free graph
/** \detail This function generates a scale-free graph using the 
 *          Barabási-Albert model. The algorithm starts with a small spawning 
 *          network to which new vertices are added one at a time. Each new 
 *          vertex receives a connection to mean_degree existing vertices with a 
 *          probability that is proportional to the number of links of the 
 *          corresponding vertex. 
 *
 * \tparam Graph        The graph type
 * \tparam RNG          The random number generator type
 * 
 * \param num_vertices  The total number of vertices
 * \param mean_degree   The mean degree
 * \param rng           The random number generator
 * 
 * \return Graph        The scale-free graph
 */
template <typename Graph, typename RNG>
Graph create_BarabasiAlbert_graph(std::size_t num_vertices,
                                  std::size_t mean_degree,
                                  RNG& rng)
{
    // Create an empty graph
    Graph g;
    
    // Check for cases in which the algorithm does not work
    if (num_vertices < mean_degree){
        throw std::invalid_argument("The mean degree has to be smaller than "
                                    "the total number of vertices!");
    }
    else if (mean_degree % 2){
        throw std::invalid_argument("The mean degree needs to be even but "
                                    "is not an even number!");
    }
    else if (boost::is_directed(g)){
        throw std::runtime_error("This scale-free generator algorithm "
                                 "only works for undirected graphs! " 
                                 "But the provided graph is directed.");
    }
    else{
        // Define helper variables
        auto num_edges = 0;
        auto deg_ignore = 0;

        // Create initial spawning network that is fully connected
        for (std::size_t i = 0; i<mean_degree+1; ++i){
            for (std::size_t j = 0; j<i; ++j){
                // Increase the number of edges only if an edge was added
                if (boost::add_edge(i, j, g).second == true){
                    ++num_edges;
                }
            }
        }

        // Keep account whether an edge has been added or not
        bool edge_added = false;

        // Add i times a vertex and connect it randomly but weighted 
        // to the existing vertices
        std::uniform_real_distribution<> distr(0, 1);
        for (std::size_t i = 0; i<(num_vertices - mean_degree - 1); ++i){
            // Add a new vertex
            const auto new_vertex = boost::add_vertex(g);
            auto edges_added = 0;

            // Add the desired number of edges
            for (std::size_t edge = 0; edge<mean_degree/2; ++edge){
                // Keep track of the probability
                auto prob = 0.;

                // Loop through every vertex and look if it can be connected
                for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v)
                {
                    // Until now, no edge has been added. Reset edge_added.
                    edge_added = false;
                    // accumulate the probability fractions
                    prob += boost::out_degree(*v, g) 
                            / ((2. * num_edges) - deg_ignore);

                    if (distr(rng) <= prob){
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


/// Create a scale-free directed graph
/** \detail This function generates a scale-free graph using the model from
 *          Bollobás et al. Multi-edges and self-loops are not allowed.
 *          The graph is built by continuously adding new edges via preferential
 *          attachment. In each step, an edge is added in one of the following
 *          three ways:
 *          - A: add edge from a newly added vertex to an existing one
 *          - B: add edge between two already existing vertices
 *          - C: add edge from an existing vertex to a newly added vertex
 *          As the graph is directed there can be different attachment
 *          probability distributions for in-edges and out-edges. 
 *          The probability for choosing a vertex as source (target) of the new
 *          edge is proportional to its current out-degree (in-degree).
 *          Each newly added vertex has a fixed initial probability to be chosen
 *          as source (target) which is proportional to del_out (del_in).
 *
 * \tparam Graph    The graph type
 * \tparam RNG      The random number generator type

 * \param n         The total number of vertices
 * \param alpha     The probability for option 'A'
 * \param beta      The probability for option 'B'
 * \param gamma     The probability for option 'C'
 * \param del_in    The unnormalized attraction of newly added vertices
 * \param del_out   The unnormalized connectivity of newly added vertices
 * \param rng       The random number generator

 * \return Graph    The scale-free directed graph
 */
template <typename Graph, typename RNG>
Graph create_BollobasRiordan_graph(std::size_t num_vertices,
                                   double alpha,
                                   double beta,
                                   double gamma,
                                   double del_in,
                                   double del_out,
                                   RNG& rng)
{
    // Create empty graph.
    Graph g;

    // Check for cases in which the algorithm does not work.
    if (alpha + beta + gamma != 1.) {
        throw std::invalid_argument("The probabilities alpha, beta and gamma "
                                    "have to add up to 1!");
    }
    if (not boost::is_directed(g)) {
        throw std::runtime_error("This algorithm only works for directed "
                                 "graphs but the graph type specifies an "
                                 "undirected graph!");
    }

    // Create three-cycle as spawning network.
    boost::add_vertex(g);
    boost::add_vertex(g);
    boost::add_vertex(g);
    boost::add_edge(0, 1, g);
    boost::add_edge(1, 2, g);
    boost::add_edge(2, 0, g);

    std::uniform_real_distribution<> distr(0, 1);

    // Define helper variables.
    auto num_edges = boost::num_edges(g);
    auto norm_in = 0.;
    auto norm_out = 0.;
    bool skip;

    // In each step, add one edge to the graph. A new vertex may or may not be
    // added to the graph. In each step, choose option 'A', 'B' or 'C' with the
    // respective probability fractions 'alpha', 'beta' and 'gamma'.
    while (boost::num_vertices(g) < num_vertices) {
        skip = false;
        auto v = boost::vertex(0, g);
        auto w = boost::vertex(0, g);

        // Update the normalization for in-degree and out-degree probabilities.
        norm_in = num_edges + del_in * boost::num_vertices(g);
        norm_out = num_edges + del_out * boost::num_vertices(g);
        const auto rand_num = distr(rng);
        
        if (rand_num < alpha) {
            // option 'A'
            // Add new vertex v and add edge (v,w) with w drawn from the
            // discrete in-degree probability distribution of already existing
            // vertices.
            auto prob_sum = 0.;
            const auto r = distr(rng);

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
            // (in-degree) probability distribution of already existing
            // vertices.
            auto prob_sum_in = 0.;
            auto prob_sum_out = 0.;
            const auto r_in = distr(rng);
            const auto r_out = distr(rng);

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
            // discrete out-degree probability distribution of already
            // existing vertices.
            double prob_sum = 0.;
            const auto r = distr(rng);

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


/// Create a Watts-Strogatz small-world graph
/** \details This function creates a small-world graph using the Watts-Strogatz 
 *           model. It creates a k-regular graph and relocates vertex connections
 *           with a given probability.
 *  \warning The graph generating small_world_generator function from the
 *           boost graph library that is used in this function rewires only 
 *           in_edges. The out_edge distribution is still constant with the 
 *           delta-peak at the mean_degree.
 * 
 * /tparam Graph        The graph type
 * /tparam RNG          The random number generator type
 * 
 * /param num_vertices  The total number of vertices
 * /param mean_degree   The mean degree (= mean in-degree
 *                      = mean out-degree for directed graphs)
 * /param p_rewire      The rewiring probability
 * /param RNG           The random number generator
 * 
 * /return Graph        The small-world graph
 */
template <typename Graph, typename RNG>
Graph create_WattsStrogatz_graph(std::size_t num_vertices,
                                 std::size_t mean_degree,
                                 double p_rewire,
                                 RNG& rng)
{
    // Create an empty graph
    Graph g;

    // Define a small-world generator
    using SWGen = boost::small_world_iterator<RNG, Graph>;

    if (boost::is_directed(g)) {
        mean_degree *= 2;
    }

    // Create a small-world graph
    g = Graph(SWGen(rng, num_vertices, mean_degree, p_rewire),
              SWGen(), 
              num_vertices);

    // Return the graph
    return g;
}


/// Create a k-regular graph (a circular graph)
/** \brief  Create a regular graph with degree k.
 *          Creates a regular graph arranged on a circle where vertices are 
 *          connected to their k/2 next neighbors on both sides for the case 
 *          that k is even. If k is uneven an additional connection is added to 
 *          the opposite lying vertex. In this case, the total number of 
 *          vertices n has to be even otherwise the code returns an error.
 * 
 * \tparam Graph        The graph type
 * 
 * \param num_vertices  The number of vertices
 * \param degree        The degree of every vertex
 */
template<typename Graph>
Graph create_k_regular_graph(int num_vertices, 
                             int degree) {
    // Create an empty graph
    Graph g(num_vertices);

    if (boost::is_directed(g)) {
        throw std::runtime_error("This algorithm only works for undirected "
                                 "graphs in the current implementation but "
                                 "the graph type specifies a directed graph!");
    }

    // Case of uneven degree
    if (degree % 2 == 1)
    {
        // Case of uneven number of vertices
        if (num_vertices % 2 == 1){
            throw std::invalid_argument("If the degree is uneven, the number "
                                        "of vertices cannot be uneven too!");
        }
        // Case of even number of vertices
        // Imagine vertices arranged on a circle.
        // For every node add connections to the next degree/2 next 
        // neighbors in both directions.
        // Additionally add a node to the opposite neighbor on the circle
        for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
            for (auto nb = -degree/2; nb <= degree/2; ++nb){
                if (nb != 0){
                    // Calculate the source and target of the new edge
                    auto source = _cycled_index(*v, num_vertices);
                    auto target = _cycled_index(*v + nb, num_vertices)
                                    % num_vertices;
                    
                    // If the edge does not exist yet, create it
                    if (not edge(source, target, g).second){
                        boost::add_edge(source, target, g);
                    }
                }
                else if (nb == 0){
                    // Calculate source and target of the edge
                    auto source = _cycled_index(*v, num_vertices);
                    auto target = _cycled_index(*v + num_vertices / 2, 
                                                num_vertices) 
                                    % num_vertices;

                    // If the edge does not exist yet, create it
                    if (not boost::edge(source, target, g).second){
                        boost::add_edge(source, target, g);
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
                    auto target = _cycled_index(*v + nb, num_vertices) 
                                    % num_vertices;

                    // Add the new edge if it not yet exists
                    if (not boost::edge(source, target, g).second){
                        boost::add_edge(source, target, g); 
                    }
                }
            }
        }
    }
    return g;
}


// .. Convenient graph creation function ......................................

/// Create a graph from a configuration node 
/** \detail Select a graph creation algorithm and create the graph object from  
 *          a configuration node.
 * 
 * \tparam Graph        The graph type
 * \tparam Config       The configuration node type
 * \tparam RNG          The random number generator type
 * 
 * \param cfg           The configuration
 * \param rng           The random number generator
 * 
 * \return Graph        The graph
 */
template<typename Graph, typename RNG>
Graph create_graph(const Utopia::DataIO::Config& cfg, RNG& rng)
{
    // Get the graph generating model
    const std::string model = get_as<std::string>("model", cfg);

    // Call the correct graph creation algorithm depending on the configuration
    // node.
    if (model == "regular")
    {
        return create_k_regular_graph<Graph>(
                    get_as<int>("num_vertices", cfg),
                    get_as<int>("mean_degree", cfg));
    }
    else if (model == "ErdosRenyi")
    {
        return create_ErdosRenyi_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<bool>("parallel", cfg),
                    get_as<bool>("self_edges", cfg),
                    rng);
    }
    else if (model == "WattsStrogatz")
    {
        return create_WattsStrogatz_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<double>("p_rewire", cfg),
                    rng);
    }
    else if (model == "BarabasiAlbert")
    {
        return create_BarabasiAlbert_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    rng);
    }
    else if (model == "BollobasRiordan")
    {
        return create_BollobasRiordan_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<double>("alpha", cfg),
                    get_as<double>("beta", cfg),
                    get_as<double>("gamma", cfg),
                    get_as<double>("del_in", cfg),
                    get_as<double>("del_out", cfg),
                    rng);
    }
    else {
        throw std::invalid_argument("The given graph model '" + model + 
                                    "'does not exist! Valid options are: "
                                    "'regular', 'ErdosRenyi', "
                                    "'WattsStrogatz', 'BarabasiAlbert', "
                                    "'BollobasRiordan'.");
    }
}


} // namespace Graph
} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_HH
