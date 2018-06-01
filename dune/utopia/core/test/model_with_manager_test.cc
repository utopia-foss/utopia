#include "model_with_manager_test.hh"

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // get the test config file
        Utopia::DataIO::Config cfg("model_core_test.yml");

        // create a temporary file and get the basegroup
        auto output_path = cfg["output_path"].as<std::string>();
        std::cout << "output_path: " << output_path << std::endl;
        auto file = Utopia::DataIO::HDFFile(output_path);
        auto basegroup = file.get_basegroup();

        // initialize an RNG
        auto seed = cfg["seed"].as<int>();
        std::cout << "seed: " << seed << std::endl;
        auto rng = std::make_shared<std::mt19937>(seed);

        // create the manager with a certain grid size using a setup function
        auto grid_size = cfg["grid_size"].as<int>();
        auto manager = setup_core_model_manager(grid_size);

        // class template argument deduction YESSSSS
        CoreModel model("test", cfg, basegroup, rng, // Base class constructor
                        manager);                    // CoreModel class

        // --- Tests begin here --- //
        // check initial condition
        const auto& cells = model.data();
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->state() == 0.0; })
        );
        assert(none_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->is_tagged; })
        );

        // check application
        model.perform_step();
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->state() == 4.0; })
        );
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->is_tagged == cell->is_boundary(); })
        );

        // some data types
        using GridTypes = Utopia::GridTypeAdaptor<Utopia::DefaultGrid<2>>;
        using State = double;
        constexpr bool sync = false;
        using Tag = Utopia::DefaultTag;
    
        using Cell = Utopia::Cell<State, sync, Tag,
            typename GridTypes::Position, typename GridTypes::Index>;
        Utopia::CellContainer<Cell> init;

        typename GridTypes::Position pos({0.0, 0.0});
        std::generate_n(std::back_inserter(init), cells.size(),
            [&pos](){ return std::make_shared<Cell>(0.0, pos, false, 0); }
        );

        // apply initial condition
        model.set_initial_condition(init);
        init.clear();
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->state() == 0.0; })
        );
        assert(none_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->is_tagged; })
        );

        // -- cleanup -- //
        // TODO close and remove tmpfile

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Exception thrown!" << std::endl;
        return 1;
    }
}
