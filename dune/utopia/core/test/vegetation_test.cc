#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/models/vegetation/vegetation.hh>


int main(int argc, char *argv[])
{
    Dune::MPIHelper::instance(argc, argv);

    std::cout << "Creating Config\n";
        
    std::string config_filepath = "../../../../../dune/utopia/models/vegetation/vegetation_cfg.yml";
    Utopia::DataIO::Config config(config_filepath);
        
    std::cout << "Creating manager\n";
    constexpr bool sync = true;
    using State = double;
    using Tag = Utopia::DefaultTag;
    State initial_state = 0;
    int grid_size = 4;
    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
    auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    std::cout << "Creating model\n";
    Utopia::VegetationModel model(config);

    for (int i = 0; i < 500; ++i)
         model.perform_step();
    return 0;
}
