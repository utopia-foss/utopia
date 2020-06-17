#ifndef UTOPIA_CORE_APPLY_HH
#define UTOPIA_CORE_APPLY_HH

#include <type_traits>
#include <vector>

#include "state.hh"
#include "zip.hh"


namespace Utopia {

namespace impl {
/// Return the element type of any container holding pointers to entities
template<class Container>
using entity_t = typename Container::value_type::element_type;
}


/**
 *  \addtogroup Rules
 *  \{
 */

/// Switch for enabling/disabling shuffling the cells for asynchronous updates
enum class Shuffle {
    /// Shuffle the container before applying the rule sequentially
    on,
    /// Immediately apply the rule sequentially
    off
};

/// Helper class for checking rule signatures and return types
/**
 *  This class checks if a rule can be invoked with the given arguments (note
 *  that `Args` are expected to be conainers/sequences of the actual arguments)
 *  and reports the rule return type. It also statically checks if the rule
 *  return type can be converted to the expected state, but only if the rule
 *  does not return `void`.
 *
 *  \tparam State The state type of the entities this rule is applied on
 *  \tparam Rule The type of the rule
 *  \tparam Args One or multiple containers with rule arguments
 */
template<typename State, typename Rule, typename... Args>
class rule_invoke_result
{
private:
  // Check if rule can be invoked
  static_assert(
    std::is_invocable_v<Rule,
                        typename std::remove_reference_t<Args>::reference...>,
    "Cannot invoke the Rule with the given container elements as arguments!"
    " Please check the rule signature!");

public:
    /// Report rule invoke result
    using type = std::invoke_result_t<
        Rule,
        typename std::remove_reference_t<Args>::reference...>;

private:
    // Check that rule returns type that can be converted to state
    static_assert(
        std::is_same_v<type, void> or std::is_convertible_v<type, State>,
        "Invoking the rule must return a type that can be converted to the "
        "Entity state type!");
};

/// Helper definition to query the rule result type
/** \see rule_invoke_result
 */
template<typename State, typename Rule, typename... Args>
using rule_invoke_result_t =
    typename rule_invoke_result<State, Rule, Args...>::type;

/// Helper function to check if the rule returns `void`
/** \see rule_invoke_result
 */
template<typename State, typename Rule, typename... Args>
constexpr bool is_void_rule ()
{
    return std::is_same_v<void, rule_invoke_result_t<State, Rule, Args...>>;
}

/// Helper function to create a tuple from a tuple using an index sequence
/**
 *  \tparam I Index sequence to access the elements of the tuple
 *  \param t Tuple to create a tuple from
 *  \return Tuple containing the elements of the input tuple
 */
template<class Tuple, std::size_t... I>
constexpr decltype(auto)
make_tuple_from_tuple_impl(Tuple&& t, std::index_sequence<I...>)
{
    return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}

/// Helper function to create a tuple from a tuple
/**
 *  This seems silly, but calling `std::make_tuple` decays objects of
 *  `std::reference_wrapper<T> to `T&`, which is exactly what we use this
 *  function for.
 *
 *  This function creates a index sequence for the tuple and calls the
 *  implementation helper make_tuple_from_tuple_impl().
 *
 *  \param t Tuple to create a tuple from
 *  \return Tuple containing the elements of the input tuple
 */
template<class Tuple>
constexpr decltype(auto)
make_tuple_from_tuple(Tuple&& t)
{
    return make_tuple_from_tuple_impl(
        std::forward<Tuple>(t),
        std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

// -- Manually-managed state updates ------------------------------------------

/// Apply a rule synchronously to manually updated states
/** This creates a cache for new states whose contents are moved into the
 *  respective state containers after all rules have been applied.
 *
 *  \tparam mode Update mode for this rule. This is the overload for
 *               synchronous updates (Update::sync).
 *  \tparam Rule The type of the rule function (object).
 *  \tparam ContTarget The type of entity container.
 *  \tparam ContArgs The types of argument containers.
 *
 *  \param rule The function (object) to apply to the entities
 *  \param cont_target The container of entities the function will be applied to
 *  \param cont_args The containers of additional argument for the function.
 */
template<Update mode,
         class Rule,
         class ContTarget,
         class... ContArgs,
         typename std::enable_if_t<mode == Update::sync, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<ContTarget>::mode
                        == Update::manual, int> = 0>
void apply_rule(Rule&& rule,
                const ContTarget& cont_target,
                ContArgs&&... cont_args)
{
    using State = typename impl::entity_t<ContTarget>::State;

    // Make sure to call `invoke_result_type` at least for a conversion check
    static_assert(not is_void_rule<State, Rule, ContTarget, ContArgs...>(),
                  "Cannot apply void rules in a synchronous update!");

    // initialize the state cache
    std::vector<State> state_cache;
    state_cache.reserve(cont_target.size());

    // create the input zip iterators
    auto range = Itertools::zip(cont_target, cont_args...);
    using std::begin, std::end;

    // Apply the rule
    // NOTE: Capute by reference is fine because rule is a temporary
    std::transform(begin(range), end(range),
                   back_inserter(state_cache),
                   [&rule](auto&& args) {
                       return std::apply(rule,
                                         std::forward<decltype(args)>(args));
                   });

    // move the cache
    for (size_t i = 0; i < cont_target.size(); ++i) {
        cont_target[i]->state = std::move(state_cache[i]);
    }
}

/// Apply a rule asynchronously to manually updated states
/** Directly overwrite the states of the passed entities, according to
 *  the storage order inside the container.
 *
 *  \tparam mode Update mode for this rule. This is the overload for
 *               asynchronous updates (Update::async).
 *  \tparam shuffle Switch for enabling the shuffling of cells before applying
 *                  the rule. This is the overload with shuffling disabled
 *                  (Shuffle::off).
 *  \tparam Rule The type of the rule function (object).
 *  \tparam Container The type of entity container.
 *  \tparam ContArgs The types of argument containers.
 *
 *  \param rule The function (object) to apply to the entities
 *  \param container The container of entities the function will be applied to
 *  \param cont_args The containers of additional argument for the function.
 */
template<Update mode,
         Shuffle shuffle = Shuffle::on,
         class Rule,
         class ContTarget,
         class... ContArgs,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<ContTarget>::mode
                        == Update::manual, int> = 0,
         typename std::enable_if_t<shuffle == Shuffle::off, int> = 0>
void
apply_rule(Rule&& rule,
           const ContTarget& cont_target,
           ContArgs&&... cont_args)
{
    // Create the input range zip iterators
    auto range = Itertools::zip(cont_target, cont_args...);
    using std::begin, std::end;

    // Apply the rule, distinguishing by return type of the rule
    using State = typename impl::entity_t<ContTarget>::State;
    if constexpr (is_void_rule<State, Rule, ContTarget, ContArgs...>()) {
        // Is a void-rule; no need to set the return value
        std::for_each(begin(range), end(range), [&rule](auto&& args) {
            std::apply(rule,
                       std::forward<decltype(args)>(args));
        });
    }
    else {
        std::for_each(begin(range), end(range), [&rule](auto&& args) {
            auto& cell = std::get<0>(args);
            cell->state = std::apply(rule,
                                    std::forward<decltype(args)>(args));
        });
    }
}

/// Apply a rule asynchronously and shuffled to manually updated states
/** Copy the container of (pointers to) entities, shuffle it, and apply the
 *  rule sequentially to the shuffled container. The original container
 *  remains unchanged.
 *
 *  \tparam mode Update mode for this rule. This is the overload for
 *               asynchronous updates (Update::async).
 *  \tparam shuffle Switch for enabling the shuffling of containers before
 *                  applying the rule. This is the overload with shuffling
 *                  enabled (Shuffle::on).
 *  \tparam Rule The type of the rule function (object).
 *  \tparam Container The type of entity container.
 *  \tparam RNG The type of the random number generator.
 *  \tparam ContArgs The types of argument containers.
 *
 *  \param rule The function (object) to apply to the entities
 *  \param cont_target The container of entities the function will be applied to
 *  \param rng The random number generator used for shuffling the entities
 *  \param cont_args The containers of additional argument for the function.
 *
 *  \note Shuffling only changes the order of execution of this function!
 *        It occurs on `cont_target` and all `cont_args` *simultaneously*,
 *        guaranteeing that entity at index `i` will be applied with additional
 *        arguments at index `i` for all container indices `i`. If you want to
 *        shuffle argument containers or entities independently from each other,
 *        you must do so yourself before applying this function.
 *        shuffle the arguments against the entities they are applied on, do
 *        so yourself *before* calling this function!
 */
template<Update mode,
         Shuffle shuffle = Shuffle::on,
         class Rule,
         class ContTarget,
         class RNG,
         class... ContArgs,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<ContTarget>::mode
                        == Update::manual, int> = 0,
         typename std::enable_if_t<shuffle == Shuffle::on, int> = 0>
void
apply_rule(Rule&& rule,
           const ContTarget& cont_target,
           RNG&& rng,
           ContArgs&&... cont_args)
{
    // Create the input range zip iterators
    auto range = Itertools::zip(cont_target, cont_args...);
    using std::begin, std::end;

    // NOTE: std::shuffle requires the container elements to be swappable.
    //       This is not the case for tuples of T& so we need a container of
    //       tuples of std::reference_wrapper<T>. For this to work, we must
    //       propagate the const-ness of the container to the wrapper.
    //       When applying the rule, we must convert the reference wrappers back
    //       to regular references, which can be done with std::make_tuple.
    using Tuple = std::tuple<
        // 'ContTarget' is always const
        std::reference_wrapper<
            const typename std::remove_reference_t<ContTarget>::value_type>,
        // Universal references to 'ContArgs' *can* be const
        std::conditional_t<
            // Check if container is const
            std::is_const_v<std::remove_reference_t<ContArgs>>,
            // If true:
            std::reference_wrapper<
                const typename std::remove_reference_t<ContArgs>::value_type>,
            // If false:
            std::reference_wrapper<
                typename std::remove_reference_t<ContArgs>::value_type>>...>;

    // Create a container of tuples of arguments and shuffle it
    std::vector<Tuple> args_container(begin(range), end(range));
    std::shuffle(begin(args_container),
                 end(args_container),
                 std::forward<RNG>(rng));

    // Apply the rule, distinguishing by return type of the rule
    using State = typename impl::entity_t<ContTarget>::State;
    if constexpr(is_void_rule<State, Rule, ContTarget, ContArgs...>())
    {
        // Is a void-rule; no need to set the return value
        std::for_each(
            begin(args_container), end(args_container), [&rule](auto&& args) {
                // NOTE: 'args' is tuple of reference_wrappers, and we want a
                //       tuple of regular references.
                std::apply(
                    rule,
                    make_tuple_from_tuple(std::forward<decltype(args)>(args)));
        });
    }
    else
    {
        std::for_each(
            begin(args_container), end(args_container), [&rule](auto&& args) {
                // NOTE: 'args' is tuple of reference_wrappers, and we want a
                //       tuple of regular references.
                auto tpl = make_tuple_from_tuple(
                    std::forward<decltype(args)>(args));
                auto& entity = std::get<0>(tpl);
                // NOTE: Do not move 'tpl' as this invalidates ref 'entity'
                entity->state = std::apply(rule, tpl);
        });
    }
}


// -- Synchronous state updates -----------------------------------------------
/// Apply a rule synchronously on the state of all entities of a container
/** Applies the rule function to each of the entities' states and
 *  stores the result in a buffer. Afterwards, it iterates over all
 *  entities again and applies the buffer to the actual state.
 *
 *  \param rule       An application rule, see \ref rule
 *  \param Container  A container with the entities upon whom rule is applied
 *
 *  \warning Using this overload is discouraged because the type of update
 *           is determined by the StateContainer. See \ref Rules for details.
 */
template<
    class Rule,
    class Container,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<sync, void>
    apply_rule(const Rule& rule, const Container& container)
{
    // Apply the rule
    std::for_each(
        std::begin(container), std::end(container),
        [&rule](const auto& entity){ entity->state_new() = rule(entity); }
    );
    
    // Update the state, moving it from the buffer state to the actual state
    std::for_each(
        std::begin(container), std::end(container),
        [](const auto& entity){ entity->update(); }
    );
}


// -- Asynchronous state updates ----------------------------------------------
/// Apply a rule on asynchronous states without prior shuffling
/** \param rule       An application rule, see \ref rule
 *  \param Container  A container with the entities upon whom rule is applied
 *
 *  \warning Using this overload is discouraged because the type of update
 *           is determined by the StateContainer. See \ref Rules for details.
 */
template<
    bool shuffle=true,
    class Rule,
    class Container,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<not sync && not shuffle, void>
    apply_rule(const Rule& rule, const Container& container)
{
    // Apply the rule, distinguishing by return type of the rule
    using ReturnType =
        std::invoke_result_t<Rule, typename Container::value_type>;

    if constexpr(std::is_same_v<ReturnType, void>)
    {
        std::for_each(
            std::begin(container), std::end(container),
            [&rule](const auto& entity){ rule(entity); }
        );
    }
    else
    {
        std::for_each(
            std::begin(container), std::end(container),
            [&rule](const auto& entity){ entity->state() = rule(entity); }
        );
    }
}


/// Apply a rule on asynchronous states with prior shuffling
/** \param rule       An application rule, see \ref rule
 *  \param Container  A container with the entities upon whom rule is applied
 *
 *  \warning Using this overload is discouraged because the type of update
 *           is determined by the StateContainer. See \ref Rules for details.
 */
template<
    bool shuffle=true,
    class Rule,
    class Container,
    class RNG,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<not sync && shuffle, void>
    apply_rule(const Rule& rule, const Container& container, RNG&& rng)
{
    std::remove_const_t<Container> container_shuffled(container);
    std::shuffle(std::begin(container_shuffled),
                 std::end(container_shuffled),
                 std::forward<RNG>(rng));

    // Apply the rule, distinguishing by return type of the rule
    using ReturnType =
        std::invoke_result_t<Rule, typename Container::value_type>;

    if constexpr(std::is_same_v<ReturnType, void>)
    {
        std::for_each(
            std::begin(container_shuffled), std::end(container_shuffled),
            [&rule](const auto& entity){ rule(entity); }
        );
    }
    else
    {
        std::for_each(
            std::begin(container_shuffled), std::end(container_shuffled),
            [&rule](const auto& entity){ entity->state() = rule(entity); }
        );
    }
}

/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_APPLY_HH    
