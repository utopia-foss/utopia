#include <dune/utopia/core/setup.hh>

#include "vegetation.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and config file reference
        Utopia::PseudoParent pp(argv[1]);
        auto config = pp.get_cfg();

        // Create the manager instance
        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;
        State initial_state = 0.0;
        int grid_size = config["vegetation"]["grid_size"].template as<int>();
        auto grid = Utopia::Setup::create_grid(grid_size);
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
