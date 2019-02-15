#ifndef DATAIO_CFG_UTILS_HH
#define DATAIO_CFG_UTILS_HH

#include <boost/core/demangle.hpp>
#include <yaml-cpp/yaml.h>

#include "../core/types.hh"


namespace Utopia {

namespace DataIO
{
    
    /// Type of the configuration
    using Config = YAML::Node;
    // NOTE This type is made available mainly that we can potentially change
    //      the type used for the config. If changing something here, it might
    //      still be required to explicitly change other parts of core and/or
    //      data i/o where yaml-cpp is referenced directly

} // namespace DataIO

// NOTE This is not inside the Utopia::DataIO namespace to make includes more
//      convenient.

// -- Config access convenience functions -- //

/// General config access via template parameter
/** This function is a wrapper for the yaml-cpp `YAML::Node::as` fuction with
 *  helpful error messages.
 *  \tparam ReturnType The type to evaluate from the YAML::Node
 *  \param node The configuration node to evaluate
 *  \return The value of the node cast to ReturnType
 *  \throw YAML::Exception On bad conversions or non-existent nodes
 */
template<typename ReturnType>
ReturnType as_(const Utopia::DataIO::Config& node) {
    // Try reading
    try {
        return node.template as<ReturnType>();
    }
    // Did not work -> try to give a understandable error message
    catch (YAML::BadConversion& e) {
        // Due to the node being a zombie, e.g. b/c key was missing, or an
        // actual bad type conversion...
        std::stringstream e_msg;

        // Create an error message depending on whether there is a mark; if
        // there is none, it is indicative of the node being a zombie...
        if (!node.Mark().is_null()) {
            e_msg << "Could not read from config; got "
                  << boost::core::demangle(typeid(e).name()) << "! "
                  << "Check that the corresponding line of the config file "
                     "matches the desired type conversion. "
                     "The value of the node is:  "
                  << YAML::Dump(node)
                  << std::endl;
        }
        else {
            e_msg << "Could not read from config; got "
                  << boost::core::demangle(typeid(e).name()) << "! "
                  << "Perhaps the node was a zombie? Check that the key you "
                     "are trying to create a node with actually exists."
                  << std::endl;
        }

        // Re-throw with the custom error message
        throw YAML::Exception(node.Mark(), e_msg.str());
        // NOTE Mark() provides the line and column in the config file; the
        //      error message is created depending on whether the mark was null
    }
    catch (std::exception& e) {
        // This should catch all other exceptions thrown by yaml-cpp
        // Provide info on the type of error
        std::cerr << boost::core::demangle(typeid(e).name())
                  << " occurred during reading from config!" << std::endl;

        // Now re-throw the original exception
        throw;
    }
    catch (...) {
        throw std::runtime_error("Unexpected exception occurred during "
                                 "reading from config!");
    }
}

// TODO could add an optional argument to as_ for a fallback value


// -- Shortcuts -- //

/// Shortcut to retrieve a config entry as int
int as_int(const Utopia::DataIO::Config& node) {
    return as_<int>(node);
}

/// Shortcut to retrieve a config entry as double
double as_double(const Utopia::DataIO::Config& node) {
    return as_<double>(node);
}

/// Shortcut to retrieve a config entry as bool
bool as_bool(const Utopia::DataIO::Config& node) {
    return as_<bool>(node);
}

/// Shortcut to retrieve a config entry as std::string
std::string as_str(const Utopia::DataIO::Config& node) {
    return as_<std::string>(node);
}

/// Shortcut to retrieve a config entry as std::vector
template<typename T>
std::vector<T> as_vector(const Utopia::DataIO::Config& node) {
    return as_<std::vector<T>>(node);
}

/// Shortcut to retrieve a config entry as std::array
template<typename T, std::size_t len>
std::array<T, len> as_array(const Utopia::DataIO::Config& node) {
    return as_<std::array<T, len>>(node);
}

/// Retrieve a config entry as Armadillo column vector
/** \note This method is necessary because arma::Col::fixed cannot be
  *       constructed from std::vector. In such cases, the target vector is
  *       constructed element-wise.
  *
  * \tparam CVecT The Armadillo vector type to return
  * \tparam dim   The dimensionality of the vector (only needed for)
  */
template<typename CVecT, DimType dim=0>
CVecT as_arma_vec(const Utopia::DataIO::Config& node) {
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

/// Shortcut to retrieve a config entry as SpaceVec of given dimensionality
template<DimType dim>
SpaceVecType<dim> as_SpaceVec(const Utopia::DataIO::Config& node) {
    return as_arma_vec<SpaceVecType<dim>, dim>(node);
}

/// Shortcut to retrieve a config entry as MultiIndex of given dimensionality
template<DimType dim>
MultiIndexType<dim> as_MultiIndex(const Utopia::DataIO::Config& node) {
    return as_arma_vec<MultiIndexType<dim>, dim>(node);
}



} // namespace Utopia
#endif // DATAIO_CFG_UTILS_HH
