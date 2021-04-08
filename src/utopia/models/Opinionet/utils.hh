#ifndef UTOPIA_MODELS_OPINIONET_UTILS
#define UTOPIA_MODELS_OPINIONET_UTILS

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace Utopia::Models::Opinionet::Utils {

// .. Random distribution utility functions ...................................
// Generate a random number within the given range
template<typename RT, typename T, typename RNGType>
RT get_rand(std::pair<T, T> range, RNGType& rng) {
    //Note T: unnecessary check, already part of e.g. uniform_real_distribution
    if (!std::is_floating_point<RT>() and !std::is_integral<RT>()) {
        throw std::invalid_argument(
            "get_rand supports only integer or floating point return types!"
        );
    }
    if (range.first > range.second) {
        throw std::invalid_argument(
            "Error, invalid parameter range! Upper limit has to be higher "
            "than the lower limit."
        );
    }

    if constexpr (std::is_floating_point<RT>()) {
        return std::uniform_real_distribution<RT>(range.first, range.second)(rng);
    }
    else {
        return std::uniform_int_distribution<RT>(range.first, range.second)(rng);
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
    return std::is_convertible<
        typename boost::graph_traits<NWType>::directed_category,
        boost::directed_tag>::value;
}

// Get random neighbour of vertex v (for directed and undirected graphs).
// Only appliable to vertices with degree > 0
template<typename NWType, typename VertexDescType, typename RNGType>
auto get_rand_neighbor(VertexDescType& v, NWType& nw, RNGType& rng) {

    int nb_shift = get_rand<int>(
            std::make_pair<int, int>(0, boost::out_degree(v, nw)-1), rng
    );
    auto nb = boost::adjacent_vertices(v, nw).first;
    nb += nb_shift;
    return *nb;
}

// Select random neighbor with probability proportional to edge weight.
// Only applicable to vertices with degree > 0
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
            cumulative_weights += nw[boost::edge(v, w, nw).first].weight;
            if (cumulative_weights >= nb_prob_frac) {
                nb = w;
                break;
            }
        }
    }
    else {
        nb = Utils::get_rand_neighbor(v, nw, rng);
    }

    return nb;
}

// Iterate over a vertex' out-edges and set the weights to
// 1/(opinion difference), then normalize them.
template<typename NWType, typename VertexDescType>
void set_and_normalize_weights(const VertexDescType v, NWType& nw)
{
    double weight_norm = 0.;
    for (const auto e : range<IterateOver::out_edges>(v, nw)) {
        double op_diff = fabs(nw[target(e, nw)].opinion-nw[v].opinion);
        nw[e].weight = 1./op_diff;
        weight_norm += nw[e].weight;
    }
    if (weight_norm != 0.) {
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            nw[e].weight /= weight_norm;
        }
    }
}

} // namespace Utopia::Models::Opinionet::Utils

#endif // UTOPIA_MODELS_OPINIONET_UTILS
