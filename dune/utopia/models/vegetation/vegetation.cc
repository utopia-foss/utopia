#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <dune/utopia/core/setup.hh>
#include <dune/utopia/data_io/types.hh>

#include "vegetation.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Read in the config file
        const std::string config_file = argv[1];

        // Create PseudoParent
        Utopia::PseudoParent pp(config_file);
        auto config = pp.get_cfg();

        // Create the manager instance
        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;
        State initial_state = 0.0;
        auto grid = Utopia::Setup::create_grid(config["grid_size"].template as<int>());
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
        auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

        // Set the initial state, then create the model instance
        Utopia::Models::Vegetation model("vegetation", pp, manager);

        // And iterate it for a number of steps
        auto num_steps = config["num_steps"].as<int>();
        std::cout << "num_steps: " << num_steps << std::endl;
        std::cout << "Starting iteration ..." << std::endl;

        for(int i = 0; i < num_steps; ++i) {
            model.iterate();
        }

        // Test set_boundary_condition
        /*std::cout << "Testing set_boundary_condition ..." << std::endl;
        std::normal_distribution<> rain{10.1,1.2};
        auto bc = std::make_tuple(rain, 0.2, 0.2);
        model.set_boundary_condition(bc);

        // Test data
        std::cout << "Testing get data ..." << std::endl;
        auto d = model.data();

        // Test set_initial_condition
        std::cout << "Testing set_initial_condition ..." << std::endl;
        model.set_initial_condition(d);*/
        

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
