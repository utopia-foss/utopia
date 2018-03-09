#ifndef VEGETATION_SETUP_HH
#define VEGETATION_SETUP_HH

namespace Utopia {

namespace Setup {

decltype(auto) vegetation(const unsigned int grid_size)
{
    constexpr bool sync = true;
    using State = double;
    using Tag = Utopia::DefaultTag;

    State initial_state = 3.0;
    double birth_rate = 0.1;
    std::normal_distribution<> rain{10,2};
    std::tuple<std::normal_distribution<>, double> bc = std::make_tuple(rain, birth_rate);

    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
    auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    return Utopia::Vegetation(manager, bc);
}

} // Setup

} // Utopia

#endif // VEGETATION_SETUP_HH
