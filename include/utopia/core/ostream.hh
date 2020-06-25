#ifndef UTOPIA_CORE_OSTREAM_HH
#define UTOPIA_CORE_OSTREAM_HH

#include "type_traits.hh"

namespace Utopia
{

namespace Utils{

/**
 * @brief pretty print a pair
 *
 * @tparam T first type in pair, automatically determined
 * @tparam U second type in pair, automatically determined
 * @param out stream used for output
 * @param pair pair to put out
 */
template < typename T, typename U >
std::ostream&
operator<<(std::ostream& out, const std::pair< T, U >& pair)
{
    out << "(" << pair.first << ", " << pair.second << ")";
    return out;
}

/**
 * Output the content of a container to the stream 'out' in the format
 * '[x1, x2, x3, ...]' where the x_{i} are the elements the container holds.
 * This does not(!) take care of outputting the elements, in the sense that
 * an output operator for the latter has to be preexisting.
 * @param out stream used for output
 * @param container Container object to output.
 */
template < class T >
std::enable_if_t< is_container_v< T >, std::ostream& >
operator<<(std::ostream& out, const T& container)
{
    if (container.size() == 0)
    {
        out << "[]";
    }
    else
    {
        out << std::setprecision(16) << "[";

        for (auto it = container.begin();
             it != std::next(std::begin(container), container.size() - 1);
             ++it)
        {
            out << *it << ", "; // becomes a recursive call in case of nested
                                // containers
        }
        out << *std::next(std::begin(container), container.size() - 1) << "]";
    }
    return out;
}

/**
 * @brief Output container to stream 'out'- specialization for vector of
 * booleans. Note that a vector of booleans is not the same as a vector of any
 * other type because it does some storage optimization to make booleans take up
 * single bits only.
 *
 * @param out stream used for output
 * @param container vector of booleans
 * @return std::ostream&
 */
std::ostream&
operator<<(std::ostream& out, const std::vector< bool >& container)
{
    if (container.size() == 0)
    {
        out << "[]" << std::endl;
    }
    else
    {
        out << "[";
        for (std::size_t i = 0; i < container.size() - 1; ++i)
        {
            out << container[i] << ", ";
        }
        out << container.back() << "]";
    }
    return out;
}

/**
 * @brief Output (unordered) associative containers to an std::ostream
 *
 * @tparam MapType Some type representing an (uordered) associative container,
 * automatically determined.
 * @tparam Key Key type of the container, automatically determined.
 * @tparam Value Mapped type of the container, automatically determined.
 * @tparam Args Additional template args, automatically determined.
 * @param out outstream to use for output
 * @param map Map like type, has to define key_type, map_type and have
 * pair<const Key, Value> as value_type
 * @return std::ostream Used outstream
 */
template < template < typename, typename, typename... > class MapType,
           typename Key,
           typename Value,
           typename... Args >
std::enable_if_t<
    is_associative_container_v< MapType< Key, Value, Args... > > or
        is_unordered_associative_container_v< MapType< Key, Value, Args... > >,
    std::ostream& >
operator<<(std::ostream& out, const MapType< Key, Value, Args... >& map)
{
    if (map.size() == 0)
    {
        out << "[]";
    }
    else
    {
        out << "[";
        for (auto it = map.begin();
             it != std::next(map.begin(), map.size() - 1);
             ++it)
        {
            out << *it << ", ";
        }
        out << *std::next(map.begin(), map.size() - 1) << "]";
    }
    return out;
}

/** @brief Report a tuple to an outstream, This works by piping the single
 *  elements inside the tuple to the stream, and therefor requires `operator<<`
 *  for the underlying types to be defined.
 *
 *  For tuple values `a`, `b`, ..., the output will look like
 *  \verbatim (a, b, ...) \endverbatim
 */
template < typename... Types >
std::ostream&
operator<<(std::ostream& ostr, std::tuple< Types... > tuple)
{
    std::string val_str("(");

    auto report_val = [&val_str](auto&& val) {
        val_str += std::to_string(val) + ", ";
    };
    boost::hana::for_each(tuple, report_val);

    // remove final comma
    val_str.erase(val_str.end() - 2);
    val_str += ")";

    ostr << val_str;
    return ostr;
}


/**
 * @brief Turn any object for which operator<< exists into a string. Mostly
 * useful for logging data via spdlog which for instance cannot log containers
 *        per default.
 *
 * @tparam T automatically determined
 * @param t object to be turned into a string to be logged
 * @return std::string string representation provided by std::stringstream
 */
template < typename T >
std::string
str(T&& t)
{
    std::stringstream s;
    s << t;
    return s.str();
}


}
}
#endif