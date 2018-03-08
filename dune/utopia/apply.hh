#ifndef APPLY_HH
#define APPLY_HH

namespace Utopia {
    
    template<bool sync, class Rule, class Container, class Manager>
    void apply_rule(Rule rule, Container& container, Manager& manager);
    
    ///synchronous update rule
    template<class Rule, class Container, class Manager>
    void apply_rule<true> (Rule rule, Container& container, Manager& manager)
    {
        //reduce the number of parameters in rule 
        //by binding parameter 2 to manager
        auto bind_rule=std::bind(rule, std::placeholders::_1, manager);
        
        for_each(container.begin(),container.end(),
            [&bind_rule](const auto cell){ cell->state_new() = bind_rule(cell); });
        for_each(container.begin(),container.end(),
            [](const auto cell){ cell->update(); });         
    }  
    
    ///asynchronous update rule
    template<class Rule, class Container, class Manager>
    void apply_rule<false> (Rule rule, Container& container, Manager& manager)
    {
        //reduce the number of parameters in rule 
        //by binding parameter 2 to manager
        auto bind_rule=std::bind(rule, std::placeholders::_1, manager);
        
        std::random_shuffle(container.begin(),container.end());
        for_each(container.begin(),container.end(),
            [&bind_rule](const auto cell){ cell->state_new() = bind_rule(cell); });
               
    }     
}
#endif // APPLY_HH    