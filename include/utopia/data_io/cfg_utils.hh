#ifndef DATAIO_CFG_UTILS_HH
#define DATAIO_CFG_UTILS_HH

#include <list>
#include <sstream>

#include <boost/core/demangle.hpp>
#include <armadillo>
#include <yaml-cpp/yaml.h>

#include "../core/exceptions.hh"
#include "../core/string.hh"
#include "../core/types.hh"

namespace Utopia
{
namespace DataIO
{

/*!
 * \addtogroup DataIO
 * \{
 */

/*!
 * \addtogroup ConfigUtilities
 * \{
 */

/**
 * \page ConfigUtils Configuration file Module
 *
 * \section what Overview
 * This module implements functions which allow for improved access to yaml-cpp
 * processed yaml-files. The primary focus lies on improved exceptions and
 * usability, mainly through the ::Utopia::get_as function.
 *
 * Utopia relies heavily on the use of YAML configuration files. These files
 * and substructures of it are available as so called "config nodes",
 * ::Utopia::DataIO::Config objects, which are hierarchically nested maps,
 * sequences, and scalars.
 *
 * When retrieving values from these nodes, a type cast becomes necessary.
 * While the library used for YAML reading provides such a function, we found
 * it more convenient to define some thin wrappers around them, which supply
 * more information when something goes wrong.
 *
 * To remember the order of arguments, this sentence can be employed:
 * ``get_as`` a ``double``: the entry named ``my_double`` from this ``cfg``
 * node. Of course, YAML has no types associated with each entry; the
 * ::Utopia::get_as function *tries* to do this conversion and, if it fails,
 * throws an exception.
 *
 * \section impl Implementation
 * The core of this module consists of the ::get_as function, which i
 * conjunction with ::improve_yaml_exception works to improve error messages
 * and simplifies the syntax of converting yaml nodes to c++ types.
 * Additionally, specializations for non-standard types are provided, e.g. for
 * working with types from the Armadillo linear algebra package.
 */

// end group ConfigUtilities
/**
 *  \}
 */

// .. Config reading helper functions .........................................

// TODO Make this an actual Utopia exception
/// Improves yaml-cpp exceptions occurring for a given node
template < class Exc >
YAML::Exception
improve_yaml_exception(const Exc&    e,
                       const Config& node,
                       std::string   prefix = {})
{
    // The string stream for the new, improved error message
    std::stringstream e_msg;
    e_msg << prefix << " ";
    e_msg << "Got " << boost::core::demangle(typeid(e).name()) << ". ";

    // Create a custom error message depending on whether the node is a
    // zombie or a mark is available
    if (not node)
    {
        // Was a zombie
        e_msg << "The given node was a Zombie! Check that the key you are "
                 "trying to read from actually exists. ";
    }
    else if (not node.Mark().is_null())
    {
        // A mark is available to use as a hint
        // NOTE Mark() provides the line and column in the config file, if
        //      available, i.e.: if not a zombie node
        e_msg << "Check that the corresponding line of the config file "
                 "matches the desired read operation or type conversion. ";
    }

    // Give some information on the node's content:
    e_msg << "The content of the node is:" << std::endl << YAML::Dump(node);

    // Return the custom exception object; can be thrown on other side
    return YAML::Exception(node.Mark(), e_msg.str());
}

/// Given a config node, returns a string representation of it
/** This is done by dumping the config node into an std::stringstream.
 */
std::string
to_string(const Config& node)
{
    std::stringstream s;
    s << YAML::Dump(node);
    return s.str();
}

} // namespace DataIO

/*!
 * \addtogroup ConfigUtilities
 * \{
 */

// NOTE All code below is not inside the Utopia::DataIO namespace to make
//      includes into models more convenient.

/// This function is a wrapper around the yaml-cpp YAML::Node::as function
/** It enhances the error messages that can occur in a read operation.
 *
 *  Example:
 *  \code{.cc}
 *   using namespace Utopia;
 *
 *   auto cfg = YAML::Load("{my_double: 3.14, my_int: 42, my_str: foo}");
 *   auto my_double = get_as<double>("my_double", cfg);
 *   auto my_uint = get_as<unsigned int>("my_int", cfg);
 *   auto my_int = get_as<int>("my_int", cfg);
 *   auto my_str = get_as<std::string>("my_str", cfg);
 *  \endcode
 *
 *  \note  This method may throw Utopia::KeyError,
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
template < typename ReturnType >
ReturnType
get_as(const std::string& key, const DataIO::Config& node)
{
    try
    {
        return node[key].template as< ReturnType >();
    }
    catch (YAML::Exception& e)
    {
        if (not node[key])
        {
            throw KeyError(key, node);
        }
        // else: throw an improved error message
        throw DataIO::improve_yaml_exception(e, node,
                                             "Could not read key '" + key +
                                             "' from given config node!");
    }
    catch (std::exception& e)
    {
        // Some other exception; provide at least some info and context
        std::cerr << boost::core::demangle(typeid(e).name())
                  << " occurred during reading key '" << key
                  << "' from config!" << std::endl;

        // Re-throw the original exception
        throw;
    }
    catch (...)
    {
        throw std::runtime_error("Unexpected exception occurred during "
                                 "reading key '" + key + "'' from config!");
    }
}

/// Like Utopia::get_as, but instead of KeyError, returns a fallback value
template < typename ReturnType >
ReturnType
get_as(const std::string& key, const DataIO::Config& node, ReturnType fallback)
{
    try
    {
        return get_as< ReturnType >(key, node);
    }
    catch (KeyError&)
    {
        return fallback;
    }
    // All other errors will (rightly) be thrown
}

// end group ConfigUtilities
/**
 *  \}
 */

// Armadillo-related specialization
// TODO These should be implemented specializations of the get_as function!
namespace DataIO
{

/// Retrieve a config entry as Armadillo column vector using get_
/** \note This method is necessary because arma::Col::fixed cannot be
 *       constructed from std::vector. In such cases, the target vector is
 *       constructed element-wise.
 *
 * \tparam CVecT The Armadillo vector type to return
 * \tparam dim   The dimensionality of the vector (only needed for)
 */
template < typename CVecT, DimType dim = 0 >
CVecT
get_as_arma_vec(const std::string& key, const DataIO::Config& node)
{
    // Extract the field vector element type; assuming Armadillo interface
    using element_t = typename CVecT::elem_type;

    // Check if it can be constructed from a vector
    if constexpr (std::is_constructible< CVecT, std::vector< element_t > >())
    {
        return get_as< std::vector< element_t > >(key, node);
    }
    else
    {
        static_assert(dim > 0,
                      "Need template argument dim given if target type is not "
                      "constructible from std::vector.");

        // Needs to be constructed element-wise
        CVecT      cvec;
        const auto vec = get_as< std::array< element_t, dim > >(key, node);

        for (DimType i = 0; i < dim; i++)
        {
            cvec[i] = vec[i];
        }

        return cvec;
    }
}
} // namespace DataIO


/*!
 * \addtogroup ConfigUtilities
 * \{
 */

/// Special case of Utopia::get_as to retrieve an entry as SpaceVec
/** \tparam dim The dimensionality of the returned Utopia::SpaceVecType
 */
template < DimType dim >
SpaceVecType< dim >
get_as_SpaceVec(const std::string& key, const DataIO::Config& node)
{
    return DataIO::get_as_arma_vec< SpaceVecType< dim >, dim >(key, node);
}

/// Special case of Utopia::get_as to retrieve an entry as MultiIndex
/** \tparam dim The dimensionality of the returned Utopia::MultiIndexType
 */
template < DimType dim >
MultiIndexType< dim >
get_as_MultiIndex(const std::string& key, const DataIO::Config& node)
{
    return DataIO::get_as_arma_vec< MultiIndexType< dim >, dim >(key, node);
}

// end group ConfigUtilities
/**
 *  \}
 */

// -- INTERNALLY USED Functions to work with Config trees ---------------------

namespace _internal {

/// Helper function for recursive_setitem
/** Expects an (already deep-copied) node that is then recursively iterated
  * through along the key sequence. For the last key in the sequence, the
  * value is being set. Empty path segments in the key sequence are skipped.
  *
  * \throws     std::invalid_argument upon *trailing* empty path segments in
  *             the key sequence.
  */
template<class T, class Keys = std::list<std::string> >
Config __recursive_setitem (Config d,
                            Keys&& key_sequence,
                            const T& val)
{
    // Get the next key
    std::string key;
    while (key.empty() and not key_sequence.empty()) {
        key = key_sequence.front();
        key_sequence.pop_front();
    }
    if (key.empty()) {
        throw std::invalid_argument(
            "During recursive_setitem, failed to retrieve a valid key for "
            "continuing recursion. Make sure the given key sequence ("
            + join(key_sequence, " -> ") + ") contains no empty elements!"
        );
    }

    // Check for end of recursion
    if (key_sequence.empty()) {
        if (d.IsScalar()) {
            // Discard the scalar and replace it with an empty mapping
            d = Config{};
        }
        d[key] = val;
        return d;
    }

    // Continue recursion, creating the intermediate node if it does not exist
    if (not d[key]) {
        d[key] = Config{};
    }
    d[key] = __recursive_setitem(d[key], std::forward<Keys>(key_sequence),
                                 val);
    return d;
}


/// Recursively retrieve an element from the configuration tree
/**
  * Actually uses a loop internally to avoid recursion... ¯\_(ツ)_/¯
  *
  * \note       Only works with string-like keys, i.e. can't access sequences.
  *
  * \warning    Due to yaml-cpp quirks, creates a deep copy of the given config
  *             to keep clear of any mutability side effects. Subsequently,
  *             this function should not be used in performance-critical code.
  *             Furthermore, to *set* an element in the configuration tree, the
  *             `recursive_setitem` functionality should be used -- assigning a
  *             value to the returned Config object will **not** lead to a
  *             change in the tree provided via the `d` argument.
  *
  * \param  d               The configuration tree to retrieve the element from
  * \param  key_sequence    A key sequence denoting the path within the tree
  *                         to get the item from.
  *
  * \returns    Config      The configuration tree element at the given path.
  */
Config recursive_getitem (const Config& d,
                          const std::vector<std::string>& key_sequence)
{
    using Utopia::KeyError;
    using Utopia::join;

    Config rv = YAML::Clone(d);
    for (const auto& key : key_sequence ) {
        try {
            rv = get_as<Config>(key, rv);
        }
        catch (KeyError& err) {
            throw KeyError(
                key, rv,
                "recursive_getitem failed for key or key sequence '"
                + join(key_sequence, " -> ") + "'!"
            );
        }
    }
    return rv;
}

/// Overload for recursive_getitem, accepting a string-like key sequence
/** Returns the configuration node at the path specified by the key sequence.
  *
  * \param  d               The configuration tree to retrieve the element from
  * \param  key_sequence    A string denoting the path within the tree to get
  *                         the item from. The path is split into segments
  *                         using Utopia::split
  * \param  delims          Delimiters (plural!) that split the string into a
  *                         key sequence.
  *
  * \returns    Config      The configuration tree element at the given path.
  *
  * \warning    *Any* character in the `delims` sequence acts as separator.
  *             See Utopia::split for more information on behaviour of the
  *             `delims` argument.
  */
Config recursive_getitem (const Config& d,
                          const std::string& key_sequence,
                          const std::string& delims = ".")
{
    return recursive_getitem(d, split(key_sequence, delims));
}


/// Recursively sets an element in a configuration tree
/** This also creates all intermediate segments that may be missing.
  *
  * As the YAML::Node::operator[] does not return references, this internally
  * has to create a (deep) copy of the given node `d`. After recursing through
  * it and setting the value, it will finally assign to the `d` reference.
  *
  * \note   Only works with string-like keys, i.e. can't access sequences.
  *
  * \param  d             The configuration tree to set the entry in.
  * \param  key_sequence  A container with a sequence of keys to walk along
  *                       inside the tree.
  * \param  val           The value to assign to the element of the tree
  *                       specified by the key sequence.
  */
template<class T>
void recursive_setitem (Config& d,
                        std::list<std::string> key_sequence,
                        const T val)
{
    if (key_sequence.empty()) {
        throw std::invalid_argument(
            "Key sequence for recursive_setitem may not be empty!"
        );
    }
    d = __recursive_setitem(YAML::Clone(d), std::move(key_sequence), val);
}

/// Overload for recursive_setitem that splits a string into a key sequence
/**
  * \param  d             The configuration tree to set the entry in.
  * \param  key_sequence  A string to split into a sequence of keys to walk
  *                       along inside the tree.
  * \param  val           The value to assign to the element of the tree
  *                       specified by the key sequence.
  * \param  delims        Delimiters to generate a key sequence from the given
  *                       string `key_sequence`.
  *
  * \warning     *Any* character in the `delims` sequence acts as separator.
  *              See Utopia::split for more information on behaviour of the
  *              `delims` argument.
  */
template<class T>
void recursive_setitem (Config& d,
                        const std::string& key_sequence,
                        const T val,
                        const std::string& delims = ".")
{
    recursive_setitem(
        d, split<std::list<std::string>>(key_sequence, delims), val
    );
}

} // namespace _internal
} // namespace Utopia

#endif // DATAIO_CFG_UTILS_HH
