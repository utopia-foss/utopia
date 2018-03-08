#include <dune/utopia/apply.hh>
#include <cassert>
#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>
#include "grid_cells_test.hh"
#include <dune/utopia/neighborhoods.hh>
int main()
{
    auto grid = Utopia::Setup::create_grid<2>(5);
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
    using Pos = typename Utopia::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;
    // structured, periodic
    auto mc = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()!=22);
    }
    
    
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
    
    
    
    Utopia::apply_rule <true>(my_rule, mc.cells(), mc);
    
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()!=22);
    assert(some_cell->state_new()==22);
    }
    
    Utopia::apply_rule <true>(my_update,mc.cells(),mc);
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()==some_cell->state_new());
    assert(some_cell->state()==22);
    }
    
    return 0;
}