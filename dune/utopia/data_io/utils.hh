#ifndef DATAIO_UTILS_HH
#define DATAIO_UTILS_HH

#include <boost/core/demangle.hpp>
#include <yaml-cpp/yaml.h>
#include "types.hh"

namespace Utopia {
namespace DataIO {

// -- Config access convenience functions -- //

/// General config access via template parameter
template<typename ReturnType>
ReturnType as_(Config node) {
    // Check if the node is valid
    // TODO

    // Try reading
    try {
        return node.template as<ReturnType>();
    }
    // Did not work -> try to give a reasonable error message
    catch (YAML::BadConversion& e) {
        // Probably due to the node being a zombie, e.g. b/c key was missing
        std::cerr << "Could not read from config due to a "
                  << boost::core::demangle(typeid(e).name()) << "! "
                  << "Is conversion possible? Does the node even exist?"
                  << std::endl;

        // Re-throw the original exception
        throw;
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

/// Shortcut to retrieve an entry as double
double as_double(Config node) {
    return as_<double>(node);
}

/// Shortcut to retrieve an entry as bool
bool as_bool(Config node) {
    return as_<bool>(node);
}

/// Shortcut to retrieve an entry as std::string
std::string as_str(Config node) {
    return as_<std::string>(node);
}


} // namespace DataIO
} // namespace Utopia
#endif // DATAIO_UTILS_HH
