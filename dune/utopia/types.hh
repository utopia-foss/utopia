#ifndef TYPES_HH
#define TYPES_HH

namespace Utopia
{
    
/// Type of default grid: Rectangular, lower left cell center has coordinates (0,0)
template<int dim>
using DefaultGrid = Dune::YaspGrid<dim>;

/// Type of default random number generator
using DefaultRNG = std::mt19937;

/// Extrct data types dependent on the grid data type
/** \tparam GridType Type of the grid
 */
template<typename GridType>
struct GridTypeAdaptor
{
    //! spatial dimensions of the grid
    static constexpr int dim = GridType::dimension;
    //! Coordinate type
    using Coordinate = typename GridType::ctype;
    //! Position vector
    using Position = typename Dune::FieldVector<Coordinate,dim>;
    //! Type of GridView implemented by Dune
    using GridView = typename GridType::LeafGridView;
    //! Type of VTKSequenceWriter
    using VTKWriter = typename Dune::VTKSequenceWriter<GridView>;
    //! Type of Grid Index Mapper
    using Mapper = typename Dune::MultipleCodimMultipleGeomTypeMapper<GridView>;
    //! Type of grid index
    using Index = typename Mapper::Index;
};

/// Type of the variably sized container for cells
template<typename CellType>
using CellContainer = std::vector<std::shared_ptr<CellType>>;

/// Type of the variably sized container for agents
template<typename AgentType>
using AgentContainer = std::vector<std::shared_ptr<AgentType>>;

/// Container dummy if no cells or individuals are used
using EmptyContainer = std::array<std::shared_ptr<int>,0>;

} // namespace Utopia

#endif // TYPES_HH
