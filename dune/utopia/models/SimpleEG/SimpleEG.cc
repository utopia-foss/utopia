#include <iostream>

#include <dune/utopia/core/setup.hh>
#include "SimpleEG.hh"

using namespace Utopia::Models::SimpleEG;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);


        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        // TODO find a better way to check for this argument?!
        if (Utopia::as_bool(pp.get_cfg()["SimpleEG"]["periodic"])) {
            // Periodic grid
            SimpleEGModel model("SimpleEG", pp,
                                create_grid_manager_cells<true>("SimpleEG",
                                                                pp, state0));
            model.run();

        }
        else {
            // Non-periodic grid
            SimpleEGModel model("SimpleEG", pp,
                                create_grid_manager_cells<false>("SimpleEG",
                                                                 pp, state0));
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
