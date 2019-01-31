#include <iostream>

#include "ContDisease.hh"

using namespace Utopia::Models::ContDisease;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        if (Utopia::as_bool(pp.get_cfg()["ContDisease"]["periodic"])) {
            // Periodic grid
            ContDiseaseModel model("ContDisease", pp,
                create_grid_manager_cells<CellState, true>("ContDisease", pp)
                );
            model.run();
        }
        else {
            // Non-periodic grid
            ContDiseaseModel model("ContDisease", pp,
                create_grid_manager_cells<CellState, false>("ContDisease", pp)
                );
            model.run();
        }

        return 0;
    }
    catch (Utopia::Exception& e) {
        return Utopia::handle_exception(e);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occurred!" << std::endl;
        return 1;
    }
}
