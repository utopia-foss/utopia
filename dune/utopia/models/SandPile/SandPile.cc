#include <iostream>

#include "SandPile.hh"

using namespace Utopia::Models::SandPile;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv) {
    // Compile time parameters that specify the cells and the grid
    constexpr bool periodic = false;
    constexpr unsigned short dim = 2;
    constexpr bool structured = true;
    constexpr bool sync = false;

    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it.
        // SandPileModel takes as arguments the model name, the parent model
        // and the manager of the grid.
        SandPileModel model("SandPile", pp,
            create_grid_manager_cells<State, periodic, dim,
                                      structured, sync>("SandPile", pp)
        );
        model.run();

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
