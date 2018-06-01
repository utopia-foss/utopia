#include "model_with_manager_test.hh"

/// clean up method that is performed after the tests
void cleanup(Utopia::DataIO::HDFFile& tmpfile, std::string tmpfile_path) {
    std::cout << "Cleaning up ..." << std::endl;

    // close and remove the temporary file
    tmpfile.close();
    std::cout << "  tmpfile closed" << std::endl;

    std::remove(tmpfile_path.c_str());
    std::cout << "  tmpfile removed" << std::endl;

    std::cout << "Cleanup finished." << std::endl;
}


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // -- Setup model -- //
        // get the test config file (path is relative to executable!)
        std::cout << "Loading config ..." << std::endl;
        Utopia::DataIO::Config cfg("model_with_manager_test.yml");
        std::cout << "  Loaded." << std::endl;

        // create a temporary file and get the basegroup
        std::cout << "Creating temporary output file ..." << std::endl;
        auto tmpfile_path = cfg["output_path"].as<std::string>();
        std::cout << "  output_path: " << tmpfile_path << std::endl;

        auto tmpfile = Utopia::DataIO::HDFFile(tmpfile_path, "w");
        std::cout << "  file created" << std::endl;
        auto basegroup = tmpfile.get_basegroup();
        std::cout << "  basegroup created" << std::endl;

        // initialize an RNG
        std::cout << "Creating shared RNG ..." << std::endl;
        auto seed = cfg["seed"].as<int>();
        std::cout << "  seed: " << seed << std::endl;

        auto rng = std::make_shared<std::mt19937>(seed);
        std::cout << "  RNG created" << std::endl;

        // create the manager with a certain grid size using a setup function
        std::cout << "Creating GridManager ..." << std::endl;
        auto grid_size = cfg["grid_size"].as<int>();
        std::cout << "  grid_size: " << grid_size << std::endl;
        
        auto manager = setup_manager(grid_size);
        std::cout << "  manager created" << std::endl;

        // class template argument deduction YESSSSS
        std::cout << "Initializing model ..." << std::endl;
        MngrModel model("test", cfg, basegroup, rng, // Base class constructor
                        manager);                    // MngrModel class

        
        // --- Tests begin here --- //
        std::cout << "Commencing tests ..." << std::endl;

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

        std::cout << "Tests successful. :)" << std::endl;

        // -- cleanup -- //
        cleanup(tmpfile, tmpfile_path);

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
