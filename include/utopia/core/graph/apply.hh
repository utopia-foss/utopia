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


/** \ingroup Graph
 *  \addtogroup Rules
 *  \{
 */


namespace GraphUtils{

/// Apply a rule asynchronously
/** This helper function applies a rule to a range of entities that is given
 *  through an iterator pair one after the other.
 * 
 * \tparam Iter     The iterator type
 * \tparam Graph    The graph type
 * \tparam Rule     The rule type
 * 
 * \param it_begin  The begin of the graph entity iterator range.
 * \param it_end    The end of the graph entity iterator range.
 * \param g         The graph
 * \param rule      The rule function to be applied to each element within the
 *                  iterator range.
 */
template<typename Graph, typename Iter, typename Rule>
void apply_async(Iter it_begin, Iter it_end, Graph&& g, Rule&& rule)
{
    // Determine whether the lambda function returns a void
    using GraphType = typename std::remove_reference_t<Graph>;
    using VertexDesc =
        typename boost::graph_traits<GraphType>::vertex_descriptor;
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

/// Apply a rule synchronously
/** This helper function applies a rule to a range of entities that is given
 *  through an iterator pair.
 *  A state cache is created that stores the returned states of the rule 
 *  function. After the rule was applied to each graph entity within the 
 *  iterator range the cached states are moved to the actual states of the
 *  graph entities, thus, updating their states synchronously.
 * 
 * \tparam Iter     The iterator type
 * \tparam Graph    The graph type
 * \tparam Rule     The rule type
 * 
 * \param it_begin  The begin of the graph entity iterator range.
 * \param it_end    The end of the graph entity iterator range.
 * \param g         The graph
 * \param rule      The rule function to be applied to each element within the
 *                  iterator range.
 * 
 * \warning Be careful to not operate directly on the state of a graph entity
 *          within the rule function. Rather, first create a copy of the state
 *          and return the copied and changed state at the end of the function.
 */
template<typename Iter, typename Graph, typename Rule>
void apply_sync(Iter it_begin, Iter it_end, Graph&& g, Rule&& rule)
{
    // initialize the state cache
    std::vector<decltype(g[*it_begin].state)> state_cache;
    state_cache.reserve(std::distance(it_begin, it_end));

    // apply the rule
    std::transform(it_begin, it_end,
                std::back_inserter(state_cache),
                std::forward<Rule>(rule));

    // move the cache
    auto counter = 0u;
    for (auto entity : boost::make_iterator_range(it_begin, it_end)){
        g[entity].state = std::move(state_cache[counter]);
        
        ++counter;
    }
}

} // namespace GraphUtils


/// Apply a rule on graph entity properties
/** This overload specified the apply_rule function for not shuffled entities.
 * 
 * \tparam iterate_over Over which graph entity to iterate over. See 
 *          \ref IterateOver
 * \tparam mode         The update mode \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 *  
 * \param rule          The rule that takes a graph entity descriptor
 *                      (vertex_descriptor or edge_descriptor).
 *                      If the graph entity states are updated synchronously
 *                      the rule function needs to return a copied state and
 *                      changed state that overwrites the old state.
 *                      Returning a state is optional for asynchronous update.
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle,
         typename Graph, 
         typename Rule,
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void apply_rule(Rule&& rule, Graph&& g)
{
    using namespace GraphUtils;

    auto [it, it_end] = Utopia::GraphUtils::iterator_pair<iterate_over>(g);

    if constexpr (mode == Update::sync) {
        apply_sync(it, it_end, g, rule);
    }
    else if constexpr (mode == Update::async){
        apply_async(it, it_end, g, rule);
    }
    else{
        static_assert((mode == Update::async or mode == Update::sync), 
            "apply_rule only works with 'Update::async' or 'Update::sync'!");
    }
}


/// Apply a rule on graph entity properties
/** This overload specified the apply_rule function for shuffled entities.
 * 
 * \tparam iterate_over Over which graph entity to iterate over. See 
 *          \ref IterateOver
 * \tparam mode         The update mode \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 * \tparam RNG          The random number generator type
 *  
 * \param rule          The rule that takes a graph entity descriptor
 *                      (vertex_descriptor or edge_descriptor).
 *                      If the graph entity states are updated synchronously
 *                      the rule function needs to return a copied state and
 *                      changed state that overwrites the old state.
 *                      Returning a state is optional for asynchronous update. 
 * \param g             The graph
 * \param rng           The random number generator
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void apply_rule(Rule&& rule, Graph&& g, RNG&& rng)
{
    using namespace GraphUtils;

    // Get types
    using GraphType = typename std::remove_reference_t<Graph>;
    using VertexDesc =
        typename boost::graph_traits<GraphType>::vertex_descriptor;
    
    // Get the iterators, create a vector with a copy because the 
    // original iterators are const, thus cannot be shuffed,
    // and shuffle them.
    auto [it, it_end] = Utopia::GraphUtils::iterator_pair<iterate_over>(g);
    std::vector<VertexDesc> it_shuffled(it, it_end);
    std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), rng);

    if constexpr (mode == Update::sync) {
        apply_sync(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
    else if constexpr (mode == Update::async){
        // Apply the rule to each element asynchronously
        apply_async(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
    else{
        static_assert((mode == Update::async or mode == Update::sync), 
            "apply_rule only works with 'Update::async' or 'Update::sync'!");
    }
}


/// Apply a rule on graph entity properties
/** This overload specified the apply_rule function for not shuffled entities 
 *  where getting the correct iterator pair is dependent on a parent_vertex, 
 *  for example if the rule should be applied to the neighbors, inv_neighbors, 
 *  in_degree, out_degree or degree wrt. the parent_vertex.
 * 
 * \tparam iterate_over Over which graph entity to iterate over. See 
 *          \ref IterateOver
 * \tparam mode         The update mode \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 *  
 * \param rule          The rule that takes a graph entity descriptor
 *                      (vertex_descriptor or edge_descriptor).
 *                      If the graph entity states are updated synchronously
 *                      the rule function needs to return a copied state and
 *                      changed state that overwrites the old state.
 *                      Returning a state is optional for asynchronous update.
 * \param parent_vertex The parent vertex 
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle,
         typename Graph, 
         typename Rule,
         typename VertexDesc = typename boost::graph_traits<
            std::remove_reference_t<Graph>>::vertex_descriptor, 
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void apply_rule(Rule&& rule, 
                VertexDesc parent_vertex,
                Graph&& g)
{
    using namespace GraphUtils;

    auto [it, it_end] = Utopia::GraphUtils::iterator_pair<iterate_over>
                            (parent_vertex, g);

    if constexpr (mode == Update::sync) {
        apply_sync(it, it_end, g, rule);
    }
    else if constexpr (mode == Update::async){
        apply_async(it, it_end, g, rule);
    }
    else{
        static_assert((mode == Update::async or mode == Update::sync), 
            "apply_rule only works with 'Update::async' or 'Update::sync'!");
    }
}


/// Apply a rule on graph entity properties
/** This overload specified the apply_rule function for shuffled entities 
 *  where getting the correct iterator pair is dependent on a parent_vertex, 
 *  for example if the rule should be applied to the neighbors, inv_neighbors, 
 *  in_degree, out_degree or degree wrt. the parent_vertex.
 * 
 * \tparam iterate_over Over which graph entity to iterate over. See 
 *          \ref IterateOver
 * \tparam mode         The update mode \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 * \tparam RNG          The random number generator type
 *  
 * \param rule          The rule that takes a graph entity descriptor
 *                      (vertex_descriptor or edge_descriptor).
 *                      If the graph entity states are updated synchronously
 *                      the rule function needs to return a copied state and
 *                      changed state that overwrites the old state.
 *                      Returning a state is optional for asynchronous update.
 * \param parent_vertex The parent vertex 
 * \param g             The graph
 * \param rng           The random number generator
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename VertexDesc = typename boost::graph_traits<
            std::remove_reference_t<Graph>>::vertex_descriptor,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void apply_rule(Rule&& rule, 
                const VertexDesc parent_vertex, 
                Graph&& g, 
                RNG&& rng)
{
    using namespace GraphUtils;

    // Get the iterators, create a vector with a copy because the 
    // original iterators are const, thus cannot be shuffed,
    // and shuffle them.
    auto [it, it_end] = Utopia::GraphUtils::iterator_pair<iterate_over>
                            (parent_vertex, g);
    std::vector<VertexDesc> it_shuffled(it, it_end);
    std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), rng);

    if constexpr (mode == Update::sync) {
        apply_sync(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
    else if constexpr (mode == Update::async){
        // Apply the rule to each element asynchronously
        apply_async(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
    }
    else{
        static_assert((mode == Update::async or mode == Update::sync), 
            "apply_rule only works with 'Update::async' or 'Update::sync'!");
    }
}

/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_APPLY_HH    