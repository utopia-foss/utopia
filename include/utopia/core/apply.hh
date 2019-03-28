#ifndef UTOPIA_CORE_APPLY_HH
#define UTOPIA_CORE_APPLY_HH

#include <type_traits>
#include <vector>

#include "state.hh"

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

/// Apply a rule synchronously to manually updated states
/** This creates a cache for new states whose contents are moved into the
 *  respective state containers after all rules have been applied.
 *
 *  \tparam mode Update mode for this rule. This is the overload for
 *               synchronous updates (Update::sync).
 *  \tparam Rule The type of the rule function (object).
 *  \tparam Container The type of entity container.
 *
 *  \param rule The function (object) to apply to the entities
 *  \param container The container of entities the function will be applied to
 */
template<Update mode,
         class Rule,
         class Container,
         typename std::enable_if_t<mode == Update::sync, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<Container>::mode
                        == Update::manual, int> = 0>
void apply_rule (Rule&& rule, const Container& container)
{
    // initialize the state cache
    using State = typename impl::entity_t<Container>::State;
    std::vector<State> state_cache;
    state_cache.reserve(container.size());

    // apply the rule
    std::transform(begin(container), end(container),
                   back_inserter(state_cache),
                   std::forward<Rule>(rule));

    // move the cache
    for (size_t i = 0; i < container.size(); ++i) {
        container[i]->state = std::move(state_cache[i]);
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
 *  \tparam RNG The type of the random number generator (unused in this 
 *              overload).
 *
 *  \param rule The function (object) to apply to the entities
 *  \param container The container of entities the function will be applied to
 */
template<Update mode,
         Shuffle shuffle = Shuffle::on,
         class Rule,
         class Container,
         class RNG,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<Container>::mode
                        == Update::manual, int> = 0>
std::enable_if_t<shuffle == Shuffle::off, void>
    apply_rule (Rule&& rule, const Container& container, RNG&&)
{
    // apply the rule
    std::for_each(begin(container), end(container),
                  [&rule](const auto& cell)
                  { cell->state = rule(cell); });
}

/// Apply a rule asynchronously and shuffled to manually updated states
/** Copy the container of (pointers to) entities, shuffle it, and apply the
 *  rule sequentially to the shuffled container. The original container
 *  remains unchanged.
 *
 *  \tparam mode Update mode for this rule. This is the overload for
 *               asynchronous updates (Update::async).
 *  \tparam shuffle Switch for enabling the shuffling of cells before applying
 *                  the rule. This is the overload with shuffling enabled
 *                  (Shuffle::on).
 *  \tparam Rule The type of the rule function (object).
 *  \tparam Container The type of entity container.
 *  \tparam RNG The type of the random number generator.
 *
 *  \param rule The function (object) to apply to the entities
 *  \param container The container of entities the function will be applied to
 *  \param rng The random number generator used for shuffling the entities
 */
template<Update mode,
         Shuffle shuffle = Shuffle::on,
         class Rule,
         class Container,
         class RNG,
         typename std::enable_if_t<mode == Update::async, int> = 0,
         typename std::enable_if_t<
                    impl::entity_t<Container>::mode
                        == Update::manual, int> = 0>
std::enable_if_t<shuffle == Shuffle::on, void>
    apply_rule (Rule&& rule, const Container& container, RNG&& rng)
{
    // copy the original container and shuffle it
    std::remove_const_t<Container> container_shuffled(container);
    std::shuffle(begin(container_shuffled),
                 end(container_shuffled),
                 std::forward<RNG>(rng));

    // apply the rule
    std::for_each(begin(container_shuffled), end(container_shuffled),
                  [&rule](const auto& cell) { cell->state = rule(cell); });
}

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
    for_each(container.begin(), container.end(),
        [&rule](const auto& entity){ entity->state_new() = rule(entity); }
    );
    for_each(container.begin(), container.end(),
        [](const auto& entity){ entity->update(); }
    );
}

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
    for_each(container.begin(), container.end(),
        [&rule](const auto& entity){ entity->state() = rule(entity); }
    );
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
    std::remove_const_t<Container> copy_container(container);
    std::shuffle(copy_container.begin(), copy_container.end(),
        std::forward<RNG>(rng));
    for_each(copy_container.begin(), copy_container.end(),
        [&rule](const auto& entity){ entity->state() = rule(entity); }
    );
}

/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_APPLY_HH    
