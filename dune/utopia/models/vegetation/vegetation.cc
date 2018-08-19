#include <dune/utopia/core/setup.hh>

#include "vegetation.hh"

using namespace Utopia::Models::Vegetation;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Vegetation model("vegetation", pp,
            create_grid_manager_cells<State, true>("vegetation", pp));

        // Just run!
        model.run();

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
