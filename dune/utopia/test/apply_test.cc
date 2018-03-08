#include <dune/utopia/apply.hh>
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
    
    
    apply_rule <true>(my_rule, mc.cells(), mc);
    apply_rule <true>(my_update,mc.cells(),mc);
    
    
    return 0;
}