#ifndef GRID_HH
#define GRID_HH

namespace Utopia {

/// Prints output which type of constructor was called
struct CopyMoveAware {
    CopyMoveAware() = default;
    CopyMoveAware(const CopyMoveAware&){
        std::cout << "Copy Constructor called" << std::endl;
    }
    CopyMoveAware(CopyMoveAware&&){
        std::cout << "Move Constructor called" << std::endl;
    }
};

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
template<typename GridType, typename RNG, bool structured, bool periodic>
class GridManagerBase :
    public CopyMoveAware
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
    explicit GridManagerBase (const GridWrapper<GridType>& wrapper,
        const std::shared_ptr<RNG> rng):
        CopyMoveAware(),
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv, Dune::mcmgElementLayout()),
        _rng(rng)
    {
        std::cout << "Custom GridManager Constructor called!" << std::endl;
    }

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

    /// Return shared_ptr to the random number generator
    std::shared_ptr<RNG>& rng () { return _rng; }
};

/// Define Type for destinguishing managers
struct Manager {
    enum Type { Cells, Agents };
};

/// Unspecified grid manager
template<Manager::Type type, typename DataType, typename GridType, typename RNG, bool structured, bool periodic>
class GridManager
{};

/// GridManager specialization for cells
template<typename DataType, typename GridType, typename RNG, bool structured, bool periodic>
class GridManager<Manager::Cells, DataType, GridType, RNG, structured, periodic>:
    public GridManagerBase<GridType, RNG, structured, periodic>
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;
    /// Base class type
    using Base = GridManagerBase<GridType, RNG, structured, periodic>;
    /// Data type of cells (shared pointer to it)
    using Cell = DataType;
    /// Data type of the container
    using Container = CellContainer<Cell>;

private:
    /// container for CA cells
    CellContainer<Cell> _cells;

public:
    /// Create a GridManager from a grid and cells
    explicit GridManager (const GridWrapper<GridType>& wrapper,
        const CellContainer<Cell>& cells, 
        const std::shared_ptr<RNG> rng):
        Base(wrapper, rng),
        _cells(cells)
    { }

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const { return _cells; }
};

/// GridManager specialization for cells
template<typename DataType, typename GridType, typename RNG, bool structured, bool periodic>
class GridManager<Manager::Agents, DataType, GridType, RNG, structured, periodic>:
    public GridManagerBase<GridType, RNG, structured, periodic>
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;
    /// Base class type
    using Base = GridManagerBase<GridType, RNG, structured, periodic>;
    /// Data type of cells (shared pointer to it)
    using Agent = DataType;
    /// Data type of the container
    using Container = AgentContainer<Agent>;

private:
    /// container for agents
    AgentContainer<Agent> _agents;

public:
    /// Create a GridManager from a grid and cells
    explicit GridManager (const GridWrapper<GridType>& wrapper,
        const AgentContainer<Agent>& agents, 
        const std::shared_ptr<RNG> rng):
        Base(wrapper, rng),
        _agents(agents)
    { }

    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents () const { return _agents; }
    /// Return reference to the managed agents
    AgentContainer<Agent>& agents () { return _agents; }

    /// Erase all agents for which the rule evaluates to true
    template<typename Rule>
    void erase_if(Rule rule) {
        auto cutoff = std::remove_if(_agents.begin(), _agents.end(), rule);
        _agents.erase(cutoff, _agents.end());
    }
};

} // namespace Utopia

#endif // GRID_HH
