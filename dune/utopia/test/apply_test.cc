#include <functional>
#include <random>
#include <dune/utopia/utopia.hh>
#include <dune/utopia/apply.hh>
#include <cassert>
#include <dune/common/exceptions.hh>
#include "grid_cells_test.hh"
#include <dune/utopia/neighborhoods.hh>

int main()
{
    //build the ingredients for a manager
    auto grid = Utopia::Setup::create_grid<2>(5);
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
    
    // build a manager - structured, periodic
    auto mc = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    
    //check cell values
    for(auto some_cell:mc.cells())
    {
    assert(some_cell->state()!=22);
    }
    
    //define a rule - neighborhoods could be considered but are ignored here 
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
       
    //check that the order of the container was not altered   
    //define a rule
    auto do_nothing = [](const auto cell, auto mc2)
    {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, mc2);
        return cell->state(); 
    };
    //alter the value in one cell
    mc2.cells()[0]->state()=0;
    
    //apply the rule in a asynchronous fashion
    Utopia::apply_rule(do_nothing, mc2.cells(), mc2);
    
    //check that the order of cells has not been altered
    //i.e the different cell is still in the same spot
    assert(mc2.cells()[0]->state()==0);
    for(int i=1;i<mc2.cells().size();i++)
    {
        assert(mc2.cells()[i]->state()==22);
    }
    
    //ascertain that the order of calls is in fact random!
    // :( this test might in fact fail for working code, because randomness is tested (but improbable)
    
    int apply_counter=0;
    int* p=&apply_counter;
    //define a rule
    auto get_bigger = [p](const auto cell, auto mc2)
    {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, mc2);
        return ((*p)++); 
    };
    
    mc2.cells()[0]->state()=22;
    Utopia::apply_rule(get_bigger, mc2.cells(), mc2);
    
    //apply_counter gets bigger with every call,
    //This order should not be visible in the cell contaier
    bool sometimes_gets_bigger=false;
    bool sometimes_gets_smaller=false;
    for(int i=1;i<mc2.cells().size();i++)
    {
        if(mc2.cells()[i]>mc2.cells()[i-1])
        {
            sometimes_gets_bigger=true;
        }
        else if(mc2.cells()[i]<mc2.cells()[i-1])
        {
            sometimes_gets_smaller=true;
        }
        //wrote the same value twice which should not happen here
        else
        {
            assert(false);
        }
    }
    assert(sometimes_gets_bigger && sometimes_gets_smaller);
    
    //assert every cell was called
    assert(apply_counter==mc2.cells().size());
    
    //test that apply_rule works for agents as well
    auto agent_grid = Utopia::Setup::create_grid<2>(5);
    auto agent_agents = Utopia::Setup::create_agents_on_grid<int, Utopia::DefaultTag, std::size_t>(agent_grid,30);
    auto ma = Utopia::Setup::create_manager_agents<true, true>(agent_grid, agent_agents);
    
    assert(ma.agents().size()==30);
    for(auto agent: ma.agents())
    {
        assert(agent->state()!=23);
    }
    //define a rule
    auto my_agent_rule = [](const auto cell, auto ma)
    {
        return 23; 
    };
    Utopia::apply_rule(my_agent_rule, ma.agents(), ma);
    for(auto agent: ma.agents())
    {
        assert(agent->state()==23);
    }
    return 0;
}