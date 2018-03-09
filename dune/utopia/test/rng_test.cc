#include <cassert>

#include <dune/utopia/base.hh>
#include <dune/utopia/setup.hh>

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);
        auto grid = Utopia::Setup::create_grid<2>(54);
        auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
        auto agents = Utopia::Setup::create_agents_on_grid(grid, 1234);

        // Test default RNG
        auto ma1 = Utopia::Setup::create_manager_agents<false, false>(grid, agents);
        //auto mc0 = Utopia::GridManager<Utopia::Manager::Cells, false, false>(grid, cells);
        auto mc1 = Utopia::Setup::create_manager_cells<false, false>(grid, cells);
        assert((*ma1.rng())() == (*mc1.rng())());
        
        // Test custom RNG
        using RNG = std::minstd_rand; //Utopia::DefaultRNG;
        auto rng = std::make_shared<RNG>(123456789);
        auto ma2 = Utopia::Setup::create_manager_agents<false, false>(grid, agents, rng);
        auto mc2 = Utopia::Setup::create_manager_cells<false, false>(grid, cells, rng);
        assert((*ma2.rng())() != (*mc2.rng())());

        // copy shared pointer to RNG into new manager
        auto ma3 = Utopia::Setup::create_manager_agents<false, false>(grid, agents, ma2.rng());

        // create RNG 'clone' and check if we really copied
        rng = std::make_shared<RNG>(123456789);
        rng->discard(2);
        assert((*ma3.rng())() == (*rng)());

        return 0;
    }
    catch(Dune::Exception c){
        std::cerr << c << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
