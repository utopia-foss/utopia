#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "dummy.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Read in the config file
        const std::string config_file = argv[1];
        Utopia::DataIO::Config config(config_file);

        // Initialize the HDF file
        auto output_path = config["output_path"].as<std::string>();
        std::cout << "output_path: " << output_path << std::endl;
        auto file = Utopia::DataIO::HDFFile(output_path, "w");

        // ...and get the basegroup that this model will write into
        auto basegroup = file.get_basegroup();

        // Initialize the RNG
        auto seed = config["seed"].as<int>();
        std::cout << "seed: " << seed << std::endl;
        auto rng = std::make_shared<std::mt19937>(seed);

        // Set the initial state, then create the model instance
        std::vector<double> state(1E3, 0.0);
        Utopia::DummyModel model("dummy", state, config, basegroup, rng);

        // And iterate it for a number of steps
        auto num_steps = config["num_steps"].as<int>();
        std::cout << "num_steps: " << num_steps << std::endl;
        std::cout << "Starting iteration ..." << std::endl;

        for(int i = 0; i < num_steps; ++i) {
            model.iterate();
        }

        // Sleep (to be read by frontend)
        unsigned int sleep_time = 300; // in milliseconds
        unsigned int num_sleeps = 3;

        for (unsigned int i = 0; i < num_sleeps; ++i) {
            std::cout << "Sleep #" << (i+1) << " ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }

        std::cout << "All done." << std::endl << "Really." << std::endl;

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
