#ifndef DATAIO_CFG_UTILS_HH
#define DATAIO_CFG_UTILS_HH

#include <boost/core/demangle.hpp>

#include "../core/types.hh"  // NOTE: DataIO::Config type declared there
#include "../core/exceptions.hh"


namespace Utopia {
namespace DataIO {
// Config reading helper functions ........................................

/// Improves yaml-cpp exceptions occurring for a given node
template<class Exc>
YAML::Exception improve_yaml_exception(const Exc& e,
                                       const Config& node,
                                       std::string prefix = {})
{
    // The string stream for the new, improved error message
    std::stringstream e_msg;
    e_msg << prefix << " ";
    e_msg << "Got " << boost::core::demangle(typeid(e).name()) << ". ";

    // Create a custom error message depending on whether the node is a
    // zombie or a mark is available
    if (not node) {
        // Was a zombie
        e_msg << "The given node was a Zombie! Check that the key you are "
                 "trying to read from actually exists. ";
    }
    else if (not node.Mark().is_null()) {
        // A mark is available to use as a hint 
        // NOTE Mark() provides the line and column in the config file, if
        //      available, i.e.: if not a zombie node
        e_msg << "Check that the corresponding line of the config file "
                 "matches the desired read operation or type conversion. ";
    }

    // Give some information on the node's content:
    e_msg << "The content of the node is:  " << YAML::Dump(node);

    // Return the custom exception object; can be thrown on other side
    return YAML::Exception(node.Mark(), e_msg.str());
}
} // namespace DataIO

// NOTE All code below is not inside the Utopia::DataIO namespace to make
//      includes into models more convenient.


// -- as_ interface -----------------------------------------------------------
/**
 *  \addtogroup ConfigUtilities
 *  \{
 */

/// Read an entry from a config node and convert it to a certain return type
/** This function is a wrapper around the yaml-cpp YAML::Node::as fuction and
 *  enhances the error messages that can occurr in a read operation.
 *
 *  \deprecated  This function is deprecated as the error messages are hard to
 *               interpret in cases of zombie nodes; as an alternative, use
 *               the Utopia::get_as function and associated shortcuts.
 *
 *  \tparam  ReturnType  The type to evaluate from the YAML::Node
 *
 *  \param   node        The configuration node to evaluate
 *
 *  \return  The value of the node, cast to ReturnType
 *  \throw   YAML::Exception On bad conversions or non-existent nodes
 */
template<typename ReturnType>
[[deprecated("The as_ function is deprecated and will soon be removed! Please "
             "use the new get_as(key, node) method instead.")]]
ReturnType as_(const DataIO::Config& node) {
    // Try reading
    try {
        return node.template as<ReturnType>();
    }
    // Did not work -> try to give a understandable error message
    catch (YAML::Exception& e) {
        // Presumably due to the node being a zombie or a bad type
        // conversion... Re-throw with improved custom error message:
        throw DataIO::improve_yaml_exception(e, node,
                                             "Could not read from config!");
    }
    catch (std::exception& e) {
        // Some other exception; provide at least some info and context
        std::cerr << boost::core::demangle(typeid(e).name())
                  << " occurred during reading from config!" << std::endl;

        // Re-throw the original exception
        throw;
    }
    catch (...) {
        throw std::runtime_error("Unexpected exception occurred during "
                                 "reading from config!");
    }
}

/// Shortcut for Utopia::as_ to retrieve an entry as int
int as_int(const DataIO::Config& node) {
    return as_<int>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as double
double as_double(const DataIO::Config& node) {
    return as_<double>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as bool
bool as_bool(const DataIO::Config& node) {
    return as_<bool>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as std::string
std::string as_str(const DataIO::Config& node) {
    return as_<std::string>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as std::vector
template<typename T>
std::vector<T> as_vector(const DataIO::Config& node) {
    return as_<std::vector<T>>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as std::array
template<typename T, std::size_t len>
std::array<T, len> as_array(const DataIO::Config& node) {
    return as_<std::array<T, len>>(node);
}

// end group ConfigUtilities
/**
 *  \}
 */


// Armadillo-related specialization
namespace DataIO {
/// Retrieve a config entry as Armadillo column vector
/** \note This method is necessary because arma::Col::fixed cannot be
  *       constructed from std::vector. In such cases, the target vector is
  *       constructed element-wise.
  *
  * \tparam CVecT The Armadillo vector type to return
  * \tparam dim   The dimensionality of the vector (only needed for)
  */
template<typename CVecT, DimType dim=0>
CVecT as_arma_vec(const DataIO::Config& node) {
    // Extract the field vector element type; assuming Armadillo interface
    using element_t = typename CVecT::elem_type;

    // Check if it can be constructed from a vector
    if constexpr (std::is_constructible<CVecT, std::vector<element_t>>()) {
        return as_<std::vector<element_t>>(node);
    }
    else {
        static_assert(dim > 0,
            "Need template argument dim given if target type is not "
            "constructible from std::vector.");

        // Needs to be constructed element-wise
        CVecT cvec;
        const auto vec = as_array<element_t, dim>(node);

        for (DimType i=0; i<dim; i++) {
            cvec[i] = vec[i];
        }

        return cvec;
    }
}
} // namespace DataIO


/**
 *  \addtogroup ConfigUtilities
 *  \{
 */

/// Shortcut for Utopia::as_ to retrieve an entry as SpaceVec
/** \tparam dim The dimensionality of the returned Utopia::SpaceVecType
 */
template<DimType dim>
SpaceVecType<dim> as_SpaceVec(const DataIO::Config& node) {
    return DataIO::as_arma_vec<SpaceVecType<dim>, dim>(node);
}

/// Shortcut for Utopia::as_ to retrieve an entry as MultiIndex
/** \tparam dim The dimensionality of the returned Utopia::MultiIndexType
 */
template<DimType dim>
MultiIndexType<dim> as_MultiIndex(const DataIO::Config& node) {
    return DataIO::as_arma_vec<MultiIndexType<dim>, dim>(node);
}


// -- get_ interface ----------------------------------------------------------

/// Return the entry with the specified key from the specified node
/** This function is a wrapper around the yaml-cpp YAML::Node::as fuction and
 *  enhances the error messages that can occurr in a read operation.
 *
 *  \note  Unlike Utopia::as_, this method may throw Utopia::KeyError,
 *         which contains the name of the key that could not be accessed.
 *         Also, the conversion needs to be supported by the underlying YAML
 *         library.
 *
 *  \tparam ReturnType  The type to evaluate from the YAML::Node
 *
 *  \param  key         The key that is to be read
 *  \param  node        The node to read the entry with the given key from
 *
 *  \return The value of node[key], cast to ReturnType
 *
 *  \throw  Utopia::KeyError On missing key
 *  \throw  YAML::Exception On bad conversions or other YAML-related errors
 */
template<typename ReturnType>
ReturnType get_as(const std::string& key, const DataIO::Config& node) {
    try {
        return node[key].template as<ReturnType>();
    }
    catch (YAML::Exception& e) {
        if (not node[key]) {
            throw KeyError(key, node);
        }
        // else: throw an improved error message
        throw DataIO::improve_yaml_exception(e, node,
            "Could not read key '" + key + "' from given config node!");
    }
    catch (std::exception& e) {
        // Some other exception; provide at least some info and context
        std::cerr << boost::core::demangle(typeid(e).name())
                  << " occurred during reading key '" << key
                  << "' from config!" << std::endl;

        // Re-throw the original exception
        throw;
    }
    catch (...) {
        throw std::runtime_error("Unexpected exception occurred during "
            "reading key '" + key + "'' from config!");
    }
}

// end group ConfigUtilities
/**
 *  \}
 */


// Armadillo-related specialization
// TODO These should be implemented specializations of the get_as function!
namespace DataIO {
/// Retrieve a config entry as Armadillo column vector using get_
/** \note This method is necessary because arma::Col::fixed cannot be
  *       constructed from std::vector. In such cases, the target vector is
  *       constructed element-wise.
  *
  * \tparam CVecT The Armadillo vector type to return
  * \tparam dim   The dimensionality of the vector (only needed for)
  */
template<typename CVecT, DimType dim=0>
CVecT get_as_arma_vec(const std::string& key, const DataIO::Config& node) {
    // Extract the field vector element type; assuming Armadillo interface
    using element_t = typename CVecT::elem_type;

    // Check if it can be constructed from a vector
    if constexpr (std::is_constructible<CVecT, std::vector<element_t>>()) {
        return get_as<std::vector<element_t>>(key, node);
    }
    else {
        static_assert(dim > 0,
            "Need template argument dim given if target type is not "
            "constructible from std::vector.");

        // Needs to be constructed element-wise
        CVecT cvec;
        const auto vec = get_as<std::array<element_t, dim>>(key, node);

        for (DimType i=0; i<dim; i++) {
            cvec[i] = vec[i];
        }

        return cvec;
    }
}
} // namespace DataIO


/**
 *  \addtogroup ConfigUtilities
 *  \{
 */

/// Special case of Utopia::get_as to retrieve an entry as SpaceVec
/** \tparam dim The dimensionality of the returned Utopia::SpaceVecType
 */
template<DimType dim>
SpaceVecType<dim> get_as_SpaceVec(const std::string& key,
                                  const DataIO::Config& node)
{
    return DataIO::get_as_arma_vec<SpaceVecType<dim>, dim>(key, node);
}

/// Special case of Utopia::get_as to retrieve an entry as MultiIndex
/** \tparam dim The dimensionality of the returned Utopia::MultiIndexType
 */
template<DimType dim>
MultiIndexType<dim> get_as_MultiIndex(const std::string& key,
                                      const DataIO::Config& node)
{
    return DataIO::get_as_arma_vec<MultiIndexType<dim>, dim>(key, node);
}

// end group ConfigUtilities
/**
 *  \}
 */

} // namespace Utopia
#endif // DATAIO_CFG_UTILS_HH
