#include <iostream>

#include "Savanna.hh"

using namespace Utopia::Models::Savanna;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        if (Utopia::as_bool(pp.get_cfg()["Savanna"]["periodic"])) {
            // Periodic grid
            SavannaModel model("Savanna", pp,
                create_grid_manager_cells<State, true>("Savanna", pp)
            );
            model.run();
        }
        else {
            // Non-periodic grid
            SavannaModel model("Savanna", pp,
                create_grid_manager_cells<State, false>("Savanna", pp)
            );
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
