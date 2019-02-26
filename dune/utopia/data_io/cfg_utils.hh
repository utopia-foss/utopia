#ifndef DATAIO_CFG_UTILS_HH
#define DATAIO_CFG_UTILS_HH

#include <boost/core/demangle.hpp>
#include <yaml-cpp/yaml.h>

#include "../core/types.hh"
#include "../core/exceptions.hh"


namespace Utopia {
namespace DataIO {
    /// Type of a dict-like configuration structure used throughout Utopia
    using Config = YAML::Node;
    // NOTE This type is made available mainly that we can potentially change
    //      the type used for the config. If changing something here, it might
    //      still be required to explicitly change other parts of core and/or
    //      data i/o where yaml-cpp is referenced directly

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


// -- Configuration access ----------------------------------------------------
// NOTE The below is not inside the Utopia::DataIO namespace to make includes
//      into models more convenient.

/// Read an entry from a config node and convert it to a certain return type
/** This function is a wrapper for the yaml-cpp `YAML::Node::as` fuction with
 *  helpful error messages.
 *
 *  \tparam  ReturnType  The type to evaluate from the YAML::Node
 *
 *  \param   node        The configuration node to evaluate
 *
 *  \return  The value of the node, cast to ReturnType
 *  \throw   YAML::Exception On bad conversions or non-existent nodes
 */
template<typename ReturnType>
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


/// Return the entry with the specified key from the specified node
/** \detail Unlike as_, this method allows to throw KeyErrors, which contain
  *         the name of the key that could not be accessed
  */
template<typename ReturnType>
ReturnType get_(const std::string& key, const DataIO::Config& node) {
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


// -- Shortcuts ---------------------------------------------------------------

/// Shortcut to retrieve a config entry as int
int as_int(const DataIO::Config& node) {
    return as_<int>(node);
}

/// Shortcut to retrieve a config entry as int using the get_ method
int get_int(const std::string& key, const DataIO::Config& node) {
    return get_<int>(key, node);
}

/// Shortcut to retrieve a config entry as double
double as_double(const DataIO::Config& node) {
    return as_<double>(node);
}

/// Shortcut to retrieve a config entry as double using the get_ method
double get_double(const std::string& key, const DataIO::Config& node) {
    return get_<double>(key, node);
}

/// Shortcut to retrieve a config entry as bool
bool as_bool(const DataIO::Config& node) {
    return as_<bool>(node);
}

/// Shortcut to retrieve a config entry as bool using the get_ method
bool get_bool(const std::string& key, const DataIO::Config& node) {
    return get_<bool>(key, node);
}

/// Shortcut to retrieve a config entry as std::string
std::string as_str(const DataIO::Config& node) {
    return as_<std::string>(node);
}

/// Shortcut to retrieve a config entry as std::string using the get_ method
std::string get_str(const std::string& key, const DataIO::Config& node) {
    return get_<std::string>(key, node);
}

/// Shortcut to retrieve a config entry as std::vector
template<typename T>
std::vector<T> as_vector(const DataIO::Config& node) {
    return as_<std::vector<T>>(node);
}

/// Shortcut to retrieve a config entry as std::vector using the get_ method
template<typename T>
std::vector<T> get_vector(const std::string& key, const DataIO::Config& node) {
    return get_<std::vector<T>>(key, node);
}

/// Shortcut to retrieve a config entry as std::array
template<typename T, std::size_t len>
std::array<T, len> as_array(const DataIO::Config& node) {
    return as_<std::array<T, len>>(node);
}

/// Shortcut to retrieve a config entry as std::array using the get_ method
template<typename T, std::size_t len>
std::array<T, len> get_array(const std::string& key, const DataIO::Config& node) {
    return get_<std::array<T, len>>(key, node);
}


// -- Armadillo-related specializations ---------------------------------------

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

    /// Retrieve a config entry as Armadillo column vector using get_
    /** \note This method is necessary because arma::Col::fixed cannot be
      *       constructed from std::vector. In such cases, the target vector is
      *       constructed element-wise.
      *
      * \tparam CVecT The Armadillo vector type to return
      * \tparam dim   The dimensionality of the vector (only needed for)
      */
    template<typename CVecT, DimType dim=0>
    CVecT get_arma_vec(const std::string& key, const DataIO::Config& node) {
        // Extract the field vector element type; assuming Armadillo interface
        using element_t = typename CVecT::elem_type;

        // Check if it can be constructed from a vector
        if constexpr (std::is_constructible<CVecT, std::vector<element_t>>()) {
            return get_<std::vector<element_t>>(key, node);
        }
        else {
            static_assert(dim > 0,
                "Need template argument dim given if target type is not "
                "constructible from std::vector.");

            // Needs to be constructed element-wise
            CVecT cvec;
            const auto vec = get_array<element_t, dim>(key, node);

            for (DimType i=0; i<dim; i++) {
                cvec[i] = vec[i];
            }

            return cvec;
        }
    }
} // namespace DataIO


/// Shortcut to retrieve a config entry as SpaceVec of given dimensionality
template<DimType dim>
SpaceVecType<dim> as_SpaceVec(const DataIO::Config& node) {
    return DataIO::as_arma_vec<SpaceVecType<dim>, dim>(node);
}

/// Shortcut to retrieve a config entry as SpaceVec using the get_ method
template<DimType dim>
SpaceVecType<dim> get_SpaceVec(const std::string& key,
                               const DataIO::Config& node)
{
    return DataIO::get_arma_vec<SpaceVecType<dim>, dim>(key, node);
}

/// Shortcut to retrieve a config entry as MultiIndex of given dimensionality
template<DimType dim>
MultiIndexType<dim> as_MultiIndex(const DataIO::Config& node) {
    return DataIO::as_arma_vec<MultiIndexType<dim>, dim>(node);
}

/// Shortcut to retrieve a config entry as MultiIndex using the get_ method
template<DimType dim>
MultiIndexType<dim> get_MultiIndex(const std::string& key,
                                   const DataIO::Config& node)
{
    return DataIO::get_arma_vec<MultiIndexType<dim>, dim>(key, node);
}



} // namespace Utopia
#endif // DATAIO_CFG_UTILS_HH
