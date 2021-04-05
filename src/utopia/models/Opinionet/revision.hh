#ifndef UTOPIA_MODELS_OPINIONET_REVISION
#define UTOPIA_MODELS_OPINIONET_REVISION

#include <math.h>
#include "modes.hh"
#include "utils.hh"


namespace Utopia::Models::Opinionet::Revision {

using modes::Interaction_type;
using modes::Opinion_space_type;
using modes::Rewiring;

// .. Opinion update functions .................................................

// Hegselmann-Krause opinion update function
template<typename NWType, typename VertexDescType>
void update_opinion_HK (
    const VertexDescType v,
    NWType& nw,
    const double susceptibility,
    const double tolerance)
{

    double new_opinion = nw[v].opinion;
    size_t number_of_interaction_partners = 0;

    if constexpr (Utils::is_directed<NWType>()) {
        // Calculate weighted average opinion
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            auto nb = target(e, nw);
            if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
                new_opinion += nw[nb].opinion * nw[e].weight;
                ++number_of_interaction_partners;
            }
        }

        // Normalize weighted opinion average to number of interaction partners
        // participating in interaction
        new_opinion *= out_degree(v, nw) / number_of_interaction_partners;
    }

    else {
        for (const auto e : range<IterateOver::out_edges>(v, nw)) {
            auto nb = target(e, nw);
            if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
                new_opinion += nw[nb].opinion;
                ++number_of_interaction_partners;
            }
        }

        // Normalize weighted opinion average to number of interaction partners
        // participating in interaction
        new_opinion /= number_of_interaction_partners;
    }

    // Update opinion
    nw[v].opinion += susceptibility * (new_opinion - nw[v].opinion);

}

// Deffuant update function
template<Opinion_space_type op_space,
         typename NWType, typename RNGType, typename VertexDescType>
void update_opinion_Deffuant (
    const VertexDescType v,
    NWType& nw,
    const double susceptibility,
    const double tolerance,
    std::uniform_real_distribution<double> prob_distr,
    RNGType& rng)
{
    // Get neighbor
    auto nb = Utils::select_neighbor(v, nw, prob_distr, rng);

    // Discrete case: flip to nb opinion if probability = susceptibility
    if constexpr (op_space == Opinion_space_type::discrete) {
        if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
            const double interaction_probability = prob_distr(rng);
            if (interaction_probability < susceptibility) {
                nw[v].opinion = nw[nb].opinion;
            }
        }
    }

    // Continuous case: move towards nb opinion proportionally to susceptibility
    else {
        if (fabs(nw[v].opinion - nw[nb].opinion) <= tolerance) {
            nw[v].opinion += susceptibility * (nw[nb].opinion - nw[v].opinion);
        }
    }

}

// .. Rewiring .................................................................

// Selects a random edge. If the opinion distance of the source and target
// exceed the tolerance, the edge is rewired to a random target.
template<typename NWType, typename RNGType>
void rewire_random_edge(NWType& nw, const double tolerance, RNGType& rng)
{
    // Choose random edge for rewiring
    auto e = random_edge(nw, rng);
    auto s = source(e, nw);

    if (fabs(nw[s].opinion - nw[target(e, nw)].opinion) > tolerance) {
          auto new_target = boost::random_vertex(nw, rng);

          if (new_target != s and not boost::edge(s, new_target, nw).second)
          {
              boost::remove_edge(e, nw);
              boost::add_edge(s, new_target, nw);

              if constexpr (Utils::is_directed<NWType>()) {
                  Utils::set_and_normalize_weights(s, nw);
              }
          }
      }
}

// .. Revision .................................................................

template<Interaction_type interaction_type,
         Opinion_space_type op_space,
         Rewiring rewire,
         typename NWType,
         typename RNGType>
void revision(
        NWType& nw,
        const double susceptibility,
        const double tolerance,
        std::uniform_real_distribution<double> prob_distr,
        RNGType& rng)
{

    // Choose random vertex for revision
    auto v = random_vertex(nw, rng);

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
            Utils::set_and_normalize_weights(v, nw);
        }

        if constexpr (rewire == Rewiring::RewiringOn) {
            rewire_random_edge(nw, tolerance, rng);
        }
    }
}

} // namespace Utopia::Models::Opinionet::Revision

#endif // UTOPIA_MODELS_OPINIONET_REVISION
