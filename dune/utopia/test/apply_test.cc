#include <functional>
#include <dune/utopia/apply.hh>
#include <cassert>
#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>
#include "grid_cells_test.hh"
#include <dune/utopia/neighborhoods.hh>

int main()
{
    //build the ingredients for a manager
    auto grid = Utopia::Setup::create_grid<2>(5);
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
    using Pos = typename Utopia::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;
   
    // build a manager - structured, periodic
    auto mc = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    
    //check cell values
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()!=22);
    }
    
    //define a rule
    auto my_rule = [](const auto cell, auto mc)
    {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, mc);
        return 22; 
    };
   
    //apply the rule in a synchronous fashion
    Utopia::apply_rule(my_rule, mc.cells(), mc);
    
    //make sure the rule was applied correctly
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()==22);
    assert(some_cell->state_new()==22);
    }
    
    //repeat test for asynchronous application
    auto grid2 = Utopia::Setup::create_grid<2>(5);
    auto cells2 = Utopia::Setup::create_cells_on_grid<false>(grid);
    using Pos2 = typename Utopia::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;
   
    // build a manager - structured, periodic
    auto mc2 = Utopia::Setup::create_manager_cells<true, true>(grid2, cells2);
    
    //check cell values
    for(auto some_cell:mc2.cells())
    {
    assert(some_cell->state()!=22);
    }
    
    //define a rule
    auto my_rule2 = [](const auto cell, auto mc2)
    {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, mc2);
        return 22; 
    };
   
    //apply the rule in a asynchronous fashion
    Utopia::apply_rule(my_rule2, mc2.cells(), mc2);
    
    //make sure the rule was applied correctly
    for(auto some_cell:mc2.cells())
    {
    assert(some_cell->state()==22);
    }
   
    return 0;
}