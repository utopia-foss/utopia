#ifndef UTOPIA_CORE_GRAPH_APPLY_HH
#define UTOPIA_CORE_GRAPH_APPLY_HH

#include <boost/bind.hpp>

#include "graph_iterators.hh"
#include "graph_apply.hh"
#include "apply.hh"             // for Shuffle enum
#include "state.hh"             // for Update enum
#include "zip.hh"

namespace Utopia {


/**
 *  \addtogroup Rules
 *  \{
 */


template<Over over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void apply_rule(Rule&& rule, Graph&& g)
{
    if constexpr (mode == Update::sync) {
        // Get the iterators
        auto [it, it_end] = iterate<over>(g);

        // initialize the state cache
        using State = typename std::iterator_traits<decltype(it)>::value_type::State;
        std::vector<State> state_cache;
        state_cache.reserve(std::distance(it, it_end));

        // apply the rule
        std::transform(it, it_end,
                    back_inserter(state_cache),
                    std::forward<Rule>(
                        boost::bind(&rule, _1, g)));

        // move the cache
        Utopia::Itertools::ZipIterator zip_it_begin(it, std::begin(state_cache));
        Utopia::Itertools::ZipIterator zip_it_end(it, std::end(state_cache));
        for (auto zip_it = zip_it_begin; zip_it != zip_it_end; ++zip_it){
            auto [entity, state_cached] = *zip_it;
            g[entity].state = std::move(state_cached);
        }
    }

    else if constexpr (mode == Update::async){
        // Apply the rule to each element 
        for (auto g_entity : range<over>(g)){
            g[g_entity].state = rule(g_entity, g);
        }
    }
}


template<Over over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void apply_rule(Rule&& f, Graph&& g, RNG&& rng)
{
    // Get iterators ...
    auto [it, it_end] = iterate<over>(g);

    // ... and shuffle them
    std::shuffle(it, it_end, rng);

    if constexpr (mode == Update::sync) {
        // TODO implement
    }

    else if constexpr (mode == Update::async){
        // Apply the rule to each element 
        for (auto g_entity : boost::make_iterator_range(it, it_end)){
            g[g_entity].state = rule(g_entity, g);
        }
    }
}


/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_APPLY_HH    