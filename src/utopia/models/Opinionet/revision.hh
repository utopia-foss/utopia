#ifndef UTOPIA_MODELS_OPINIONET_REVISION
#define UTOPIA_MODELS_OPINIONET_REVISION

#include <cmath>

#include "modes.hh"
#include "utils.hh"


namespace Utopia::Models::Opinionet::Revision {

using modes::Interaction_type;
using modes::Opinion_space_type;
using modes::Rewiring;

// .. Opinion update functions ................................................

/// Hegselmann-Krause opinion update function
template<typename NWType, typename VertexDescType>
void update_opinion_HK (
    const VertexDescType v,
    NWType& nw,
    const double susceptibility,
    const double tolerance)
{
    double expectation = 0;
    size_t num_interaction_partners = 0;

    // Directed case: Calculate the expectation based on the probability
    // distribution given by the weights.
    if constexpr (Utils::is_directed<NWType>()) {
        VertexDescType nb;
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            nb = target(e, nw);
            if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
                expectation += nw[nb].opinion * nw[e].weight;
                ++num_interaction_partners;
            }
        }

        // If interaction partners found: normalize weighted opinion average to
        // number of interaction partners participating in interaction.
        if (num_interaction_partners != 0) {
            expectation *= static_cast<double>(boost::out_degree(v, nw)) /
                  num_interaction_partners;
        }
        else {
            expectation = nw[v].opinion;
        }
    }

    // Undirected case: Calculate the opinion average
    else {
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            auto nb = target(e, nw);
            if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
                expectation += nw[nb].opinion;
                ++num_interaction_partners;
            }
        }

        // If interaction partners found: normalize opinion average to
        // number of interaction partners
        if (num_interaction_partners != 0) {
            expectation /= num_interaction_partners;
        }
        else {
            expectation = nw[v].opinion;
        }
    }

    // Update opinion
    nw[v].opinion += susceptibility * (expectation - nw[v].opinion);
}

/// Deffuant opinion update function
template<Opinion_space_type op_space,
         typename NWType, typename RNGType, typename VertexDescType>
void update_opinion_Deffuant (
    const VertexDescType v,
    NWType& nw,
    const double susceptibility,
    const double tolerance,
    std::uniform_real_distribution<double>& prob_distr,
    RNGType& rng)
{
    // Get neighbor
    const VertexDescType nb = Utils::select_neighbor(v, nw, prob_distr, rng);

    // Discrete case: adopt nb opinion with probability = susceptibility
    if constexpr (op_space == Opinion_space_type::discrete) {
        if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
            const double interaction_probability = prob_distr(rng);
            if (interaction_probability < susceptibility) {
                nw[v].opinion = nw[nb].opinion;
            }
        }
    }

    // Continuous case: move towards nb opinion proportional to susceptibility
    else {
        if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
            nw[v].opinion += susceptibility * (nw[nb].opinion - nw[v].opinion);
        }
    }
}

// .. Rewiring ................................................................

/// Selects a random edge. If the opinion distance of the source and target
/// exceeds the tolerance, the edge is rewired to a random target.
template<typename NWType, typename RNGType>
void rewire_random_edge(
    NWType& nw,
    const double tolerance,
    const double weighting,
    RNGType& rng)
{
    using namespace boost;
    // Choose random edge for rewiring
    const auto e = random_edge(nw, rng);
    const auto s = source(e, nw);

    if (fabs(nw[s].opinion - nw[target(e, nw)].opinion) > tolerance) {
        const auto new_target = random_vertex(nw, rng);

        if (new_target != s and not edge(s, new_target, nw).second)
          {
            remove_edge(e, nw);
            add_edge(s, new_target, nw);

            if constexpr (Utils::is_directed<NWType>()) {
                Utils::set_and_normalize_weights(s, nw, weighting);
            }
        }
    }
}

// .. Revision ................................................................

/// Performs an opinion update and edge rewiring (if enabled).
template<Interaction_type interaction_type,
         Opinion_space_type op_space,
         Rewiring rewire,
         typename NWType,
         typename RNGType>
void revision(
    NWType& nw,
    const double susceptibility,
    const double tolerance,
    const double weighting,
    std::uniform_real_distribution<double>& prob_distr,
    RNGType& rng)
{
    // Choose random vertex for revision
    const auto v = random_vertex(nw, rng);

    if (out_degree(v, nw) != 0) {

        if constexpr (interaction_type == Interaction_type::HegselmannKrause) {
            update_opinion_HK(
                v, nw, susceptibility, tolerance
            );
        }

        else if constexpr (interaction_type == Interaction_type::Deffuant) {
            update_opinion_Deffuant<op_space>(
                v, nw, susceptibility, tolerance, prob_distr, rng
            );
        }

        if constexpr (op_space == Opinion_space_type::discrete) {
            nw[v].opinion = round(nw[v].opinion);
        }

        if constexpr (Utils::is_directed<NWType>()) {
            Utils::set_and_normalize_weights(v, nw, weighting);
        }
    }

    if constexpr (rewire == Rewiring::RewiringOn) {
        rewire_random_edge(nw, tolerance, weighting, rng);
    }
}

} // namespace Utopia::Models::Opinionet::Revision

#endif // UTOPIA_MODELS_OPINIONET_REVISION
