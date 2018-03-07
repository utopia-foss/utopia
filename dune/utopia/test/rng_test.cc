#include <random>
#include <iostream>
#include <cassert>

#include <dune/utopia/utopia.hh>

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
        assert(ma1.get_random() == mc1.get_random());
        
        // Test custom RNG
        using RNG = std::minstd_rand; //Utopia::DefaultRNG;
        std::shared_ptr<RNG> rng = std::make_shared<RNG>(123456789);
        auto ma2 = Utopia::Setup::create_manager_agents<false, false>(grid, agents, rng);
        auto mc2 = Utopia::Setup::create_manager_cells<false, false>(grid, cells, rng);
        assert(ma2.get_random() != mc2.get_random());

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
