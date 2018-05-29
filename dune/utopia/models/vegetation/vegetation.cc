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

        const std::string config_file = argv[1];
        Utopia::DataIO::Config config(config_file);

        
        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;
        int grid_size = 10;
        State initial_state = 3.0;
        auto grid = Utopia::Setup::create_grid(grid_size);
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
        auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

        Utopia::VegetationModel model(manager, config["vegetation"]);

        for(int i = 0; i < config["num_steps"].as<int>(); ++i)
            model.iterate();

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
