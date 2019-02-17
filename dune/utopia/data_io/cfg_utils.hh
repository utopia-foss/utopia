#ifndef DATAIO_CFG_UTILS_HH
#define DATAIO_CFG_UTILS_HH

#include <boost/core/demangle.hpp>
#include <yaml-cpp/yaml.h>


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
    // TODO Catch other YAML exceptions
    // TODO Check for zombie somehow?!
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


/// Return the entry with the specified key from the specified node
/** \detail Unlike as_, this method checks whether the key is present, and,
  *         if not, throws an error.
  */
template<typename ReturnType>
ReturnType get_as_(const Utopia::DataIO::Config& node, const std::string key) {
    if (not node[key]) {
        throw std::invalid_argument("KeyError: " + key);
    }

    return as_<ReturnType>(node[key]);
}

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


} // namespace Utopia
#endif // DATAIO_CFG_UTILS_HH
