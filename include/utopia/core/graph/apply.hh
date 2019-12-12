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
    // The lambda function can either be called with a vertex descriptor or 
    // and edge descriptor. Therefore, both cases need to be accounted for
    using Desc = typename std::iterator_traits<Iter>::value_type;
    using ReturnType = typename std::invoke_result_t<Rule, Desc, Graph>;
    constexpr bool lambda_returns_void = std::is_same_v<ReturnType, void>;

    // Apply the rule to each element 
    if constexpr (lambda_returns_void){
        std::for_each(it_begin, it_end, 
        [&rule, &g](auto g_entity){
            rule(g_entity, g);
        });
    }
    else {
        std::for_each(it_begin, it_end, [&rule, &g](auto g_entity){
            g[g_entity].state = rule(g_entity, g);
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
    for (auto entity : boost::make_iterator_range(it_begin, it_end)){
        state_cache.push_back(rule(entity, g));
    }

    // move the cache
    auto counter = 0u;
    for (auto entity : boost::make_iterator_range(it_begin, it_end)){
        g[entity].state = std::move(state_cache[counter]);
        
        ++counter;
    }
}

} // namespace GraphUtils



// ----------------------------------------------------------------------------
// apply_rule definitions WITHOUT the need for a reference vertex

/// Synchronously apply a rule to graph entities
/** This overload specifies the apply_rule function for a synchronous update.
 *  In such a case, it makes no sense to shuffle, so the shuffle option is not
 *  available here.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 *  
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For the synchronous update, the rule
 *                      function needs to return the new state.
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         typename Graph, 
         typename Rule,
         typename std::enable_if_t<mode == Update::sync, int> = 0>
void apply_rule(Rule&& rule, Graph&& g)
{
    using namespace GraphUtils;
    auto [it, it_end] = iterator_pair<iterate_over>(g);
    apply_sync(it, it_end, g, rule);
}


/// Asynchronously apply a rule to graph entities, without shuffling
/** This overload specifies the apply_rule function for an asynchronous update.
 *  
 * \warning Not shuffling a rule often creates unwanted artifacts. Thus, to use
 *          this function, the Shuffle::off template argument needs to be
 *          given explicitly.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 *  
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For an asynchronous update, returning the
 *                      state is optional.
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle,
         typename Graph, 
         typename Rule,
         typename std::enable_if_t<mode == Update::async, int> = 0>
void apply_rule(Rule&& rule, Graph&& g)
{
    static_assert(shuffle == Shuffle::off,
        "Refusing to asynchronously apply a rule without shuffling. Either "
        "explicitly specify Shuffle::off or pass an RNG to apply_rule to "
        "allow shuffling.");

    using namespace GraphUtils;
    auto [it, it_end] = iterator_pair<iterate_over>(g);
    apply_async(it, it_end, g, rule);
}


/// Asynchronously, in shuffled order, apply a rule to graph entities
/** Using the given RNG, the iteration order is shuffled before the rule is
 *  applied sequentially to the specified entities.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 * \tparam RNG          The random number generator type
 *  
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For an asynchronous update, returning the
 *                      state is optional.
 * \param g             The graph
 * \param rng           The random number generator
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void apply_rule(Rule&& rule, Graph&& g, RNG&& rng)
{
    using namespace GraphUtils;

    // Get the iterators and create a vector with a copy because the original
    // iterators are const, thus cannot be shuffled. Then shuffle.
    auto [it, it_end] = Utopia::GraphUtils::iterator_pair<iterate_over>(g);
    using Desc = typename std::iterator_traits<decltype(it)>::value_type;
    std::vector<Desc> it_shuffled(it, it_end);
    
    std::shuffle(std::begin(it_shuffled),
                 std::end(it_shuffled),
                 std::forward<RNG>(rng));

    // Now with the shuffled container, apply the rule to each element
    apply_async(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
}



// ----------------------------------------------------------------------------
// apply_rule definitions WITH the need for a reference vertex


/// Synchronously apply a rule to graph entities
/** This overload specified the apply_rule function for not shuffled entities 
 *  where getting the correct iterator pair is dependent on a ref_vertex, 
 *  for example if the rule should be applied to the neighbors, inv_neighbors, 
 *  in_degree, out_degree or degree wrt. the ref_vertex.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the container
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 * 
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For the synchronous update, the rule
 *                      function needs to return the new state.
 * \param ref_vertex    Reference vertex descriptor to create the iterator from
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         typename Graph, 
         typename Rule,
         typename VertexDesc =
            typename boost::graph_traits<
                std::remove_reference_t<Graph>>::vertex_descriptor,
         typename std::enable_if_t<mode == Update::sync, int> = 0>
void apply_rule(Rule&& rule, 
                const VertexDesc ref_vertex,
                Graph&& g)
{
    using namespace GraphUtils;
    auto [it, it_end] = iterator_pair<iterate_over>(ref_vertex, g);
    apply_sync(it, it_end, g, rule);
}


/// Asynchronously apply a rule to graph entities, without shuffling
/** This overload specifies the apply_rule function for an asynchronous update.
 *  
 * \warning Not shuffling a rule often creates unwanted artifacts. Thus, to use
 *          this function, the Shuffle::off template argument needs to be
 *          given explicitly.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 *  
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For an asynchronous update, returning the
 *                      state is optional.
 * \param ref_vertex    Reference vertex descriptor to create the iterator from
 * \param g             The graph
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename VertexDesc =
            typename boost::graph_traits<
                std::remove_reference_t<Graph>>::vertex_descriptor,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void apply_rule(Rule&& rule,
                const VertexDesc ref_vertex,
                Graph&& g)
{
    using namespace GraphUtils;
    auto [it, it_end] = iterator_pair<iterate_over>(ref_vertex, g);
    apply_async(it, it_end, g, rule);
}


/// Asynchronously, in shuffled order, apply a rule to graph entities
/** This overload specified the apply_rule function for shuffled entities 
 *  where getting the correct iterator pair is dependent on a ref_vertex, 
 *  for example if the rule should be applied to the neighbors, inv_neighbors, 
 *  in_degree, out_degree or degree wrt. the ref_vertex.
 * 
 * \tparam iterate_over Over which kind of graph entity to iterate over. See 
 *                      \ref IterateOver
 * \tparam mode         The update mode, see \ref UpdateMode
 * \tparam Shuffle      Whether to shuffle the iteration
 * \tparam Graph        The graph type
 * \tparam Rule         The rule type
 * \tparam RNG          The random number generator type
 * 
 * \param rule          The rule function, expecting (descriptor, graph)
 *                      as arguments. For an asynchronous update, returning the
 *                      state is optional.
 * \param ref_vertex    Reference vertex descriptor to create the iterator from
 * \param g             The graph
 * \param rng           The random number generator
 */
template<IterateOver iterate_over,
         Update mode,
         Shuffle shuffle = Shuffle::on,
         typename Graph, 
         typename Rule,
         typename RNG,
         typename VertexDesc =
            typename boost::graph_traits<
                std::remove_reference_t<Graph>>::vertex_descriptor>
void apply_rule(Rule&& rule, 
                const VertexDesc ref_vertex, 
                Graph&& g, 
                RNG&& rng)
{
    static_assert(mode == Update::async,
                  "Shuffled apply_rule is only possible for Update::async!");

    using namespace GraphUtils;

    // Get the iterators and create a vector with a copy because the original
    // iterators are const, thus cannot be shuffled. Then shuffle.
    auto [it, it_end] = iterator_pair<iterate_over>(ref_vertex, g);
    std::vector<VertexDesc> it_shuffled(it, it_end);

    std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), rng);

    apply_async(std::begin(it_shuffled), std::end(it_shuffled), g, rule);
}


/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_APPLY_HH    
