#ifndef DATAIO_UTILS_HH
#define DATAIO_UTILS_HH

#include <boost/core/demangle.hpp>
#include <yaml-cpp/yaml.h>
#include "types.hh"

namespace Utopia {
namespace DataIO {

// -- Config access convenience functions -- //

/// General config access via template parameter
template<class ReturnType>
const ReturnType as_(Config node) {
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

// -- Shortcuts -- //

/// Shortcut to retrieve an entry as double
double as_double(Config node) {
    return as_<double>(node);
}

/// Shortcut to retrieve an entry as bool
bool as_bool(Config node) {
    return as_<bool>(node);
}

/// Shortcut to retrieve an entry as std::string
const std::string as_str(Config node) {
    return as_<std::string>(node);
}

/// Shortcut to retrieve an entry as std::vector
template<typename T>
const std::vector<T> as_vector(Config node) {
    return as_<std::vector<T>>(node);
}

/// Shortcut to retrieve an entry as std::array
template<typename T, std::size_t len>
const std::array<T, len> as_array(Config node) {
    return as_<std::array<T, len>>(node);
}


} // namespace DataIO
} // namespace Utopia
#endif // DATAIO_UTILS_HH
