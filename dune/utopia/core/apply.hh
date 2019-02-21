#ifndef APPLY_HH
#define APPLY_HH

namespace Utopia {

/** \page rule The Rule Concept
 *
 * \section idea The General Idea
 * A rule is a function that computes the new state of the entity it is applied
 * to.
 *
 * \section impl Implementation
 * A rule must be implemented by the programmer as a function (object).
 * The function it represents must take a pointer to the entity as single
 * argument and may capture arbitrary objects.
 * The function's return value is the new state of the entity it is applied to.
 *
 * Currently, rules may also alter other members (i.e., tags) of the entity
 * they are applied to, and may even change states of other entities.
 */

/// Apply a rule synchronously on the state of all entities of a container
/** \detail Applies the rule function to each of the entities' states and
 *          stores the result in a buffer. Afterwards, it iterates over all
 *          entities again and applies the buffer to the actual state.
 *
 *  \param rule       An application rule, see \ref rule
 *  \param Container  A container with the entities upon whom rule is applied
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



} // namespace Utopia

#endif // APPLY_HH    
