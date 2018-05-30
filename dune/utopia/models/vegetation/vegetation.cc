#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <dune/utopia/core/setup.hh>
#include <dune/utopia/data_io/config.hh>

#include "vegetation.hh"

int main (int argc, char** argv)
{
    try {

        Dune::MPIHelper::instance(argc, argv);

        // Load the config file
        const std::string cfg_path = argv[1];
        Utopia::DataIO::Config config(cfg_path);

        // Initialize the HDF file
        auto output_path = config["output_path"].as<std::string>();
        auto file = Utopia::DataIO::HDFFile(output_path, "w");

        // ...and get the basegroup that this model will write into
        auto basegroup = file.get_basegroup();

        // Initialize the RNG
        //auto seed = config["seed"].as<int>();
        //auto rng = std::make_shared<std::mt19937>(seed);

        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;
        State initial_state = 0.0;
        auto grid = Utopia::Setup::create_grid(config["grid_size"].as<int>());
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
        auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
        std::cout << "Created Manager.\n";

        Utopia::VegetationModel model("vegetation", 
                                      config["vegetation"],
                                      basegroup,
                                      manager);

        for(int i = 0; i < config["num_steps"].as<int>(); ++i)
            model.iterate();
        std::cout << "Finished iterating.\n";

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
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
