#ifndef APPLY_HH
#define APPLY_HH

namespace Utopia {

/// synchronous application of rules
/**\param rule Function that takes an entity and manager and returns a new state
 * \param Container A container with the entities upon whom rule is applied
 * \param Manager holds the entities to be modified
 */
template<
    class Rule,
    class Container,
    class Manager,
    bool sync=impl::entity_t<Container>::is_sync()>
std::enable_if_t<sync, void>
apply_rule(Rule rule, Container& container, Manager& manager)
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


/// aynchronous application of rules
/**\param rule Function that takes an entity and manager and returns a new state
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
apply_rule(Rule rule, Container& container, Manager& manager)
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