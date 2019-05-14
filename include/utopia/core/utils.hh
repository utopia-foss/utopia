#ifndef UTOPIA_CORE_UTILS_HH
#define UTOPIA_CORE_UTILS_HH

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include <tuple>
#include <utility>
#include <string>

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/for_each.hpp>

namespace Utopia
{
namespace Utils
{
/**
 * @brief Helper function for removing pointer qualifiers from a type recursivly
 *        This is the recursion base case.
 * @tparam T  Type to remove the pointer qualifiers from.
 */
template <typename T, typename U = std::void_t<>>
struct remove_pointer
{
    using type = T;
};

/**
 * @brief Helper function for removing pointer qualifiers from a type recursivly
 *        Provides a member type definition called 'type' which is equal to T
 *        if the first template argument is of type T* or T** or T***...
 * @tparam T  Type to remove the pointer qualifiers from.
 */
template <typename T>
struct remove_pointer<T, std::enable_if_t<std::is_pointer_v<T>, std::void_t<>>>
{
    using type = typename remove_pointer<std::remove_pointer_t<T>>::type;
};

/**
 * @brief Shorthand for 'typename remove_pointer<T>::type'
 *
 * @tparam T
 */
template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

/**
 * @brief Oveload of remove_pointer metafunction for array types (stack allocated)
 *
 * @tparam T
 */
template <typename T>
struct remove_pointer<T, std::enable_if_t<std::is_array_v<T>, std::void_t<>>>
{
    using type = typename remove_pointer<std::remove_all_extents_t<T>>::type;
};

/**
 * @brief Function for removing the qualifiers from a type T. Qualifiers are
 *        'const', 'volative', pointer, reference or any legal combination thereof.
 *
 * @tparam T Type to remove the qualifiers from.
 */
template <typename T>
struct remove_qualifier
{
    using type = std::remove_cv_t<remove_pointer_t<std::remove_reference_t<T>>>;
};

/**
 * @brief Shorthand for 'typename remove_qualifier::value'
 *
 * @tparam T
 */
template <typename T>
using remove_qualifier_t = typename remove_qualifier<T>::type;

/**
 * @brief Meta-function for checking if a type is a stringtype, that is
 *        it is either std::basic_string, const char* or char* - prototype
 *
 * @tparam T
 */
template <typename T>
struct is_string_helper : public std::false_type
{
};

/**
 * @brief Specialization of 'is_string_helper' for std::basic_string
 *
 * @tparam
 */
template <typename T>
struct is_string_helper<std::basic_string<T>> : public std::true_type
{
};

/**
 * @brief Metafunction for determining if a type is a string-like type, i.e.
 *        std::string, const char*, char*
 * @tparam T
 */
template <typename T>
struct is_string : public is_string_helper<remove_qualifier_t<T>>
{
};

/**
 * @brief Overload of is_string for pure const char*, which would loose its
 *        pointer qualifier if this was not provided
 * @tparam
 */
template <>
struct is_string<const char*> : public std::true_type // is_string_helper<const char*>
{
};

/**
 * @brief Overload of is_string for pure const char*, which would loose its
 *        pointer qualifier if this was not provided
 * @tparam
 */
template <>
struct is_string<char*> : public std::true_type // public is_string_helper<char*>
{
};

/**
 * @brief Shorthand for 'is_string<T>::value'
 *

 */
template <typename T>
constexpr inline bool is_string_v = is_string<T>::value;

/*
README:
The following are metafunctions to determine type classifications.
The logic is like this:
the prototype of, say, is_container has two template args, the second of which is
defaulted to std::void_t<>. It inherits from std::false_type. Then overloads for
based on conditions to be fullfilled or for certain types are supplied ,
are casted to std::void_t<Whatever> and inherit from std::true_type.
As always in such cases, this relies on SFINAE. When a type is given to one
of the metafunctions, if the tested condition is true or an overload for a
given type is found, the second template arg becomes std::void_t<Whatever>
and the respective metafunction, inheriting from std::true_type is instantiated.
The type test, is_container for instance, evaluates to true.
Otherwise, the second template parameter is std::void_t<>, and the
prototype  is used, which inherits from std::false_type.
*/

/**
 * @brief Metafunction for checking if a type is a containertype, which does
 *        not include string types - prototype
 *
 * @tparam T
 * @tparam std::void_t<>
 */
template <class T, class U = std::void_t<>>
struct is_container : public std::false_type
{
};

/**
 * @brief Shorthand for 'is_container::value
 *
 * @tparam T
 */
template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;

/**
 * @brief Metafunction for checking if a type is a containertype..
 *        A container is everything which has an internal type 'iterator',
 *        but which is not a string.
 *
 * @tparam T
 */
template <typename T>
struct is_container<T, std::void_t<typename remove_qualifier_t<T>::iterator, std::enable_if_t<!is_string_v<T>, int>>>
    : public std::true_type
{
};

/**
 * @brief Prototype for get_size functor for constexpr size containers.
 *        Returns the size of the container at compile time.
 *        If no object for which an overload exists is passed,
 *        the get_size::value defaults to 1, indicating a scalar.
 *        If you want to have another type be usable with get_size,
 *        provide an overload for it as shown by the various overloads
 *        existing for get_size already.
 *
 * @tparam T
 */
template <typename T>
struct get_size
    : std::integral_constant<std::size_t, std::numeric_limits<std::size_t>::max()> // scalar have an unrealistic size -> distinguish from size 1 array/tuple...
{
};

/**
 * @brief get_size overload for std::array
 *
 * @tparam T
 * @tparam N
 */
template <typename T, std::size_t N>
struct get_size<std::array<T, N>> : std::integral_constant<std::size_t, N>
{
};

/**
 * @brief Overload for tuples
 *
 * @tparam Ts
 */
template <typename... Ts>
struct get_size<std::tuple<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)>
{
};

/**
 * @brief Overload for pairs
 *
 * @tparam Ts
 */
template <typename A, typename B>
struct get_size<std::pair<A, B>> : std::integral_constant<std::size_t, 2>
{
};

/**
 * @brief shorthand for get_size
 *
 * @tparam T type to get size of. Only works for types with constant expr size
 *         given as a template parameter
 */
template <typename T>
inline constexpr std::size_t get_size_v = get_size<T>::value;

/**
 * @brief  Check if a given type is homogeneous and has a size which is a
 *         compile time constant. The most common type which fulfils that is
 *         std::array, hence the name 'is_array_like'. For a new type to be
 *         considered array_like, this functor has to be overloaded together
 *         with get_size.
 *
 * @tparam T Some type to check array-likeness for
 */
template <typename T, typename U = std::void_t<>>
struct is_array_like : std::false_type
{
};

/**
 * @brief Specialization of is_array_like for std::array.
 *
 * @tparam T Elementtype of the array
 * @tparam N Number of elements the array type stores.
 */
template <typename T, std::size_t N>
struct is_array_like<std::array<T, N>, std::void_t<decltype(get_size<std::array<T, N>>::value)>>
    : std::true_type
{
};

/**
 * @brief Shorthand for is_array_like::value.
 *
 * @tparam T Type to check array_likeness for
 */
template <typename T>
inline constexpr bool is_array_like_v = is_array_like<T>::value;

/**
 * @brief Prototype for determining if type T is 'tuplelike'.
 *        This is not intended to be used explicitly, refer to 
 *        'is_tuple_like', which serves this purpose.
 *
 * @tparam T Type to test
 * @tparam s size of the type, determined via 'get_size'
 */
template <typename T, std::size_t s>
struct is_tuple_like_base : std::true_type
{
};

/**
 * @brief Prototype for determining if type T is 'tuplelike' - specialization for scalars.
 *        This is not intended to be used explicitly, refer to 
 *        'is_tuple_like', which serves this purpose.
 *
 * @tparam T Type to test
 */
template <typename T>
struct is_tuple_like_base<T, std::numeric_limits<std::size_t>::max()> : std::false_type
{
};

/**
 * @brief Determine if type T is tuple_like, i.e. usable with std::get.
 * 
 * @tparam T 
 */
template <typename T>
struct is_tuple_like : is_tuple_like_base<T, get_size_v<T>>
{
};


/**
 * @brief Shorthand for is_tuple_like::value
 *
 * @tparam T  Type to test
 */
template <typename... Ts>
inline constexpr bool is_tuple_like_v = is_tuple_like<Ts...>::value;


/**
 * @brief Determines if the type T implements a call operator with arbitrary 
 *        signature
 * 
 * @tparam T Type to check for if it is callable
 */
template<typename T>
class is_callable_object
{

    struct Fallback
    {
        void operator()();
    };

    struct Derived
      : T
      , Fallback
    {};

    template<typename U, U>
    struct Check;

    typedef char True[1];  // typedef for an array of size one.
    typedef char False[2]; // typedef for an array of size two.

    template<typename S>
    static constexpr True& f(...);

    template<typename S>
    static constexpr False& f(Check<void (Fallback::*)(), &S::operator()>*);

  public:
    static constexpr bool value = (sizeof(f<Derived>(nullptr)) == 1);
};

/**
 * @brief Shorthand for is_callable_object<T>::value
 *
 * @tparam T
 */
template<typename T>
constexpr bool is_callable_object_v = is_callable_object<T>::value;

/**
 * @brief 
 * 
 * @tparam T 
 * @tparam U 
 * @param out 
 * @param pair 
 * @return std::ostream& 
 */
template<typename T, typename U> 
std::ostream& operator<<(std::ostream& out, const std::pair<T, U>& pair){
    out << "(" << pair.first << ", " << pair.second << ")";
    return out;
}


/**
 * Output the content of a container to the stream 'out' in the format
 * '[x1, x2, x3, ...]' where the x_{i} are the elements the container holds.
 * This does not(!) take care of outputting the elements, in the sense that
 * an output operator for the latter has to be preexisting.
 * @param container Container object to output.
 *
 */
template <class T>
std::enable_if_t<is_container_v<T>, std::ostream&> operator<<(std::ostream& out,
                                                              const T& container)
{
    if (container.size() == 0)
    {
        out << "[]";
    }
    else
    {
        out << std::setprecision(16) << "[";

        for (auto it = container.begin();
             it != std::next(std::begin(container), container.size() - 1); ++it)
        {
            out << *it << ", "; // becomes a recursive call in case of nested containers
        }
        out << *std::next(std::begin(container), container.size() - 1) << "]";
    }
    return out;
}

/**
 * @brief Output container to stream 'out'- specialization for vector of booleans
 *
 * @tparam T
 * @tparam 0
 * @param out
 * @param container
 * @return std::ostream&
 */
std::ostream& operator<<(std::ostream& out, const std::vector<bool>& container)
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
 * @brief Output associative containers to an std::ostream
 *
 * @tparam MapType Some type representing an associative container, automatically determined.
 * @tparam Key Key type of the container, automatically determined.
 * @tparam Value Mapped type of the container, automatically determined.
 * @tparam Args Additional template args, automatically determined.
 * @param out outstream to use for output
 * @param map Map like type, has to define key_type, map_type and have pair<const Key, Value> as value_type
 * @return std::ostream Used outstream
 */
template <template <typename, typename, typename...> class MapType, typename Key, typename Value, typename... Args>
std::enable_if_t<std::is_same_v<typename MapType<Key, Value, Args...>::key_type, Key> and
                     std::is_same_v<typename MapType<Key, Value, Args...>::mapped_type, Value> and
                     std::is_same_v<typename MapType<Key, Value, Args...>::value_type, std::pair<const Key, Value>>,
                 std::ostream&>
operator<<(std::ostream& out, const MapType<Key, Value, Args...>& map)
{
    if(map.size() == 0){
        out << "[]";
    }
    else{
        out << "[";
        for(auto it = map.begin(); it != std::next(map.begin(), map.size() -1); ++it){
            out << *it << ", ";
        }
        out << *std::next(map.begin(), map.size() - 1) << "]";
    }
    return out;
}



/// Report a tuple to an outstream
/** This works by piping the single elements inside the tuple to the stream,
 *  and therefor requires `operator<<` for the underlying types to be defined.
 *
 *  For tuple values `a`, `b`, ..., the output will look like
 *  \verbatim (a, b, ...) \endverbatim
 */
template<typename... Types>
std::ostream& operator<< (std::ostream& ostr, std::tuple<Types...> tuple)
{
    std::string val_str("(");

    auto report_val = [&val_str](auto&& val){
        val_str += std::to_string(val) + ", ";
    };
    boost::hana::for_each(tuple, report_val);

    // remove final comma
    val_str.erase(val_str.end() - 2);
    val_str += ")";

    ostr << val_str;
    return ostr;
}


}
}


#endif
