#ifndef UTOPIA_CORE_GRAPH_APPLY_HH
#define UTOPIA_CORE_GRAPH_APPLY_HH

#include <type_traits>
#include <boost/graph/graph_traits.hpp>

#include "iterator.hh"
#include "apply.hh"
#include "../apply.hh"             // for Shuffle enum
#include "../state.hh"             // for Update enum
#include "../zip.hh"

namespace Utopia {


/**
 *  \addtogroup Rules
 *  \{
 */

template<typename Graph, typename Iter, typename Rule>
void _apply_async(Iter it_begin, Iter it_end, Graph&& g, Rule&& rule)
{
    // Determine whether the lambda function returns a void
    using GraphType = typename std::remove_reference_t<Graph>;
    using VertexDesc = typename boost::graph_traits<GraphType>
                                                        ::vertex_descriptor;
    using ReturnType = typename std::invoke_result_t<Rule, VertexDesc>;
    constexpr bool lambda_returns_void = std::is_same_v<ReturnType, void>;

    // Apply the rule to each element 
    if constexpr (lambda_returns_void){
        std::for_each(it_begin, it_end, rule);
    }
    else {
        std::for_each(it_begin, it_end, [&rule, &g](auto g_entity){
            g[g_entity].state = rule(g_entity);
        });
    }
}


template<typename Iter, typename Graph, typename Rule>
void _apply_sync(Iter it, Iter it_end, Graph&& g, Rule&& rule)
{
    // initialize the state cache
    using State = 
        typename std::iterator_traits<decltype(it)>::value_type::State;
    std::vector<State> state_cache;
    state_cache.reserve(std::distance(it, it_end));

    // apply the rule
    std::transform(it, it_end,
                back_inserter(state_cache),
                std::forward<Rule>(rule));

    // move the cache
    Itertools::ZipIterator zip_it_begin(it, std::begin(state_cache));
    Itertools::ZipIterator zip_it_end(it_end, std::end(state_cache));
    for (auto zip_it = zip_it_begin; zip_it != zip_it_end; ++zip_it){
        auto [entity, state_cached] = *zip_it;
        g[entity].state = std::move(state_cached);
    }
}


template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void apply_rule(Rule&& rule, Graph&& g)
{
    auto [it, it_end] = Utopia::GraphUtil::iterator_pair<iterate_over>(g);

    if constexpr (mode == Update::sync) {
        _apply_sync(it, it_end, g, rule);
    }
    else if constexpr (mode == Update::async){
        _apply_async(it, it_end, g, rule);
    }
}


template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void apply_rule(Rule&& rule, Graph&& g, RNG&& rng)
{
    // Get types
    using GraphType = typename std::remove_reference_t<Graph>;
    using VertexDesc = typename boost::graph_traits<GraphType>
                                                        ::vertex_descriptor;
    
    // Get the iterators, create a vector with a copy because the 
    // original iterators are const, thus cannot be shuffed,
    // and shuffle them.
    auto [it, it_end] = Utopia::GraphUtil::iterator_pair<iterate_over>(g);
    std::vector<VertexDesc> it_shuffled(it, it_end);
    std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), rng);

    if constexpr (mode == Update::sync) {
        _apply_sync(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
    else if constexpr (mode == Update::async){
        // Apply the rule to each element asynchronously
        _apply_async(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
}


/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_APPLY_HH    