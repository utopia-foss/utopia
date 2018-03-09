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
 * The function it represents must take two arguments: A single entity
 * and a GridManager. The function's return value is the new state of the
 * entity it is applied to.
 *
 * Currently, rules may also alter other members (i.e., tags) of the entity
 * they are applied to, and may even change states of other entities.
 */

/// synchronous application of rules
/**\param rule An application rule, see \ref rule
 * \param Container A container with the entities upon whom rule is applied
 * \param Manager holds the entities to be modified
 */
template<
    class Rule,
    class Container,
    class Manager,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<sync, void>
apply_rule(Rule rule, const Container& container, Manager& manager)
{
    //reduce the number of parameters in rule
    //by binding parameter 2 to manager
    auto bind_rule = std::bind(rule, std::placeholders::_1, manager);
    
    for_each(container.begin(), container.end(),
        [&bind_rule](const auto cell){ cell->state_new() = bind_rule(cell); }
    );
    for_each(container.begin(), container.end(),
        [](const auto cell){ cell->update(); }
    );
}

/// synchronous application of rules with a reference to the manager
template<
    class Rule,
    class Container,
    class Manager,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<sync, void>
apply_rule_ref(Rule rule, const Container& container, Manager& manager)
{
    //reduce the number of parameters in rule
    //by binding parameter 2 to manager
    // auto bind_rule = std::bind(rule, std::placeholders::_1, manager);
    
    for_each(container.begin(), container.end(),
        [&rule, &manager](const auto cell){ cell->state_new() = rule(cell, manager); }
    );
    for_each(container.begin(), container.end(),
        [](const auto cell){ cell->update(); }
    );
}


/// aynchronous application of rules
/**\param rule Function that takes an entity and manager and returns a new state
 *      See \ref rule
 * \param Container A container with the entities upon whom rule is applied
 * \param Manager holds the entities to be modified
 */
template<
    bool shuffle=true,
    class Rule,
    class Container,
    class Manager,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<!sync, void>
apply_rule(Rule rule, const Container& container, Manager& manager)
{
    //reduce the number of parameters in rule
    //by binding parameter 2 to manager
    auto bind_rule=std::bind(rule, std::placeholders::_1, manager);

    if constexpr (shuffle) {
        std::remove_const_t<Container> copy_container(container);
        std::shuffle(copy_container.begin(), copy_container.end(),
            *manager.rng());
        for_each(copy_container.begin(), copy_container.end(),
            [&bind_rule](const auto cell){ cell->state() = bind_rule(cell); }
        );
    }
    else {
        for_each(container.begin(), container.end(),
            [&bind_rule](const auto cell){ cell->state() = bind_rule(cell); }
        );
    }
}

} // namespace Utopia

#endif // APPLY_HH    