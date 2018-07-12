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
        int grid_size = Utopia::as_<int>(config["vegetation"]["grid_size"]);
        auto grid = Utopia::Setup::create_grid(grid_size);
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
        auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

        // Set the initial state, then create the model instance
        Utopia::Models::Vegetation::Vegetation model("vegetation", pp, manager);

        // Just run!
        model.run();

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
