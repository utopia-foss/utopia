#include "AmeeMulti.hh"
#include "agentstate.hh"
#include "utils/generators.hh"
#include <iostream>
#include <thread>
using namespace Utopia::Models::AmeeMulti;
using Utopia::Setup::create_grid_manager_cells;
using namespace std::literals::chrono_literals;
int main(int argc, char** argv)
{
    try
    {
        // std::this_thread::sleep_for(30s);

        Dune::MPIHelper::instance(argc, argv);

        using RNG = Xoroshiro<>;
        using Celltraits = std::vector<double>;
        using Cellstate = Cellstate<Celltraits>;
        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent<RNG> pp(argv[1]);

        // make managers first -> this has to be wrapped in a factory function
        auto cellmanager =
            Utopia::Setup::create_grid_manager_cells<Cellstate, true, 2, true, false>(
                "AmeeMulti", pp);

        auto grid = cellmanager.grid();
        using GridType = typename decltype(grid)::element_type;
        using Cell = typename decltype(cellmanager)::Cell;

        Utopia::GridWrapper<GridType> wrapper{grid, cellmanager.extensions(),
                                              cellmanager.grid_cells()};

        // read stuff from the config
        bool construction =
            Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["construction"]);
        bool decay = Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["decay"]);

        using Trait = std::vector<double>;
        using Agentstate = Agentstate<Cell, Trait, RNG>;
        auto agents = Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate());
        auto agentmanager =
            Utopia::Setup::create_manager_agents<true, true>(wrapper, agents);

        // making model types
        using Modeltypes =
            Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager)::Agent>, Utopia::BCDummy, RNG>;

        if (construction && decay)
        {
            using AmeeMulti =
                AmeeMulti<Cell, decltype(cellmanager), decltype(agentmanager), Modeltypes, true, true>;

            // Periodic grid
            AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

            // run model
            model.run();
        }
        else if (construction && !decay)
        {
            using AmeeMulti =
                AmeeMulti<Cell, decltype(cellmanager), decltype(agentmanager), Modeltypes, true, false>;

            // Periodic grid
            AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

            // run model
            model.run();
        }
        else if (!construction && decay)
        {
            using AmeeMulti =
                AmeeMulti<Cell, decltype(cellmanager), decltype(agentmanager), Modeltypes, false, true>;

            // Periodic grid
            AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

            // run model
            model.run();
        }
        else
        {
            using AmeeMulti =
                AmeeMulti<Cell, decltype(cellmanager), decltype(agentmanager), Modeltypes, false, false>;

            // Periodic grid
            AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

            // run model
            model.run();
        }

        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
