#include <iostream>       // std::cout, std::endl

#include "PredatorPrey.hh"

using namespace Utopia::Models::PredatorPrey;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);


        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        if (Utopia::as_bool(pp.get_cfg()["PredatorPrey"]["periodic"])) {
            // Periodic grid
            PredatorPreyModel model("PredatorPrey", pp,
                                create_grid_manager_cells<State, true, 2, true, false>("PredatorPrey", pp));

            model.run();

        }
        else {
            // Non-periodic grid
            PredatorPreyModel model("PredatorPrey", pp,
                                create_grid_manager_cells<State, false, 2, true, false>("PredatorPrey", pp));

            model.run();
        }

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
