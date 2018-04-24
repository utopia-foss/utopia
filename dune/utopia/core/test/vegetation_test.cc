#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/models/vegetation/vegetation.hh>


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);
        constexpr bool sync = true;
        using State = double;
        using Tag = Utopia::DefaultTag;

        State initial_state = 3.0;
        double birth_rate = 0.1;
        int grid_size = 10;
        std::normal_distribution<> rain{10,2};
        std::tuple<std::normal_distribution<>, double> bc = std::make_tuple(rain, birth_rate);
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
