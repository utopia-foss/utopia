#ifndef UTOPIA_MODELS_OPINIONET_UTILS
#define UTOPIA_MODELS_OPINIONET_UTILS

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace Utopia::Models::Opinionet::Utils {

// .. Random distribution utility functions ...................................

// Generate a random number in the range [0, upper_limit]
template<typename RT, typename T, typename RNGType>
RT get_rand(T upper_limit, RNGType& rng) {
    static_assert(std::is_floating_point<RT>() or std::is_integral<RT>(),
        "get_rand supports only integer or floating point return types!");

    if constexpr (std::is_floating_point<RT>()) {
        return std::uniform_real_distribution<RT>(0., upper_limit)(rng);
    }
    else {
        return std::uniform_int_distribution<RT>(0, upper_limit)(rng);
    }
}

// Generate a random number within the given range
template<typename RT, typename T, typename RNGType>
RT get_rand(std::pair<T, T> range, RNGType& rng) {

    if (range.second >= range.first) {
        return get_rand<RT>(range.second - range.first, rng) + range.first;
    }
    else {
        throw std::invalid_argument(
            "Error, invalid parameter range! Upper limit has to be higher "
            "than the lower limit."
        );
    }
}

// Generate a random normally-distributed double
template<typename RNGType>
double get_rand_double_from_gaussian(double mu, double sigma, RNGType& rng) {

    std::normal_distribution<double> distribution(mu, sigma);
    return (double)distribution(rng);
}


// .. Network utility functions ................................................

// Check whether the network type allows for directed edges
template<typename NWType>
constexpr bool is_directed() {
    return std::is_same<
        typename boost::graph_traits<NWType>::directed_category,
        boost::directedS>::value;
}

// Get random neighbour of vertex v (for directed and undirected graphs).
template<typename NWType, typename VertexDescType, typename RNGType>
auto get_rand_neighbor(NWType& nw, VertexDescType& v, RNGType& rng) {

    int nb_shift = get_rand<int>(boost::out_degree(v, nw) - 1, rng);
    auto nb = boost::adjacent_vertices(v, nw).first;
    nb += nb_shift;
    return *nb;
}

// Select random neighbor with probability proportional to edge weight.
template<typename NWType, typename RNGType, typename VertexDescType>
VertexDescType select_neighbor(
    const VertexDescType v,
    NWType& nw,
    std::uniform_real_distribution<double> prob_distr,
    RNGType& rng)
{
    auto nb = v;

    if constexpr (Utils::is_directed<NWType>()) {
        // The probability for choosing neighbor w is given by the weight on
        // the edge (v, w).
        const double nb_prob_frac = prob_distr(rng);
        double cumulative_weights = 0.;
        for (const auto w : range<IterateOver::neighbors>(v, nw)) {
            if (cumulative_weights < nb_prob_frac) {
                cumulative_weights += nw[boost::edge(v, w, nw).first].weight;
            }
            else {
                nb = w;
                break;
            }
        }
    }
    else {
        nb = Utils::get_rand_neighbor(nw, v, rng);
    }

    return nb;
}

// Normalize the weights of the outgoing edges of vertex v to 1.
template<typename NWType, typename VertexDescType>
void normalize_weights(const VertexDescType v, NWType& nw) {

    double weight_norm = 0.;
    for (const auto e : range<IterateOver::out_edges>(v, nw)) {
        weight_norm += nw[e].weight;
    }
    if (weight_norm != 0.) {
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            nw[e].weight /= weight_norm;
        }
    }
}

// Iterate over a vertex' out-edges and set the weights to
// 1/(opinion difference), then normalize them.
template<typename NWType, typename VertexDescType>
void set_and_normalize_weights(const VertexDescType v, NWType& nw)
{
    double weight_norm = 0.;
    for (const auto e : range<IterateOver::out_edges>(v, nw)) {
        double op_diff = fabs(nw[target(e, nw)].opinion-nw[v].opinion);
        weight_norm += op_diff;
        nw[e].weight = op_diff;
    }
    if (weight_norm != 0.) {
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            nw[e].weight /= weight_norm;
        }
    }
}

} // namespace Utopia::Models::Opinionet::Utils

#endif // UTOPIA_MODELS_OPINIONET_UTILS
