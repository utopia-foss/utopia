#ifndef APPLY_HH
#define APPLY_HH

namespace Utopia {
    
    ///synchronous update rule
    template<class Container,bool sync= Container::value_type::element_type::is_sync(), class Rule , class Manager>
    std::enable_if_t<sync, void>
    apply_rule(Rule rule, Container& container, Manager& manager)
    {
       
        //reduce the number of parameters in rule 
        //by binding parameter 2 to manager
        auto bind_rule=std::bind(rule, std::placeholders::_1, manager);
        
        for_each(container.begin(),container.end(),
                [&bind_rule](const auto cell){ cell->state_new() = bind_rule(cell); });
        for_each(container.begin(),container.end(),
                [](const auto cell){ cell->update(); });  
          
         
    }  
    
    template<class Container,bool sync= Container::value_type::element_type::is_sync(), class Rule , class Manager>
    std::enable_if_t<!sync, void>
    apply_rule(Rule rule, Container& container, Manager& manager)
    {
    
        //reduce the number of parameters in rule 
        //by binding parameter 2 to manager
        auto bind_rule=std::bind(rule, std::placeholders::_1, manager);
        
       // std::random_shuffle(container.begin(),container.end());
        for_each(container.begin(),container.end(),
                [&bind_rule](const auto cell){ cell->state() = bind_rule(cell); });
               
    }  
             
}
#endif // APPLY_HH    