#ifndef VEGETATION_SETUP_HH
#define VEGETATION_SETUP_HH

namespace Utopia {

namespace Setup {

decltype(auto) vegetation(const unsigned int grid_size)
{
    constexpr bool sync = false;
    using State = double;
    using Tag = Utopia::DefaultTag;

    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, 0.0);
    auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    std::normal_distribution<> d{5,2};
    std::tuple<std::normal_distribution<>, double> bc = std::make_tuple(d, 0.5);
    return Utopia::Vegetation(manager, bc);
}

} // Setup

} // Utopia

#endif // VEGETATION_SETUP_HH
