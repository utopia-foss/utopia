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
        // Due to the node being a zombie, e.g. b/c key was missing, or a bad
        // type conversion...
        std::stringstream e_msg;

        // Create a custom error message depending on whether the node is a
        // zombie or not ...
        if (not node) {
            e_msg << "Could not read from config because the given node was a "
                     "zombie. Check that the key you are trying to create a "
                     "node with actually exists.";
        }
        else {
            e_msg << "Could not read from config; got "
                  << boost::core::demangle(typeid(e).name()) << "! "
                  << "Check that the corresponding line of the config file "
                     "matches the desired type conversion. "
                     "The value of the node is:  "
                  << YAML::Dump(node);
        }

        // Re-throw with the custom error message
        throw YAML::Exception(node.Mark(), e_msg.str());
        // NOTE Mark() provides the line and column in the config file, if
        //      available, i.e.: if not a zombie node
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
        return as_<ReturnType>(node[key]);
    }
    catch (YAML::Exception& e) {
        if (not node[key]) {
            throw KeyError(key, node);
        }
        throw;
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
    // Extract the field vector element type; assuming Armadillo interface here
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
// TODO Implement as special case of `as_` method, distinguishing by type

/// Shortcut to retrieve a config entry as SpaceVec of given dimensionality
template<DimType dim>
SpaceVecType<dim> as_SpaceVec(const DataIO::Config& node) {
    return as_arma_vec<SpaceVecType<dim>, dim>(node);
}

/// Shortcut to retrieve a config entry as MultiIndex of given dimensionality
template<DimType dim>
MultiIndexType<dim> as_MultiIndex(const DataIO::Config& node) {
    return as_arma_vec<MultiIndexType<dim>, dim>(node);
}



} // namespace Utopia
#endif // DATAIO_CFG_UTILS_HH
