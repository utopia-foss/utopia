#ifndef UTOPIA_CORE_TYPES_HH
#define UTOPIA_CORE_TYPES_HH

#include <cstdint>

#include <armadillo>

#include "../base.hh"

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


namespace impl {

/// Return the pointer type of any container holding pointers to entities
template<class Container>
using pointer_t = typename Container::value_type;

/// Return the element type of any container holding pointers to entities
template<class Container>
using entity_t = typename Container::value_type::element_type;

} // namespace impl


// -- Types introduces with the new CellManager ------------------------------

/// Type for indices
using IndexType = std::size_t;

/// Type for container of indices
using IndexContainer = std::vector<IndexType>;

/// Type for grid shape
template<std::size_t dim>
using GridShapeType = std::array<IndexType, dim>;

/// Type for vector-like data that has some physical meaning
/** \detail Uses a fixed-size Armadillo column vector of doubles
  *
  * \tparam dim  The dimensionality (or: rank) of the vector
  */
template<std::size_t dim>
using FieldVectorType = arma::vec::fixed<dim>;

} // namespace Utopia

#endif // UTOPIA_CORE_TYPES_HH
