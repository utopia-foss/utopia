#ifndef UTOPIA_CORE_TYPES_HH
#define UTOPIA_CORE_TYPES_HH

#include <cstdint>
#include <vector>
#include <memory>
#include <random>
#include <array>

#include <armadillo>
#include <yaml-cpp/yaml.h>


namespace Utopia {

/// Type of default random number generator
using DefaultRNG = std::mt19937;


/// Type of the variably sized container for entities
template<typename EntityType>
using EntityContainer = std::vector<std::shared_ptr<EntityType>>;

/// Type of the variably sized container for cells
template<typename CellType>
using CellContainer = EntityContainer<CellType>;

/// Type of the variably sized container for agents
template<typename AgentType>
using AgentContainer = EntityContainer<AgentType>;


/// Type for dimensions, i.e. very small unsigned integers
using DimType = unsigned short;

/// Type for distancens, i.e. intermediately long unsigned integers
using DistType = unsigned int;

/// Type for indices, i.e. values used for container indexing, agent IDs, ...
using IndexType = std::size_t;

/// Type for container of indices
using IndexContainer = std::vector<IndexType>;

/// Type for index type vectors that are associated with a physical space
/** \details Uses a fixed-size Armadillo column vector of IndexType
  *
  * \note   This vector is not to be interpreted as a "container"
  *
  * \tparam dim  The dimensionality (or: rank) of the vector
  */
template<DimType dim>
using MultiIndexType = arma::Col<IndexType>::fixed<dim>;

/// Type for vector-like data that is associated with a physical space
/** \details Uses a fixed-size Armadillo column vector of doubles
  *
  * \tparam dim  The dimensionality (or: rank) of the vector
  */
template<DimType dim>
using SpaceVecType = arma::Col<double>::fixed<dim>;

// .. DataIO types that are needed throughout Core ............................
namespace DataIO {
/**
 *  \addtogroup ConfigUtilities
 *  \{
 */

/// Type of a variadic dictionary-like data structure used throughout Utopia
using Config = YAML::Node;

// end group ConfigUtilities
/**
 *  \}
 */
} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_CORE_TYPES_HH
