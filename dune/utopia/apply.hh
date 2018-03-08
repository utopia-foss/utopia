#ifndef APPLY_HH
#define APPLY_HH

namespace Utopia {
    
    /*
    auto my_rule = [&mc](const auto cell)
    {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, mc);
        cell->state_new()=22;
        return 22; 
    };
    
    auto my_update = [&mc](const auto cell)
    {
        cell->update();
        return 1;
    };
    */
    //Signatur
    template<bool sync=true, class Rule, class Container, class Manager>
    void apply_rule (Rule rule, Container& container, Manager& manager)
    {
        
        for_each(container.begin(),container.end(), rule);
    }
     /*
        for (auto entity: container){
            entity->state_new() = rule(entity);
            entity->update();
        }
        */
    /*
    // call apply rule
    apply_rule(sync, my_rule, manager.cells(), manager);
    
    
    template<bool sync=false, class Rule, class Container, class Manager>
    void apply_rule (Rule rule, Container& container, Manager& manager)
    {
        std::shuffle(container)
        for (auto entity: container){
            entity->state() = rule(entity);
        }
            
    }
    
    
    //Sim
    void apply_rules_cells ()
    {
        for(const auto& f : _rules){
            for(auto&& cell : _manager.cells())
                cell->state_new() = f(cell);
            if(_update_always)
                update_cells();
        }
    }
    */
    
}
#endif // APPLY_HH    