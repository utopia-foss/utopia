#include "apply.hh"
#include <cassert>
#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>
#include "grid_cells_test.hh"

int main()
{
    auto grid = Utopia::Setup::create_grid<2>(5);
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
    
    using Pos = typename Utopia::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;
    // structured, periodic
    auto mc = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    
    
    void apply_rule <true>(my_rule, mc.cells(), mc)
    {
        
        for_each(container.begin(),container.end(), rule);
        
        /*
        for (auto entity: container){
            entity->state_new() = rule(entity);
            entity->update();
        }
        */
    }
    
    
    return 0;
}