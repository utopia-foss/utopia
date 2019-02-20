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

/// Type for dimensions, i.e. very small unsigned integers
using DimType = unsigned short;

/// Type for distancens, i.e. intermediately long unsigned integers
using DistType = unsigned int;

/// Type for indices, i.e. values used for container indexing
using IndexType = std::size_t;

/// Type for container of indices
using IndexContainer = std::vector<IndexType>;

/// Type for index type vectors that are associated with a physical space
/** \detail Uses a fixed-size Armadillo column vector of IndexType
  *
  * \note   This vector is not to be interpreted as a "container"
  *
  * \tparam dim  The dimensionality (or: rank) of the vector
  */
template<DimType dim>
using MultiIndexType = arma::Col<IndexType>::fixed<dim>;

/// Type for vector-like data that is associated with a physical space
/** \detail Uses a fixed-size Armadillo column vector of doubles
  *
  * \tparam dim  The dimensionality (or: rank) of the vector
  */
template<DimType dim>
using SpaceVecType = arma::Col<double>::fixed<dim>;

/// Mode of entity update
enum UpdateMode : bool {
    /// Entity update can happen asynchronously
    async = false,

    /// Entity update can happen synchronously
    sync = true
};
// TODO Integrate closer with (new) entity type


} // namespace Utopia

#endif // UTOPIA_CORE_TYPES_HH
