#include <dune/utopia/core/setup.hh>

#include "geomorphology.hh"

using namespace Utopia::Models::Geomorphology;
using Utopia::Setup::create_grid_manager_cells;

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and get a config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Geomorphology model("geomorphology", pp,
            // periodic=false, structured=sync=true
            create_grid_manager_cells<State, false>("geomorphology", pp));

        // Just run!
        model.run();

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
