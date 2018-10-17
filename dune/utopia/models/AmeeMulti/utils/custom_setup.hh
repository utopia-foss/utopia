#ifndef UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH
#define UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH

#include <algorithm>
#include <dune/utopia/core/grid.hh>
#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
namespace Setup
{
/// Create a set of cells on a grid
/** The cells will only map to the grid, but not share data with it.
 *  \param grid_wrapper GridWrapper instance of the grid the cells
 *  	will be created on
 *  \param state Default state of all cells
 *  \param traits Default traits of all cells
 *  \return Container with created cells
 */
template <template <typename, bool, class, typename, typename, std::size_t> class Gridcell,
          typename State,
          bool sync,
          typename Tag = EmptyTag,
          std::size_t custom_neighborhood_count = 0,
          typename GridType>
decltype(auto) create_cells_on_grid(const GridWrapper<GridType>& grid_wrapper,
                                    const State state = State())
{
    using GridTypes = GridTypeAdaptor<GridType>;
    using Position = typename GridTypes::Position;
    using GV = typename GridTypes::GridView;
    using Mapper = typename GridTypes::Mapper;
    using Index = typename GridTypes::Index;

    using CellType = Gridcell<State, sync, Tag, Position, Index, custom_neighborhood_count>;

    auto grid = grid_wrapper._grid;
    GV gv(*grid);
    Mapper mapper(gv, Dune::mcmgElementLayout());
    CellContainer<CellType> cells;
    cells.reserve(mapper.size());

    // loop over all entities and create cells
    for (const auto& e : elements(gv))
    {
        const Position pos = e.geometry().center();
        const Index id = mapper.index(e);

        // check if entity is at boundary
        bool boundary = false;
        for (const auto& is : intersections(gv, e))
        {
            if (!is.neighbor())
            {
                boundary = true;
                break;
            }
        }

        cells.emplace_back(std::make_shared<CellType>(state, pos, boundary, id));
    }

    cells.shrink_to_fit();
    return cells;
}

/// Create a grid from a model configuration
/**\detail Using information from the configuration extracted from a
 *         parent model instance, a new grid instance is returned
 *
 * \param name          The name of the model instance; needed for access
 *                      to the correct configuration parameter
 * \param parent_model  The parent model the new model instance will
 *                      reside in
 *
 * \tparam dim          Dimensionaliy of the grid, can be 2 or 3
 * \tparam ParentModel  The parent model type
 */
template <unsigned short dim = 2, typename ParentModel>
auto create_grid_from_model(const std::string name, const ParentModel& parent_model)
{
    // Get the logger
    const auto log = parent_model.get_logger();
    log->info("Setting up grid from model instance '{}'...", name);

    // Get the configuration
    const auto cfg = parent_model.get_cfg()[name];

    // Extract grid size from config
    static_assert(dim == 2 || dim == 3,
                  "Template argument dim must be 2 or 3!");
    const auto gsize = as_array<unsigned int, dim>(cfg["grid_size"]);

    // Inform about the size
    if constexpr (dim == 2)
    {
        log->info("Creating 2-dimensional grid of size: {} x {} ...", gsize[0], gsize[1]);
    }
    else
    {
        log->info("Creating 3-dimensional grid of size: {} x {} x {} ...",
                  gsize[0], gsize[1], gsize[2]);
    }

    // Create grid of that size and return
    return Utopia::Setup::create_grid<dim>(gsize);
}

/// Grid setup function
/** \detail Sets up a GridManager with cells using the configuration info
 * supplied by a model and its model configuration.
 *
 * \param name          The name of the model instance; needed for access
 *                      to the correct configuration parameter
 * \param parent_model  The parent model the new model instance will
 *                      reside in.
 * \param initial_state The initial state of all cells
 *
 * \tparam periodic     Whether the grid should be periodic
 * \tparam dim          Dimensionaliy of the grid, can be 2 or 3
 * \tparam structured   Whether the grid should be structured
 * \tparam sync         Whether the cells should be synchronous or not
 * \tparam ParentModel  The parent model type
 * \tparam State        Type of the initial state
 */
template <template <typename, bool, class, typename, typename, std::size_t> class Gridcell,
          typename State,
          bool periodic = true,
          unsigned short dim = 2,
          bool structured = true,
          bool sync = true,
          typename ParentModel>
auto create_grid_manager_cells(const std::string name,
                               const ParentModel& parent_model,
                               const State initial_state = State())
{
    // Get the grid, passing through arguments
    auto grid = create_grid_from_model<dim>(name, parent_model);

    // Create cells on that grid, passing the initial state
    auto cells = create_cells_on_grid<Gridcell, State, sync>(grid, initial_state);

    // Create the grid manager, passing the template argument
    parent_model.get_logger()->info(
        "Initializing GridManager with {} "
        "boundary conditions ...",
        (periodic ? "periodic" : "fixed"));
    // TODO add the other template arguments to the log message

    auto manager = Utopia::Setup::create_manager_cells<structured, periodic>(grid, cells);

    for (auto& cell : cells)
    {
        cell->neighborhood() = Neighborhoods::MooreNeighbor::neighbors(cell, manager);
    }

    return manager;
}

} // namespace Setup
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif