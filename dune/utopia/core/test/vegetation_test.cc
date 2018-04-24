#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/models/vegetation/vegetation.hh>


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        double growth_rate = 0.1;
        double seeding_rate = 0.2;
        std::normal_distribution<> rain{10,2};
        std::tuple<std::normal_distribution<>, double, double> bc = std::make_tuple(rain, growth_rate, seeding_rate);
        
        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;
        State initial_state = 3.0;
        int grid_size = 10;
        auto grid = Utopia::Setup::create_grid(grid_size);
        auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
        auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

        Utopia::VegetationModel model(manager, bc);

        model.iterate();

        return 0;
    }
    catch(...){
        std::cerr << "Exception thrown!" << std::endl;
        return 1;
    }
}
