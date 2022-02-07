#ifndef UTOPIA_CORE_GRAPH_CREATION_HH
#define UTOPIA_CORE_GRAPH_CREATION_HH

#include <algorithm>
#include <vector>
#include <deque>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/small_world_generator.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/dynamic_property_map.hpp>

#include <spdlog/spdlog.h>

#include "utopia/data_io/cfg_utils.hh"
#include "utopia/data_io/filesystem.hh"
#include "utopia/data_io/graph_load.hh"
#include "utopia/core/types.hh"

namespace Utopia {
namespace Graph {

/**
 *  \addtogroup Graph
 *  \{
 */


// -- Graph creation algorithms -----------------------------------------------

/// Create a complete graph
/** This function creates a complete graph, i.e. one in which every vertex is
 * connected to every other. No parallel edges are created.
 *
 * \tparam Graph            The graph type
 * \param n                 The total number of vertices
 *
 * \return Graph            The graph
 */

template <typename Graph>
Graph create_complete_graph(const std::size_t n) {

    using vertices_size_type = typename boost::graph_traits<Graph>::vertices_size_type;
    using namespace boost;

    // Create empty graph with n vertices
    Graph g{n};

    // Conntect every vertex to every other. For undirected graphs, this means
    // adding 1/2n(n-1) edges. For directed, add n(n-1) edges.
    if (is_undirected(g)){
        for (vertices_size_type v = 0; v < n; ++v){
            for (vertices_size_type k = v+1; k < n; ++k) {
                add_edge(vertex(v, g), vertex(k, g), g);
            }
        }
    }

    else {
        for (vertices_size_type v = 0; v < n; ++v) {
            for (vertices_size_type k = 1; k < n; ++k) {
                add_edge(vertex(v, g), vertex((v+k)%n, g), g);
            }
        }
    }

    // Return the graph
    return g;
}


/// Create a Erdös-Rényi random graph
/** This function uses the create_random_graph function from the boost
 *  graph library. It uses the Erdös-Rényi algorithm to generate the
 *  random graph. Sources and targets of edges are randomly selected
 *  with equal probability. Thus, every possible edge has the same
 *  probability to be created.
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
Graph create_ErdosRenyi_graph(const std::size_t num_vertices,
                              const std::size_t mean_degree,
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


/// Create a regular lattice graph
/** \details This function creates a k-regular graph. The algorithm has been
 *           adapted for directed graphs, for which it can be specified whether
 *           the underlying lattice graph is oriented or not.
 *
 * \tparam Graph            The graph type
 * \param n                 The total number of vertices
 * \param k                 The mean degree (= mean in-degree
 *                          = mean out-degree for directed graphs)
 * \param oriented         (For directed graphs) whether the created lattice
                            is oriented (only connecting to forward neighbors)
                            or not
 * \param rng               The random number generator
 *
 * \return Graph            The regular graph
 */
template <typename Graph>
Graph create_regular_graph(const std::size_t n,
                           const std::size_t k,
                           const bool oriented)
{
  using vertices_size_type = typename boost::graph_traits<Graph>::vertices_size_type;
  using namespace boost;

  if (k >= n-1) {
      return create_complete_graph<Graph>(n);
  }

  // Start with empty graph with num_vertices nodes
  Graph g{n};

  if (is_undirected(g) && k % 2){
      throw std::invalid_argument("For undirected regular graphs, the mean "
                                  "degree needs to be even!");
  }

  else if (is_directed(g) && !oriented && k % 2){
      throw std::invalid_argument("For directed regular graphs, the mean "
        "degree can only be uneven if the graph is oriented! Set "
        "'oriented = true', or choose an even mean degree.");
  }

  // Generate a regular network. For undirected graphs, the k-neighborhood are the k/2
  // vertices to the right, and the k/2 vertices to the left. For directed
  // graphs, if the lattice is set to 'not oriented', this is also the case.
  // Alternatively, for oriented = true, the k neighborhood consists of
  // k neighbors to the right.
  // No parallel edges or self-loops are created.

  // Undirected graphs
  if (is_undirected(g)){
      for (vertices_size_type v = 0; v < n; ++v) {
          for (vertices_size_type i = 1; i <= k/2; ++i) {
                add_edge(vertex(v, g), vertex((v + i) % n, g), g);
          }
      }
  }

  // Directed graphs
  else {
      if (!oriented) {
          for (vertices_size_type v = 0; v < n; ++v) {
              for (vertices_size_type i = 1; i <= k/2; ++i) {
                  // Forward direction
                  add_edge(vertex(v, g), vertex((v + i) % n, g), g);
                  // Backward direction
                  add_edge(vertex(v, g), vertex((v - i + n) % n, g), g);
              }
          }
      }

      else {
          for (vertices_size_type v = 0; v < n; ++v) {
              for (vertices_size_type i = 1; i <= k; ++i) {
                  add_edge(vertex(v, g), vertex((v + i) % n, g), g);
              }
          }
      }
  }

  // Return the graph
  return g;
}

/// Create a Klemm-Eguíluz scale-free small-world highly-clustered graph
/** This function generates a graph using the Klemm-Eguíluz model (Klemm &
 *  Eguíluz 2002). The algorithm starts with a small spawning network to which
 *  new vertices are added one at a time. Each new vertex receives a connection
 *  to mean_degree existing vertices with a probability that is proportional to
 *  the number of links of the corresponding vertex. With probability mu, links
 *  are instead rewired to a (possibly non-active) vertex, chosen with a
 *  probability that is proportional to its degree. Thus, for mu=1 we obtain
 *  the Barabasi-Albert linear preferential attachment model.
 *
 *
 * \tparam Graph        The graph type
 * \tparam RNG          The random number generator type
 *
 * \param num_vertices  The total number of vertices
 * \param mean_degree   The mean degree
 * \param mu            The probability of rewiring to a random vertex
 * \param rng           The random number generator
 *
 * \return Graph        The scale-free graph
 */

template <typename Graph, typename RNG>
Graph create_KlemmEguiluz_graph(const std::size_t num_vertices,
                                const std::size_t mean_degree,
                                const double mu,
                                RNG& rng)
{
    using vertex_size_type = typename boost::graph_traits<Graph>::vertices_size_type;
    using vertex_desc_type = typename boost::graph_traits<Graph>::vertex_descriptor;
    using namespace boost;

    if ( mu > 1. or mu < 0.) {
        throw std::invalid_argument("The parameter 'mu' must be a probability!");
    }
    else if ( mean_degree <= 2 ) {
        throw std::invalid_argument("This algorithm requires a mean degree of"
          " 3 or more!");
    }

    // Generate complete graphs separately, since they do not allow for rewiring
    if (mean_degree >= num_vertices-1) {
        return create_complete_graph<Graph>(num_vertices);
    }

    // Uniform probability distribution
    std::uniform_real_distribution<double> distr(0, 1);

    // Generate an empty graph with num_vertices vertices. This avoids having to
    // reallocate when adding vertices.
    Graph g{num_vertices};

    // Especially for low vertex counts, the original KE does not produce a
    // network with exactly the mean_degree specified. This function corrects
    // for the offset by calcuating an effective size for the spawning network.
    // This has the added benefit of not neccessetating en even mean degree.
    const std::size_t m = [&]() -> std::size_t {
        if (is_undirected(g)) {
            double a = sqrt(4.0*pow(num_vertices, 2)
                            - 4.0*num_vertices*(mean_degree+1.0) +1.0);
            a *= -0.5;
            a += num_vertices - 0.5;

            return std::round(a);
        }
        else {
            return std::round(
                num_vertices * static_cast<double>(mean_degree) / (2*(num_vertices-1))
            );
        }
    }();

    // Get a logger and output an info message saying what the actual mean
    // degree of the network will be
    const auto log = spdlog::get("core");
    if (not log) {
        throw std::runtime_error(
            "Logger 'core' was not set up but is needed for "
            "Utopia::Graph::create_KlemmEguiluz_graph !"
        );
    }

    auto actual_mean_degree = [&](){
        if (is_undirected(g)) {
            return (1.0*m*(m-1)+2.0*m*(num_vertices-m))/num_vertices;
        }
        else {
            return (2.0*m*(m-1)+2.0*m*(num_vertices-m))/num_vertices;
        }
    };
    log->info("The desired mean degree of this graph is {}; the actual mean"
              " degree of this graph will be {}.", mean_degree, actual_mean_degree());

    // Create a container for the active vertices.
    // Reserve enough space to avoid reallocating.
    std::vector<vertex_desc_type> actives;
    actives.reserve(m);

    // Create a container listing all degrees in the graph, as well as the
    // number of vertices of that degree
    std::vector<std::pair<size_t, size_t>> degrees_and_num;

    // Create a container of all vertices sorted by their degree. Reserve enough
    // space to avoid reallocating.
    const size_t num_deg = is_undirected(g) ? num_vertices : 2*(num_vertices);
    std::vector<std::deque<vertex_desc_type>> vertices_by_deg(num_deg,
        std::deque<vertex_desc_type>());

    // Create a fully-connected initial subnetwork. For all except the pure BA,
    // set all vertices as active.
    for (vertex_size_type i = 0; i < m; ++i) {
        if (is_undirected(g)) {
            for (vertex_size_type k = i+1; k < m; ++k) {
                add_edge(vertex(i, g), vertex(k, g), g);
            }
        }
        else {
            for (vertex_size_type k = 1; k < m; ++k) {
                add_edge(vertex(i, g), vertex((i+k)%m, g), g);
            }
        }

        // For the pure BA, add all vertices of the spawning network to the
        // list of vertices
        if (mu == 1) {
            if (is_undirected(g)){
                vertices_by_deg[m-1].emplace_back(vertex(i, g));
            }
            else {
                vertices_by_deg[2*(m-1)].emplace_back(vertex(i, g));
            }
        }
        else {
            actives.emplace_back(vertex(i, g));
        }
    }

    // For the pure BA, there are now m vertices each with degree m-1
    // (or 2(m-1) in the directed case). Add them all to the list of degrees.
    // For quick access, place the frequent case deg = m at the beginning of the
    // list. That way, every newly added vertex can immediately be placed into the
    // the list without needing to search for the correct index.
    if (mu == 1 and is_undirected(g)) {
        degrees_and_num.emplace_back(std::make_pair(m-1, m));
        degrees_and_num.emplace_back(std::make_pair(m, 0));
    }
    else if (mu == 1 and is_directed(g)){
        degrees_and_num.emplace_back(std::make_pair(m, 0));
        degrees_and_num.emplace_back(std::make_pair(2*(m-1), m));
    }

    // Select a degree with probability proportional to its prevalence and
    // value. Returns the degree and the index of that degree in the list.
    auto get_degree = [&](const size_t norm) -> std::pair<std::size_t, std::size_t>
    {
        const double prob = distr(rng) * norm;
        double cumulative_prob = 0;
        std::pair<std::size_t, size_t> res = {0, 0};
        for (std::size_t i = 0; i<degrees_and_num.size(); ++i) {
            cumulative_prob += degrees_and_num[i].first * degrees_and_num[i].second;
            if (cumulative_prob >= prob) {
                res = {degrees_and_num[i].first, i};
                break;
            }
        }
        return res;
    };

    // Pure BA model
    if (mu == 1) {
        // Normalisation factor: sum of all vertices x their degree
        std::size_t norm = is_undirected(g) ? m*(m-1) : m*2*(m-1);
        for (vertex_size_type n = m; n < num_vertices; ++n) {
            const auto v = vertex(n, g);
            for (std::size_t i = 0; i < m; ++i) {
                // Add an edge to a neighbor that was selected with probability
                // proportional to its in-degree
                auto d = get_degree(norm);
                std::size_t idx = distr(rng) * (vertices_by_deg[d.first].size());
                auto w = vertices_by_deg[d.first][idx];
                while (w == v or edge(v, w, g).second){
                    d = get_degree(norm);
                    idx = distr(rng) * (vertices_by_deg[d.first].size());
                    w = vertices_by_deg[d.first][idx];
                }
                add_edge(v, w, g);
                // Move that neighbor to the correct spot in the
                // list of vertices, and update the counts of the
                // relevant degree list entries.
                --degrees_and_num[d.second].second;
                if (d.second < degrees_and_num.size()-1
                    and degrees_and_num[d.second+1].first == d.first+1)
                {
                    ++degrees_and_num[d.second+1].second;
                }
                else {
                    degrees_and_num.emplace_back(std::make_pair(d.first+1, 1));
                    std::sort(
                        degrees_and_num.begin(), degrees_and_num.end(),
                        [](const auto& a, const auto& b) -> bool {
                            return a.first < b.first;
                        }
                    );
                }

                // Finally, increase the norm by 1 (since the degree of one
                // vertex was increased by 1), and move the neighbor into its
                // new bin in the vertex list.
                norm += 1;
                vertices_by_deg[d.first].erase(vertices_by_deg[d.first].begin()+idx);
                vertices_by_deg[d.first+1].emplace_back(w);
            }

            // After connecting the new vertex to m neighbors, increase the norm
            // by m, since a vertex of degree m is added. Add the new vertex to
            // the list of vertices.
            norm += m;
            vertices_by_deg[m].emplace_back(v);

            // Increase the count of the deg = m entry in the degree list.
            if (is_undirected(g)) {
                degrees_and_num[1].second++;
            }
            else {
                degrees_and_num[0].second++;
            }
        }
    }

    else {
        // Collect the sum of the degree x num vertices of that degree.
        std::size_t norm = 0;

        // Add the remaining number of vertices, and add edges to m other vertices.
        for (vertex_size_type n = m; n < num_vertices; ++n) {
            const auto v = vertex(n, g);

            // Treat the special case mu=0 (pure KE) separately
            // to avoid unnecessarily generating random numbers.
            if (mu == 0) {
                for (auto const& a : actives) {
                    add_edge(v, a, g);
                }
            }

            // With probability mu, connect to a non-active node chosen via the linear
            // attachment model. With probability 1-mu, connect to an active node.
            else {
                for (auto const& a : actives) {
                    if (distr(rng) < mu and n != m) {
                        // There may not be enough inactive nodes to rewire to.
                        // Stop the while loop after finite number of attempts
                        // to find a new neighbor. If number of attemps is
                        // surpassed, simply connect to an active node.
                        std::size_t max_attempts = n-m+2;

                        // Add an edge to a neighbor that was selected with
                        // probability proportional to its in-degree
                        auto d = get_degree(norm);
                        std::size_t idx = distr(rng) * (vertices_by_deg[d.first].size());
                        auto w = vertices_by_deg[d.first][idx];
                        while (edge(v, w, g).second and max_attempts > 0){
                            d = get_degree(norm);
                            idx = distr(rng) * (vertices_by_deg[d.first].size());
                            w = vertices_by_deg[d.first][idx];
                            --max_attempts;
                        }

                        // Move that neighbor to the correct spot in the
                        // list of vertices, and update the counts of the
                        // relevant degree list entries.
                        if (max_attempts > 0) {
                            add_edge(v, w, g);
                            --degrees_and_num[d.second].second;
                            if (d.second < degrees_and_num.size()-1
                                and degrees_and_num[d.second+1].first == d.first+1)
                            {
                                ++degrees_and_num[d.second+1].second;
                            }
                            else {
                                degrees_and_num.emplace_back(std::make_pair(d.first+1, 1));
                                std::sort(
                                    degrees_and_num.begin(), degrees_and_num.end(),
                                    [](const auto& a, const auto& b) -> bool {
                                        return a.first < b.first;
                                    }
                                );
                            }

                            // Finally, increase the norm by 1 (since the degree
                            // of one vertex was increased by 1), and move the
                            // neighbor into its new bin in the vertex list.
                            norm += 1;
                            vertices_by_deg[d.first].erase(vertices_by_deg[d.first].begin()+idx);
                            vertices_by_deg[d.first+1].emplace_back(w);
                        }
                        else {
                            add_edge(v, a, g);
                        }
                    }
                    else {
                        add_edge(v, a, g);
                    }
                }
            }

            // Calculate the sum of the active nodes in-degrees
            double gamma = 0;
            for (const auto& a : actives) {
                gamma += in_degree(a, g);
            }

            // Activate the new node and deactivate one of the old nodes.
            // Probability for deactivation is proportional to in degree.
            const double prob_to_drop = distr(rng) * gamma;
            double sum_of_probs = 0;

            for (vertex_size_type i = 0; i<actives.size(); ++i) {
                sum_of_probs += in_degree(actives[i], g);

                if (sum_of_probs >= prob_to_drop) {
                    bool deg_in_array = false;
                    // Find the correct index for the degree of the active node
                    // selected for deactivation. If no such index exists yet,
                    // create a new entry
                    for (std::size_t k = 0; k<degrees_and_num.size(); ++k) {
                        if (degrees_and_num[k].first == degree(actives[i], g)) {
                            ++degrees_and_num[k].second;
                            deg_in_array = true;
                            break;
                        }
                    }
                    // Index does not exist: create new entry
                    if (not deg_in_array) {
                        degrees_and_num.emplace_back(std::make_pair(degree(actives[i], g), 1));
                        sort(degrees_and_num.begin(), degrees_and_num.end(),
                        [](const auto& a, const auto& b) -> bool {
                            return a.first < b.first;
                        });
                    }

                    // Increase the norm by the degree of the active node.
                    norm += degree(actives[i], g);

                    // Place the active node into the correct entry of the
                    // vertex list
                    vertices_by_deg[degree(actives[i], g)].emplace_back(actives[i]);

                    // Activate the new node
                    actives[i] = v;
                    break;
                }
            }
        }
    }

    // Return the graph
    return g;

}

/// Generate a Barabási-Albert scale-free graph with parallel edges
/** This is the classic version of the generating model with a completely
 *  connected spawning network.
 *  This function generates a scale-free graph using the Barabási-Albert model.
 *  The algorithm starts with a small spawning network to which new vertices
 *  are added one at a time. Each new vertex receives a connection to
 *  mean_degree existing vertices with a probability that is proportional to
 *  the number of links of the corresponding vertex. In this version, the
 *  repeated vertices, that are added during the whole generating process, are
 *  stored. With each vertex added a uniform sample from the repeated_vertex
 *  is drawn. Each vertex thus has a probability to get selected that is
 *  proportional to the number of degrees of that vertex.
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
Graph BarabasiAlbert_parallel_generator(std::size_t num_vertices,
                                        std::size_t mean_degree,
                                        RNG& rng)
{
    // Type definitions
    using VertexDesc =
        typename boost::graph_traits<Graph>::vertex_descriptor;

    // The number of new edges added per network growing step is
    // equal to half the mean degree. This is because in calculating
    // the mean degree of an undirected graph, the edge (i,j) would be
    // counted twice (also as (j,i))
    const std::size_t num_new_edges_per_step = mean_degree / 2;

    // Create an empty graph
    Graph g{};

    // Generate the (fully-connected) spawning network
    for (unsigned v0 = 0; v0 < mean_degree; ++v0){
        boost::add_vertex(g);
        for (unsigned v1 = 0; v1 < v0; ++v1){
            boost::add_edge(boost::vertex(v0, g), boost::vertex(v1, g), g);
        }
    }

    // Create a vector in which to store all target vertices of each step ...
    std::vector<VertexDesc> target_vertices (boost::vertices(g).first,
                                            boost::vertices(g).second);

    // Create a vector that stores all the repeated vertices
    std::vector<VertexDesc> repeated_vertices{};

    // Reserve enough memory for the repeated vertices collection
    repeated_vertices.reserve(num_vertices * num_new_edges_per_step * 2);

    // Initialise a counter variable with mean_degree because
    // that is the number of vertices already added to the graph.
    std::size_t counter = boost::num_vertices(g);

    // Add (num_vertices - mean_degree) new vertices and mean_degree new edges
    while (counter < num_vertices)
    {
        const auto new_vertex = boost::add_vertex(g);

        // Add edges from the new vertex to the target vertices mean_degree
        // times
        for (auto target : target_vertices){
            boost::add_edge(new_vertex, target, g);

            // Add the target vertices to the repeated vertices container
            // as well as the new vertex for each time a new connection
            // is set.
            repeated_vertices.push_back(target);
            repeated_vertices.push_back(new_vertex);
        }

        // Reset the target vertices for the next iteration step by
        // randomly selecting mean_degree times uniformly from the
        // repeated_vertices container
        target_vertices.clear();
        std::sample(std::begin(repeated_vertices),
                    std::end(repeated_vertices),
                    std::back_inserter(target_vertices),
                    num_new_edges_per_step,
                    rng);

        ++counter;
    }

    return g;
}

/// Create a Barabási-Albert scale-free graph
/** This function generates a scale-free graph using the Barabási-Albert model.
 *  The algorithm starts with a small spawning network to which new vertices
 *  are added one at a time. Each new vertex receives a connection to
 *  mean_degree existing vertices with a probability that is proportional to
 *  the number of links of the corresponding vertex.
 *
 *  There are two slightly different variants of the algorithm, one that
 *  creates a graph with no parallel edges and one that creates a graph
 *  with parallel edges.
 *
 * \tparam Graph        The graph type
 * \tparam RNG          The random number generator type
 *
 * \param num_vertices  The total number of vertices
 * \param mean_degree   The mean degree
 * \param parallel      Whether the graph should have parallel edges or not
 * \param rng           The random number generator
 *
 * \return Graph        The scale-free graph
 */
template <typename Graph, typename RNG>
Graph create_BarabasiAlbert_graph(std::size_t num_vertices,
                                  std::size_t mean_degree,
                                  bool parallel,
                                  RNG& rng)
{
    // Generate the parallel version using the Klemm-Eguíluz generator
    if (not parallel) {
        return create_KlemmEguiluz_graph<Graph>(num_vertices, mean_degree, 1.0, rng);
    }
    else {
        // Check for cases in which the algorithm does not work.
        // Unfortunately, it is necessary to construct a graph object to check
        // whether the graph is directed or not.
        Graph g{};
        if (boost::is_directed(g)){
            throw std::runtime_error("This scale-free generator algorithm "
                                     "only works for undirected graphs! "
                                     "But the provided graph is directed.");

        } else if (num_vertices < mean_degree){
            throw std::invalid_argument("The mean degree has to be smaller than "
                                        "the total number of vertices!");
        }
        else if (mean_degree % 2){
            throw std::invalid_argument("The mean degree needs to be even but "
                                        "is not an even number!");
        }
        else{
            return BarabasiAlbert_parallel_generator<Graph>(num_vertices,
                                                            mean_degree, rng);
        }
    }
}


/// Create a scale-free directed graph
/** \details This function generates a scale-free graph using the model from
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
    if (std::fabs(alpha + beta + gamma - 1.)
            > std::numeric_limits<double>::epsilon()) {
        throw std::invalid_argument("The probabilities alpha, beta and gamma "
                                    "have to add up to 1!");
    }
    if (not boost::is_directed(g)) {
        throw std::runtime_error("This algorithm only works for directed "
                                 "graphs but the graph type specifies an "
                                 "undirected graph!");
    }
    if (beta == 1.){
        throw std::invalid_argument("The probability beta must not be 1!");
    }

    // Create three-cycle as spawning network.
    const auto v0 = boost::add_vertex(g);
    const auto v1 = boost::add_vertex(g);
    const auto v2 = boost::add_vertex(g);
    boost::add_edge(v0, v1, g);
    boost::add_edge(v1, v2, g);
    boost::add_edge(v2, v0, g);

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
 *           with a given probability. The algorithm has been adapted for
 *           directed graphs, for which it can be specified whether the
 *           underlying lattice graph is oriented or not.
 *
 * \tparam Graph            The graph type
 * \tparam RNG              The random number generator type
 *
 * \param n                 The total number of vertices
 * \param k                 The mean degree (= mean in-degree
 *                          = mean out-degree for directed graphs)
 * \param p_rewire          The rewiring probability
 * \param oriented          (for directed graphs) Whether the underlying starting
                            graph is oriented (only connecting to forward
                            neighbors) or not
 * \param RNG               The random number generator
 *
 * \return Graph            The small-world graph
 */

template <typename Graph, typename RNG>
Graph create_WattsStrogatz_graph(const std::size_t n,
                                 const std::size_t k,
                                 const double p_rewire,
                                 const bool oriented,
                                 RNG& rng)
{
  using namespace boost;
  using vertices_size_type = typename boost::graph_traits<Graph>::vertices_size_type;

  // Generate complete graphs separately, since they do not allow for rewiring
  if (k >= n-1) {
      return create_complete_graph<Graph>(n);
  }

  std::uniform_real_distribution<double> distr(0, 1);

  // Generate random vertex ids
  std::uniform_int_distribution<vertices_size_type> random_vertex_id(0, n-1);

  // Start with empty graph with num_vertices nodes
  Graph g{n};

  if (is_undirected(g) && k % 2){
      throw std::invalid_argument("For undirected Watts-Strogatz graphs, the "
                                  "mean degree needs to be even!");
  }

  else if (is_directed(g) && !oriented && k % 2){
      throw std::invalid_argument("For directed Watts-Strogatz graphs, the mean "
        "degree can only be uneven if the graph is oriented! Set "
        "'oriented = true', or choose an even mean degree.");
  }

  // If rewiring is turned off, a regular graph can be returned. This avoids
  // needlessly generating random numbers
  else if (p_rewire == 0) {
      return create_regular_graph<Graph>(n, k, oriented);
  }

  // Rewiring function
  auto add_edges = [&](vertices_size_type v,
                       const std::size_t limit,
                       const vertices_size_type& lower,
                       const vertices_size_type& upper,
                       const bool forward)
  {
      for (vertices_size_type i = 1; i <= limit; ++i) {
          vertices_size_type w;
          if (distr(rng) <= p_rewire) {
              w = random_vertex_id(rng);
              while (
                  ((forward && (w != (v + i) % n ))
                    or (!forward && (w != (v - i + n) % n)))
                  &&
                    ((upper > lower && (w >= lower && w <= upper))
                  or (upper < lower && (w >= lower or w <= upper))
                  or (edge(vertex(v, g), vertex(w, g), g).second)))
              {
                    w = random_vertex_id(rng);
              }
              add_edge(vertex(v, g), vertex(w, g), g);
          }
          else {
              w = forward ? (v + i) % n : (v - i + n) % n;
              add_edge(vertex(v, g), vertex(w, g), g);
          }
      }
  };

  // Generate a regular network, but rewiring to a random neighbor with
  // probability p_rewire. The new neighbor must not fall within the
  // k-neighborhood. For undirected graphs, the k-neighborhood are the k/2
  // vertices to the right, and the k/2 vertices to the left. For directed
  // graphs, if the lattice is set to 'not oriented', this is also the case.
  // Alternatively, for oriented_lattice = true, the k neighborhood consists of
  // k neighbors to the right.
  // No parallel edges or self-loops are created.

  // Upper and lower bounds of k-neighborhood: no rewiring takes place to
  // a vertex within these bounds
  vertices_size_type lower;
  vertices_size_type upper;

  // Undirected graphs
  if (is_undirected(g)){
      const std::size_t limit = k/2;
      for (vertices_size_type v = 0; v < n; ++v) {
          // Forwards direction only
          lower = (v - limit + n) % n;
          upper = (v + limit) % n;
          add_edges(v, limit, lower, upper, true);
      }
  }

  // Directed graphs
  else {
      // Unoriented starting graph
      if (!oriented) {
          const std::size_t limit = k/2;
          for (vertices_size_type v = 0; v < n; ++v) {
              // Forwards direction
              lower = (v + n - limit) % n;
              upper = (v + limit) % n;
              add_edges(v, limit, lower, upper, true);

              // Backwards direction
              lower = (v + n - limit) % n;
              upper = v;
              add_edges(v, limit, lower, upper, false);
          }
      }

      // Oriented starting graph
      else {
          const std::size_t limit = k;
          for (vertices_size_type v = 0; v < n; ++v) {
              // Forwards direction only, but with neighborhood range k
              lower = v;
              upper = (v + limit) % n;
              add_edges(v, limit, lower, upper, true);
          }
      }
  }

  // Return the graph
  return g;
}


// .. Convenient graph creation function ......................................

/// Create a graph from a configuration node
/** Select a graph creation algorithm and create the graph object a
 *  configuration node.
 *
 * \tparam Graph        The graph type
 * \tparam Config       The configuration node type
 * \tparam RNG          The random number generator type
 *
 * \param cfg           The configuration
 * \param rng           The random number generator
 * \param pmaps         Property maps that *may* be used by the graph
 *                      creation algorithms. At this point, only the
 *                      `load_from_file` model will make use of this,
 *                      allowing to populate a `weight` property map.
 *
 * \return Graph        The graph
 */

template<typename Graph, typename RNG>
Graph create_graph(const Config& cfg,
                   RNG& rng,
                   boost::dynamic_properties pmaps
                        = {boost::ignore_other_properties})
{
    // Get the graph generating model
    const std::string model = get_as<std::string>("model", cfg);

    // Call the correct graph creation algorithm depending on the configuration
    // node.

    if (model == "complete")
    {
      return create_complete_graph<Graph>(
                  get_as<std::size_t>("num_vertices", cfg));
    }

    else if (model == "regular")
    {

        // Get the model-specific configuration options
        const auto& cfg_r = get_as<Config>("regular", cfg);

        return create_regular_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<bool>("oriented", cfg_r));
    }
    else if (model == "ErdosRenyi")
    {
        // Get the model-specific configuration options
        const auto& cfg_ER = get_as<Config>("ErdosRenyi", cfg);

        return create_ErdosRenyi_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<bool>("parallel", cfg_ER),
                    get_as<bool>("self_edges", cfg_ER),
                    rng);
    }
    else if (model == "KlemmEguiluz")
    {
        // Get the model-specific configuration options
        const auto& cfg_KE = get_as<Config>("KlemmEguiluz", cfg);

        return create_KlemmEguiluz_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<double>("mu", cfg_KE),
                    rng);
    }
    else if (model == "WattsStrogatz")
    {
        // Get the model-specific configuration options
        const auto& cfg_WS = get_as<Config>("WattsStrogatz", cfg);

        return create_WattsStrogatz_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<double>("p_rewire", cfg_WS),
                    get_as<bool>("oriented", cfg_WS),
                    rng);
    }
    else if (model == "BarabasiAlbert")
    {
        // Get the model-specific configuration options
        const auto& cfg_BA = get_as<Config>("BarabasiAlbert", cfg);

        return create_BarabasiAlbert_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<std::size_t>("mean_degree", cfg),
                    get_as<bool>("parallel", cfg_BA),
                    rng);
    }
    else if (model == "BollobasRiordan")
    {
        // Get the model-specific configuration options
        const auto& cfg_BR = get_as<Config>("BollobasRiordan", cfg);

        return create_BollobasRiordan_graph<Graph>(
                    get_as<std::size_t>("num_vertices", cfg),
                    get_as<double>("alpha", cfg_BR),
                    get_as<double>("beta", cfg_BR),
                    get_as<double>("gamma", cfg_BR),
                    get_as<double>("del_in", cfg_BR),
                    get_as<double>("del_out", cfg_BR),
                    rng);
    }
    else if (model == "load_from_file")
    {
        // Get the model-specific configuration options
        const auto& cfg_lff = get_as<Config>("load_from_file", cfg);

        // Load and return the Graph via DataIO's loader
        return Utopia::DataIO::GraphLoad::load_graph<Graph>(cfg_lff, pmaps);

    }
    else {
        throw std::invalid_argument("The given graph model '" + model +
                                    "'does not exist! Valid options are: "
                                    "'complete', 'regular', 'ErdosRenyi', "
                                    "'WattsStrogatz', 'BarabasiAlbert', "
                                    "'KlemmEguiluz','BollobasRiordan', "
                                    "'load_from_file'.");
    }
}

/**
 *  \}
 */


} // namespace Graph
} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_CREATION_HH
