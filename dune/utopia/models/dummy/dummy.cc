#include "dummy.hh"

// Declare the model
using DummyModel = Utopia::Models::Dummy::Dummy;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);


        // -- Model definition and iteration -- //

        // Create PseudoParent from config file, setting up the HDFFile and RNG
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        std::vector<double> state(1E3, 0.0);
        DummyModel model("dummy", pp, state);

        // ... now use the PseudoParent to perform the iteration
        model.run();
        // -- Model iteration finished. -- //
        

        // Test messages needed to assert that all output is read by frontend:
        pp.get_logger()->info("All done.");
        pp.get_logger()->info("Really.");
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
        std::cerr << "An unexpected exception occured!" << std::endl;
        return 1;
    }
}
