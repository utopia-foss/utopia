#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "dummy.hh"

// Declare the model
using DummyModel = Utopia::Models::Dummy;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Read in the config file
        const std::string config_file = argv[1];

        // create PseudoParent, setting up the HDFFile and RNG
        Utopia::PseudoParent pp(config_file);

        // get the config from the PseudoParent and the logger
        auto config = pp.get_cfg();

        // Set the initial state, then create the model instance
        std::vector<double> state(1E3, 0.0);
        DummyModel model("dummy", pp, state);

        // And iterate it for a number of steps
        auto num_steps = config["num_steps"].as<int>();
        pp.get_logger()->info("num_steps: {}", num_steps);
        pp.get_logger()->info("Starting model iteration ...");

        // TODO use future pseudo parent features for iteration
        for(int i = 0; i < num_steps; ++i) {
            model.iterate();
        }

        // Sleep (to be read by frontend)
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
