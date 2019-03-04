#ifndef SETUP_HH
#define SETUP_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/types.hh>
#include <dune/utopia/core/tags.hh>
#include <dune/utopia/core/cell.hh>
#include <dune/utopia/core/agent.hh>
#include <dune/utopia/core/grid.hh>
#include <dune/utopia/data_io/cfg_utils.hh>

namespace Utopia
{

/// Return the extensions of a grid
template<class Grid>
auto determine_extensions (const std::shared_ptr<Grid> grid)
    -> std::array<typename GridTypeAdaptor<Grid>::Coordinate,
        GridTypeAdaptor<Grid>::dim>
{
    static constexpr int dim = GridTypeAdaptor<Grid>::dim;
    using Coordinate = typename GridTypeAdaptor<Grid>::Coordinate;

    auto gv = grid->leafGridView();
    std::array<Coordinate,dim> ret;

    std::fill(ret.begin(),ret.end(),0.0);
    for(const auto& v : vertices(gv)){
        const auto pos = v.geometry().center();
        for(int i = 0; i<dim; ++i){
            ret.at(i) = std::max(pos[i],ret.at(i));
        }
    }
    return ret;
}
    
/// Convenience functions for building managed structures
namespace Setup
{
    /// Create a GridManager from a grid and a CellContainer
    /** \param wrapper GridWrapper instance holding the grid
     *  \param cells CellContainer holding the cells
     */
    template<bool structured, bool periodic, typename GridType, typename CellType>
    auto create_manager_cells (
        const GridWrapper<GridType>& wrapper,
        const CellContainer<CellType>& cells)
        -> GridManager<
            Manager::Cells, CellType, GridType, structured, periodic>
    {
        return GridManager<
            Manager::Cells, CellType, GridType, structured, periodic>(
            wrapper, cells);
    }

    /// Create a GridManager from a grid and an AgentContainer
    /** \param wrapper GridWrapper instance holding the grid
     *  \param agents AgentContainer holding the cells
     */
    template<bool structured, bool periodic, typename GridType, typename AgentType>
    auto create_manager_agents (
        const GridWrapper<GridType>& wrapper,
        const AgentContainer<AgentType>& agents)
        -> GridManager<
            Manager::Agents, AgentType, GridType, structured, periodic>
    {
        return GridManager<
            Manager::Agents, AgentType, GridType, structured, periodic>(
            wrapper, agents);
    }

    /// Create an unstructured grid from a Gmsh file
    /**
     *  \tparam dim Spatial dimension of grid and mesh
     *  \param filename Name of the Gmsh file with path relative to executable
     *  \param refinement_level Level of global refinement applied to grid
     *  \return Shared pointer to the grid
     *  \warning Do not modify the grid after building other structures from it!
     */
    template<int dim=2>
    GridWrapper<Dune::UGGrid<dim>> read_gmsh (const std::string filename, const unsigned int refinement_level=0)
    {
        using Grid = Dune::UGGrid<dim>;
        Dune::GridFactory<Grid> factory;
        Dune::GmshReader<Grid>::read(factory, filename);
        std::shared_ptr<Grid> grid(factory.createGrid());

        grid->globalRefine(refinement_level);

        // add empty array
        std::array<unsigned int,dim> cells;

        GridWrapper<Grid> gw = {grid, determine_extensions(grid), cells};
        return gw;
    }

    /// Build a rectangular grid.
    /** Cells will be rectangular/cubic. Cell edge length defaults to 1 if
     *  'range' parameter is omitted.
     *  \tparam dim Spatial dimension of the grid. Default is 2.
     *  \param cells Array of size dim containing the number of cells
     *    in each dimension
     *  \param range Array of size dim containing the grid extensions
     *    in each dimension
     *  \return Shared pointer to the grid
     *  \warning Do not modify the grid after building other structures from it!
     */
    template<int dim=2>
    GridWrapper<DefaultGrid<dim>>
        create_grid (const std::array<unsigned int,dim> cells, std::array<float,dim> range = std::array<float,dim>())
    {
        using Grid = DefaultGrid<dim>;
        using GridTypes = GridTypeAdaptor<Grid>;
        using Position = typename GridTypes::Position;

        bool automated_range = false;
        for(auto it = range.cbegin(); it != range.cend(); ++it){
            if(*it == 0.0){
                automated_range = true;
                break;
            }
        }

        if(automated_range){
            std::copy(cells.cbegin(),cells.cend(),range.begin());
        }

        Position extensions;
        std::copy(range.cbegin(),range.cend(),extensions.begin());

        // convert unsigned int to int for YaspGrid constructor
        std::array<int,dim> cells_i;
        std::copy(cells.cbegin(),cells.cend(),cells_i.begin());
        auto grid = std::make_shared<Grid>(extensions,cells_i);

        GridWrapper<Grid> gw = {grid,determine_extensions(grid),cells};
        return gw;
    }

    /// Build a rectangular grid
    /** Cells will be rectangular/cubic with edge length 1.
     *  \tparam dim Spatial dimension of the grid. Default is 2.
     *  \param cells_xyz Number of cells in each direction.
     *     Total number will be cells^dim.
     *  \return Shared pointer to the grid
     *  \warning Do not modify the grid after building other structures from it!
     */
    template<int dim=2>
    decltype(auto) create_grid(const unsigned int cells_xyz)
    {
        std::array<unsigned int,dim> cells;
        cells.fill(cells_xyz);
        return create_grid<dim>(cells);
    }

    /// Create a set of cells on a grid
    /** The cells will only map to the grid, but not share data with it.
     *  \param grid_wrapper GridWrapper instance of the grid the cells
     *  	will be created on
     *  \param state Default state of all cells
     *  \param traits Default traits of all cells
     *  \return Container with created cells
    */
    template<bool sync,
        typename State = int,
        typename Tag = EmptyTag,
        std::size_t custom_neighborhood_count = 0,
        typename GridType>
    decltype(auto) create_cells_on_grid (
        const GridWrapper<GridType>& grid_wrapper,
        const State state = State())
    {

        using GridTypes = GridTypeAdaptor<GridType>;
        using Position = typename GridTypes::Position;
        using GV = typename GridTypes::GridView;
        using Mapper = typename GridTypes::Mapper;
        using Index = typename GridTypes::Index;

        using CellType = Cell<State, sync, Tag, Position, Index>;

        auto grid = grid_wrapper._grid;
        GV gv(*grid);
        Mapper mapper(gv, Dune::mcmgElementLayout());
        CellContainer<CellType> cells;
        cells.reserve(mapper.size());

        // loop over all entities and create cells
        for(const auto& e : elements(gv))
        {
            const Position pos = e.geometry().center();
            const Index id = mapper.index(e);

            // check if entity is at boundary
            bool boundary = false;
            for(const auto& is : intersections(gv,e)){
                if(!is.neighbor()){
                    boundary = true;
                    break;
                }
            }

            cells.emplace_back(std::make_shared<CellType>
                (state,pos,boundary,id));
        }

        cells.shrink_to_fit();
        return cells;
    }

    /// Randomly distribute agents on a grid
    /**
     *  \param grid_wrapper Grid wrapper instance
     *  \param count Number of agents to create
     *  \param state_initial Initial state of all agents
     *  \param traits_initial Initial traits of all agents
     *  \return Container with created agents
     */
     template<typename State=int, class Tags=EmptyTag, typename IndexType=std::size_t, typename GridType>
     decltype(auto) create_agents_on_grid(
         const GridWrapper<GridType>& grid_wrapper,
         const std::size_t count,
         const State state_initial = 0)
     {
         // fetch some types
         using Types = GridTypeAdaptor<GridType>;
         using Position = typename Types::Position;
         using Coordinate = typename Types::Coordinate;
         using Index = IndexType;
         using Agent = Agent<State,Tags,Index,Position>;

         AgentContainer<Agent> agents;

         // set up random number generator for positions
         const auto& extensions = grid_wrapper._extensions;
         std::array<std::uniform_real_distribution<Coordinate>,Types::dim> distr;
         std::transform(extensions.begin(),extensions.end(),distr.begin(),
             [](const auto& ext){
                 return std::uniform_real_distribution<Coordinate>(0.0,ext);
         });
         DefaultRNG ran(123456);

         // create agents
         for(std::size_t i = 0; i<count; ++i)
         {
             Position pos;
             std::transform(distr.begin(),distr.end(),pos.begin(),
                 [&ran](auto& dist){
                     return dist(ran);
             });
             agents.emplace_back(std::make_shared<Agent>(state_initial,i,pos));
         }

         return agents;
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
     template<unsigned short dim=2, typename ParentModel>
     auto create_grid_from_model(const std::string name,
                                 const ParentModel& parent_model) {
        // Get the logger
        const auto log = parent_model.get_logger();
        log->info("Setting up grid from model instance '{}'...", name);

        // Get the configuration
        const auto cfg = parent_model.get_cfg()[name];

        // Extract grid size from config
        static_assert(dim == 2 || dim == 3,
                      "Template argument dim must be 2 or 3!");
        const auto gsize = get_as<std::array<unsigned int, dim>>("grid_size",
                                                                 cfg);
        
        // Inform about the size
        if constexpr (dim == 2) {
            log->info("Creating 2-dimensional grid of size: {} x {} ...",
                      gsize[0], gsize[1]);
        }
        else {
            log->info("Creating 3-dimensional grid of size: {} x {} x {} ...",
                      gsize[0], gsize[1], gsize[2]);
        }

        // Create grid of that size and return
        return create_grid<dim>(gsize);
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
    template<typename State,
             bool periodic=true,
             unsigned short dim=2,
             bool structured=true,
             bool sync=true,
             typename ParentModel
            >
    auto create_grid_manager_cells(const std::string name,
                                   const ParentModel& parent_model,
                                   const State initial_state = State())
    {
        // Get the grid, passing through arguments
        auto grid = create_grid_from_model<dim>(name, parent_model);

        // Create cells on that grid, passing the initial state
        auto cells = create_cells_on_grid<sync>(grid, initial_state);

        // Create the grid manager, passing the template argument
        parent_model.get_logger()->info("Initializing GridManager with {} "
                                        "boundary conditions ...",
                                        (periodic ? "periodic" : "fixed"));
        // TODO add the other template arguments to the log message
        
        return create_manager_cells<structured, periodic>(grid, cells);
    }

    // TODO add equivalent function for agents on grid


} // namespace Setup
} // namespace Utopia

#endif // SETUP_HH
