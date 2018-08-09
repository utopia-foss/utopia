#include <iostream>

#include "CopyMeBare.hh"

using namespace Utopia::Models::CopyMeBare;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        if (Utopia::as_bool(pp.get_cfg()["CopyMeBare"]["periodic"])) {
            // Periodic grid
            CopyMeBareModel model("CopyMe", pp,
                                  setup_manager<true>("CopyMeBare", pp));
            model.run();
        }
        else {
            // Non-periodic grid
            CopyMeBareModel model("CopyMe", pp,
                                  setup_manager<false>("CopyMeBare", pp));
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
