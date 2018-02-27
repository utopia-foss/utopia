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


template<typename GridType, bool structured, bool periodic,
    typename CellType, typename AgentType>
class GridManager
{
public:
    using Traits = GridTypeAdaptor<GridType>;
    using Cell = CellType;
    using Agent = AgentType;

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

    //! container for CA cells
    CellContainer<Cell> _cells;

    //! container for agents
    AgentContainer<Agent> _agents;

public:

    /// Constructor for grid an CA cells
    explicit GridManager (
        const GridWrapper<GridType>& wrapper,
        const CellContainer<Cell>& cells ) :
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv),
        _cells(cells)
    { }

    /// Constructor for grid, CA cells, and Agents
    explicit GridManager (
        const GridWrapper<GridType>& wrapper,
        const CellContainer<Cell>& cells,
        const AgentContainer<Agent>& agents ) :
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv),
        _cells(cells),
        _agents(agents)
    { }

    /// Constructor for grid and agents
    explicit GridManager (
        const GridWrapper<GridType>& wrapper,
        const AgentContainer<Agent>& agents ) :
        _grid(wrapper._grid),
        _grid_cells(wrapper._grid_cells),
        _extensions(wrapper._extensions),
        _gv(_grid->leafGridView()),
        _mapper(_gv),
        _agents(agents)
    { }

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

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const { return _cells; }
    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents () const { return _agents; }
    /// Return reference to the managed agents
    AgentContainer<Agent>& agents () { return _agents; }

};

} // namespace Utopia

#endif // GRID_HH