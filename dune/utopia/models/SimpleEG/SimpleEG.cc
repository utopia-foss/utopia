#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "SimpleEG.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Load the config file
        const std::string cfg_path = argv[1];
        Utopia::DataIO::Config config(cfg_path);

        // Initialize the HDF file
        auto output_path = config["SimpleEG"]["output_path"].as<std::string>();
        auto file = Utopia::DataIO::HDFFile(output_path, "w");
        // TODO the output_path should be at the top level, not under SimpleEG

        // ...and get the basegroup that this model will write into
        auto basegroup = file.get_basegroup();

        // Initialize the RNG
        auto seed = config["SimpleEG"]["seed"].as<int>();
        auto rng = std::make_shared<std::mt19937>(seed);
        // TODO the seed should actually be taken from the top-level

        // Initialize the main model instance
        Utopia::SimpleEGModel model("SimpleEG", config, basegroup, rng);
        // NOTE This already implements the new model interface

        // Perform the iteration
        for(int i = 0; i < config["num_steps"].as<int>(); ++i)
            model.iterate();

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
