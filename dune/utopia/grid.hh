#ifndef GRID_HH
#define GRID_HH

namespace Utopia {

/// Struct for returning relevant grid data from Setup functions
template<typename GridType>
struct GridWrapper
{
private:
    using Traits = GridTypeAdaptor<GridType>;
    using Coordinate = typename Traits::Coordinate;
    static constexpr int dim = Traits::dim;

public:
    //! pointer to the grid
    std::shared_ptr<GridType> _grid;
    //! grid extensions in each dimension
    std::array<Coordinate,dim> _extensions;
    //! cells on the grid in each dimension
    std::array<unsigned int,dim> _grid_cells;
};

/// Common base class for grid managers holding the actual grid
template<typename GridType, bool structured, bool periodic, typename RNG>
class GridManagerBase
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;

private:
    using Grid = GridType;
    using GV = typename Traits::GridView;
    using Mapper = typename Traits::Mapper;
    using Index = typename Traits::Index;
    using Coordinate = typename Traits::Coordinate;
    using Position = typename Traits::Position;
    static constexpr int dim = Traits::dim;

    static constexpr bool _is_structured = structured;
    static constexpr bool _is_periodic = periodic;

    //! pointer to the Dune grid
    std::shared_ptr<Grid> _grid;
    //! cells on the grid in each dimension
    std::array<unsigned int,dim> _grid_cells;
    //! grid extensions in each dimension
    std::array<Coordinate,dim> _extensions;
    //! Dune GridView object for access to grid entities
    const GV _gv;
    //! Dune Mapper for grid entities
    const Mapper _mapper;
    // Random Number Generator
    std::shared_ptr<RNG> _rng;

public:

    /// Create a GridManager from a grid and cells with a default RNG
    explicit GridManagerBase (const GridWrapper<GridType>& wrapper, std::shared_ptr<RNG> rng):
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv, Dune::mcmgElementLayout()),
        _rng(rng)
    { }

    /// Create a GridManager from a grid and cells 
/*    explicit GridManagerBase (const GridWrapper<GridType>& wrapper, std::shared_ptr<RNG> rng):
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv, Dune::mcmgElementLayout(),
        _rng(rng))
    { }
*/
    /// Return true if managed grid is structured (rectangular)
    static constexpr bool is_structured () { return _is_structured; }
    /// Return true if managed grid is periodic
    static constexpr bool is_periodic () { return _is_periodic; }

    /// Return shared pointer to the managed grid
    std::shared_ptr<Grid> grid () const { return _grid; }
    /// Return reference to the grid view
    const GV& grid_view () const { return _gv; }
    /// Return reference to the grid entity mapper
    const Mapper& mapper () const { return _mapper; }

    /// Return number of cells into each direction (for structured grid)
    const std::array<unsigned int,dim>& grid_cells () const { return _grid_cells; }
    /// Return grid extensions
    const std::array<Coordinate,dim>& extensions () const { return _extensions; }
};

/// Define Type for destinguishing managers
struct Manager {
    enum Type { Cells, Agents };
};

/// Unspecified grid manager
template<Manager::Type type, typename DataType, typename GridType, bool structured, bool periodic, typename RNG = DefaultRNG>
class GridManager
{};

/// GridManager specialization for cells
template<typename DataType, typename GridType, bool structured, bool periodic, typename RNG>
class GridManager<Manager::Cells, DataType, GridType, structured, periodic, RNG>:
    public GridManagerBase<GridType, structured, periodic, RNG>
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;
    /// Base class type
    using Base = GridManagerBase<GridType, structured, periodic, RNG>;
    /// Data type of cells (shared pointer to it)
    using Cell = DataType;

private:
    /// container for CA cells
    CellContainer<Cell> _cells;

public:
    /// Create a GridManager from a grid and cells
    explicit GridManager (const GridWrapper<GridType>& wrapper,
        const CellContainer<Cell>& cells, 
        std::shared_ptr<RNG> rng = std::make_shared<RNG>(0)):
        Base(wrapper, rng),
        _cells(cells)
    { }

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const { return _cells; }
};

/// GridManager specialization for cells
template<typename DataType, typename GridType, bool structured, bool periodic, typename RNG>
class GridManager<Manager::Agents, DataType, GridType, structured, periodic, RNG>:
    public GridManagerBase<GridType, structured, periodic, RNG>
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;
    /// Base class type
    using Base = GridManagerBase<GridType, structured, periodic, RNG>;
    /// Data type of cells (shared pointer to it)
    using Agent = DataType;

private:
    /// container for agents
    AgentContainer<Agent> _agents;

public:
    /// Create a GridManager from a grid and cells
    explicit GridManager (const GridWrapper<GridType>& wrapper,
        const AgentContainer<Agent>& agents, 
        std::shared_ptr<RNG> rng = std::make_shared<RNG>(0)):
        Base(wrapper, rng),
        _agents(agents)
    { }

    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents () const { return _agents; }
    /// Return reference to the managed agents
    AgentContainer<Agent>& agents () { return _agents; }
};

} // namespace Utopia

#endif // GRID_HH
