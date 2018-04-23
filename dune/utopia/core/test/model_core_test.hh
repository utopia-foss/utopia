#ifndef UTOPIA_MODEL_TEST_MODEL_CORE_HH
#define UTOPIA_MODEL_TEST_MODEL_CORE_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>

decltype(auto) setup_model_core(const unsigned int grid_size)
{
    // types for cells
    constexpr bool sync = false;
    using State = double;
    using Tag = Utopia::DefaultTag;

    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(
        grid, 0.0);
    auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    // class template argument deduction YESSSSS
    return Utopia::CoreModel(manager);
}

#endif // UTOPIA_MODEL_TEST_MODEL_CORE_HH
