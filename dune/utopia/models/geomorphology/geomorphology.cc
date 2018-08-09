#include <dune/utopia/core/setup.hh>

#include "geomorphology.hh"

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and get a config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Utopia::Models::Geomorphology::Geomorphology model("geomorphology", pp,
            Utopia::Models::Geomorphology::setup_manager("geomorphology", pp));


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
