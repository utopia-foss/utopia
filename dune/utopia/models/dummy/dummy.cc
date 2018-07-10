#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

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
        pp.run(model);

        // -- Model iteration finished. -- //


        // The following is only for testing! (To be read by frontend)
        // TODO check whether this is still relevant
        unsigned int sleep_time = 300; // in milliseconds
        unsigned int num_sleeps = 3;

        for (unsigned int i = 0; i < num_sleeps; ++i) {
            pp.get_logger()->info("Sleep #{}", i+1);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }

        // Test messages needed to assert that all output is read by frontend:
        pp.get_logger()->info("All done.");
        pp.get_logger()->info("Really.");
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
