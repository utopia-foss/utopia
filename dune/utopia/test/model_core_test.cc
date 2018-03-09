#include "model_core_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        auto model_core = setup_model_core(100);

        // check initial condition
        const auto& cells = model_core.data();
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->state() == 0.0; })
        );
        assert(none_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->is_tagged; })
        );

        // check application
        model_core.perform_step();
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
        model_core.set_initial_condition(init);
        init.clear();
        assert(all_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->state() == 0.0; })
        );
        assert(none_of(cells.begin(), cells.end(),
            [](const auto cell){ return cell->is_tagged; })
        );

        return 0;
    }
    catch(...){
        std::cerr << "Exception thrown!" << std::endl;
        return 1;
    }
}