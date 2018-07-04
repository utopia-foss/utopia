#include <dune/utopia/core/setup.hh>
#include <dune/utopia/data_io/types.hh>

#include "geomorphology.hh"

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Create PseudoParent and get a config file reference
        Utopia::PseudoParent pp(argv[1]);
        auto config = pp.get_cfg();

        // Create the manager instance
        constexpr bool sync = true;
        using State = std::array<double, 2>; //height, watercontent
        using Tag = Utopia::DefaultTag;
        State init_state = {0.0, 0.0};
        auto grid  = Utopia::Setup::create_grid(config["grid_size"].as<int>());
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, init_state);
        auto mngr  = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

        // Create the model instance
        Utopia::Models::Geomorphology model("geomorphology", pp, mngr);

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
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
