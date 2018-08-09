#include <dune/utopia/core/setup.hh>

#include "vegetation.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Utopia::Models::Vegetation::Vegetation model("vegetation", pp,
            Utopia::Models::Vegetation::setup_manager("vegetation", pp));

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
