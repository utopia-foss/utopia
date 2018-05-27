#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/models/geomorphology/geomorphology.hh>
#include <cassert>

int main(int argc, char *argv[])
{
    Dune::MPIHelper::instance(argc, argv);
    
    constexpr bool sync = true;
    using State = std::array<double, 2>; //height, watercontent
    using Tag = Utopia::DefaultTag;
    State initial_state = {0.0, 0.0};

    auto grid  = Utopia::Setup::create_grid(100);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
    auto mngr  = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    //std::normal_distribution<> rain{1.5,1};

    std::string config_filepath = "../../../../../dune/utopia/models/geomorphology/geomorphology_cfg.yml";
    Utopia::DataIO::Config config(config_filepath);

    Utopia::GeomorphologyModel geomorphology(mngr, config);

    assert(geomorphology.data().size() == 100*100);

    for (int i = 0; i < 200; ++i)
         geomorphology.perform_step();
    return 0;
    
}
