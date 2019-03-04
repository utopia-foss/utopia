#include <dune/utopia/data_io/hdffile.hh>

#include "model_with_manager_test.hh"

using namespace Utopia;

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // -- Setup model -- //
        // create a pseudo parent
        std::cout << "Initializing pseudo parent ..." << std::endl;

        Utopia::PseudoParent pp("model_with_manager_test.yml");

        // extract the config
        // FIXME adjust setup to make this unnecessary
        auto cfg = pp.get_cfg();

        // create the manager with a certain grid size using a setup function
        std::cout << "Creating GridManager ..." << std::endl;
        auto grid_size = get_as<int>("grid_size", cfg);
        std::cout << "  grid_size: " << grid_size << std::endl;
        
        auto manager = setup_manager(grid_size);
        std::cout << "  manager created" << std::endl;

        // class template argument deduction YESSSSS
        std::cout << "Initializing model ..." << std::endl;
        MngrModel model("test", pp, manager);

        
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

        // Cleanup
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());

        std::cout << "Temporary files removed." << std::endl;
        
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
