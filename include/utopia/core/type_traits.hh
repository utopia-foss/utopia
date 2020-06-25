
#ifndef UTOPIA_CORE_TYPETRAITS_HH
#define UTOPIA_CORE_TYPETRAITS_HH

#include <cmath>
#include <iomanip>
#include <iostream> // needed for operator<< overloads
#include <limits>
#include <type_traits>
#include <vector>
#include <tuple>
#include <utility>
#include <string>
#include <sstream> // needed for making string representations of arb. types
#include <string_view>

#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>

#include <armadillo>

/*
 * Implementation logic: prototype with two template parameters, <T, std::void_t<>> 
 *         holding boolean member that is false, and 
 *         specialization with <T, std::void_t<some, conditions, defining, concept>> 
 *         holding booleaen member which is true.
*/

namespace Utopia
{
namespace Utils
{
/**
 * @brief Helper function for removing pointer qualifiers from a type recursivly
 *        This is the recursion base case.
 * @tparam T  Type to remove the pointer qualifiers from.
 */
template < typename T, typename U = std::void_t<> >
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
template < typename T >
struct remove_pointer<
    T,
    std::enable_if_t< std::is_pointer_v< T >, std::void_t<> > >
{
    using type = typename remove_pointer< std::remove_pointer_t< T > >::type;
};

/**
 * @brief Shorthand for 'typename remove_pointer<T>::type'
 *
 * @tparam T type to remove pointer for
 */
template < typename T >
using remove_pointer_t = typename remove_pointer< T >::type;

/**
 * @brief Oveload of remove_pointer metafunction for array types (stack
 * allocated)
 *
 * @tparam T type to remove qualifier for
 */
template < typename T >
struct remove_pointer< T,
                       std::enable_if_t< std::is_array_v< T >, std::void_t<> > >
{
    using type =
        typename remove_pointer< std::remove_all_extents_t< T > >::type;
};

/**
 * @brief Function for removing the qualifiers from a type T. Qualifiers are
 *        'const', 'volative', pointer, reference or any legal combination
 * thereof.
 *
 * @tparam T Type to remove the qualifiers from.
 */
template < typename T >
struct remove_qualifier
{
    using type =
        std::remove_cv_t< remove_pointer_t< std::remove_reference_t< T > > >;
};

/// Shorthand for 'typename remove_qualifier::value'
template < typename T >
using remove_qualifier_t = typename remove_qualifier< T >::type;

/**
 * @brief Check if a type T is a string-like type, i.e.
 *        std::basic_string, const char*, char*, or basic_string_view
 */
template < typename T >
struct is_string : public std::false_type
{
};

/**
 * @brief Overload of is_string for basic_string
 */
template < typename... Ts >
struct is_string< std::basic_string< Ts... > > : public std::true_type
{
};

/**
 * @brief Overload of is_string for basic_string_view
 */
template< typename... Ts> 
struct is_string<std::basic_string_view<Ts...>> : public std::true_type{};

/**
 * @brief Overload of is_string for pure const char*
 */
template <>
struct is_string< const char* > : public std::true_type
{
};

/**
 * @brief Overload of is_string for pure char*
 */
template <>
struct is_string< char* > : public std::true_type
{
};

/// Shorthand for 'is_string<T>::value'
template < typename T >
constexpr inline bool is_string_v = is_string< T >::value;

/**
 * @brief Check  if a type T is iterable, i.e., if it defines
 * T::iterable.
 *
 * @tparam T type to check
 */
template < typename T, typename = std::void_t<> >
struct is_iterable : std::false_type
{
};

/**
 * @brief Check if a type T is iterable, i.e., if it defines
 * T::iterable.
 *
 * @tparam T type to check
 */
template < typename T >
struct is_iterable< T, std::void_t< typename T::iterator > > : std::true_type
{
};

/// shorthand for is_iterable<T>::value
template < typename T >
inline constexpr bool is_iterable_v = is_iterable< T >::value;

/**
 * @brief Check if a type is a container type, which does
 *        not include string types.
 *
 * @tparam T type to check
 */
template < class T, class U = std::void_t<> >
struct is_container : public std::false_type
{
};

/// Shorthand for 'is_container::value
template < typename T >
inline constexpr bool is_container_v = is_container< T >::value;

/**
 * @brief Check if a type T is a container type.
 *        A container for us is every iterable that is not a string.
 *
 * @tparam T type to check
 */
template < typename T >
struct is_container< T,
                     std::void_t< std::enable_if_t<
                         is_iterable_v< remove_qualifier_t< T > > and
                             not is_string_v< remove_qualifier_t< T > >,
                         int > > > : public std::true_type
{
}; // FIXME: there is something inconsistent in the **usage** of this
   // metafunction that requires the use of `remove_qualifier` in order to keep
   // the tests functional. The cause is not the metafunction itself, but its
   // use. Fix this when possible at some point.

/**
 * @brief Check if a type T is an associative container type,
 *        i.e. a container type T that defines T::key_type and T::key_compare.
 *        Check out
 * https://en.cppreference.com/w/cpp/named_req/AssociativeContainer for more
 * details.
 * @tparam T type to check.
 */
template < typename T, typename = std::void_t<> >
struct is_associative_container : std::false_type
{
};

/**
 * @brief Check if a type T is an associative container type,
 *        i.e. a container type T that defines T::key_type and T::key_compare.
 *        Check out
 * https://en.cppreference.com/w/cpp/named_req/AssociativeContainer for more
 * details.
 * @tparam T type to check.
 */
template < typename T >
struct is_associative_container<
    T,
    std::void_t< std::enable_if_t< is_container_v< T >, int >,
                 typename T::key_type,
                 typename T::key_compare > > : std::true_type
{
};

/// Shorthand for is_associative_container<T>::value
template < typename T >
inline constexpr bool is_associative_container_v =
    is_associative_container< T >::value;

/**
 * @brief Check if a type T is an unordered associative container type,
 *        i.e. a container type T that defines T::key_type, T::value_type and
 * T::hasher. Check out
 * https://en.cppreference.com/w/cpp/named_req/UnorderedAssociativeContainer for
 * more details.
 * @tparam T type to check.
 */
template < typename T, typename = std::void_t<> >
struct is_unordered_associative_container : std::false_type
{
};

/**
 * @brief Check if a type T is an unordered associative container type,
 *        i.e. a container type T that defines T::key_type, T::value_type and
 * T::hasher. Check out
 * https://en.cppreference.com/w/cpp/named_req/UnorderedAssociativeContainer for
 * more details.
 * @tparam T type to check.
 */
template < typename T >
struct is_unordered_associative_container<
    T,
    std::void_t< std::enable_if_t< is_container_v< T >, int >,
                 typename T::key_type,
                 typename T::value_type,
                 typename T::hasher > > : std::true_type
{
};

/// Shorthand for is_unordered_associative_container<T>::value
template < typename T >
inline constexpr bool is_unordered_associative_container_v =
    is_unordered_associative_container< T >::value;

/**
 * @brief Check if a type T is a linear container. A linear
 * container for us is any type T that is a container and neither an associative
 * nor an unordered associative container.
 *
 * @tparam T type to check
 */
template < typename T, typename = std::void_t<> >
struct is_linear_container : std::false_type
{
};

/**
 * @brief Check if a type T is a linear container. A linear
 * container for us is any type T that is a container and neither an associative
 * nor an unordered associative container.
 *
 * @tparam T type to check
 */
template < typename T >
struct is_linear_container<
    T,
    std::void_t<
        std::enable_if_t< not is_associative_container_v< T > and
                              not is_unordered_associative_container_v< T > and
                              is_container_v< T >,
                          int > > > : std::true_type
{
};

/**
 * @brief Shorthand for is_linear_container<T>::value
 *
 * @tparam T type to check
 */
template < typename T >
inline constexpr bool is_linear_container_v = is_linear_container< T >::value;

/**
 * @brief Check if a type T is a random access container, i.e., any container
 * type T that has an iterator tagged with std::random_access_iterator_tag.
 * @tparam T type to check
 */
template < typename T, typename = std::void_t<> >
struct is_random_access_container : std::false_type
{
};

/**
 * @brief Check if a type T is a random access container, i.e., any container
 * type T that has an iterator tagged with std::random_access_iterator_tag.
 * @tparam T type to check
 */
template < typename T >
struct is_random_access_container<
    T,
    std::void_t< std::enable_if_t<
        is_linear_container_v< T > and
        std::is_convertible_v< typename std::iterator_traits<
                                   typename T::iterator >::iterator_category,
                               std::random_access_iterator_tag > > > >
    : std::true_type
{
};

/// shorthand for is_linear_container<T>::value
template < typename T >
inline constexpr bool is_random_access_container_v =
    is_random_access_container< T >::value;

/**
 * @brief Check if a type  T has a vertex descriptor
 *
 * @tparam T type to check
 */
template < typename T, typename U = std::void_t<> >
struct has_vertex_descriptor : std::false_type
{
};

/**
 * @brief Check if a type T has a vertex descriptor
 * @tparam T type to check
 */
template < typename T >
struct has_vertex_descriptor< T, std::void_t< typename T::vertex_descriptor > >
    : std::true_type
{
};

/// Shorthand for has_vertex_descriptor<T>::value
template < typename T >
inline constexpr bool has_vertex_descriptor_v =
    has_vertex_descriptor< T >::value;

/**
 * @brief Check if a type T has a edge descriptor
 *
 * @tparam T type to check
 */
template < typename T, typename U = std::void_t<> >
struct has_edge_descriptor : std::false_type
{
};

/**
 * @brief Check if a type T has an edge descriptor
 * @tparam T type to check
 */
template < typename T >
struct has_edge_descriptor< T, std::void_t< typename T::edge_descriptor > >
    : std::true_type
{
};

/// shothand for has_edge_descriptor<T>::value
template < typename T >
inline constexpr bool has_edge_descriptor_v = has_edge_descriptor< T >::value;

/**
 * @brief Check if some type T is a graph by checking if it
 *          has edge - and vertex_descriptors
 * @tparam T Type to check
 */
template < typename T, typename U = std::void_t<> >
struct is_graph : std::false_type
{
};

/**
 * @brief Check if some type T is a graph by checking if it
 *          has edge_- and vertex_descriptors
 *
 * @tparam T Type to check
 */
template < typename T >
struct is_graph<
    T,
    std::void_t< std::enable_if_t< has_edge_descriptor_v< T > and
                                       has_vertex_descriptor_v< T >,
                                   int > > > : std::true_type
{
};

/// Shorthand for is_graph<T>::value
template < typename T >
inline constexpr bool is_graph_v = is_graph< T >::value;

/**
 * @brief Return the size of a Type T containing other types at compile time.
 *        If no object for which an overload exists is passed,
 *        the get_size::value defaults to 1, indicating a scalar.
 *        If you want to have another type to be usable with get_size,
 *        provide an overload for it as shown by the various overloads
 *        existing for get_size already.
 *
 * @tparam T type to get size for
 */
template < typename T >
struct get_size
    : std::integral_constant<
          std::size_t,
          std::numeric_limits< std::size_t >::max() > // scalar have an
                                                      // unrealistic size ->
                                                      // distinguish from size 1
                                                      // array/tuple...
{
};

/// Overload for std::array
template < typename T, std::size_t N >
struct get_size< std::array< T, N > > : std::integral_constant< std::size_t, N >
{
};

/// Overload for std::tuple
template < typename... Ts >
struct get_size< std::tuple< Ts... > >
    : std::integral_constant< std::size_t, sizeof...(Ts) >
{
};

/// Overload for std::pair
template < typename A, typename B >
struct get_size< std::pair< A, B > > : std::integral_constant< std::size_t, 2 >
{
};

/// Overload for hana tuples
template < typename... Ts >
struct get_size< boost::hana::tuple< Ts... > >
    : std::integral_constant< std::size_t, sizeof...(Ts) >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::mat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::imat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::s32_mat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::u32_mat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::umat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N, arma::uword M >
struct get_size< arma::fmat::fixed< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::vec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::ivec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::s32_vec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::uvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::u32_vec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::fvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::rowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::irowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::s32_rowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::u32_rowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::urowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

/// overload of get_size for armadillo type
template < arma::uword N >
struct get_size< arma::frowvec::fixed< N > >
    : std::integral_constant< std::size_t, N >
{
};

// Shorthand for get_size<T>::value
template < typename T >
inline constexpr std::size_t get_size_v = get_size< T >::value;

/**
 * @brief  Check if a given type is homogeneous and has a size which is a
 *         compile time constant. The most common type which fulfils these
 *         requirements is std::array, hence the name 'is_array_like'. 
 *         For a new type to be considered array_like, this functor has to be 
 *         overloaded together with get_size.
 *
 * @tparam T Some type to check array-likeness for.
 */
template < typename T, typename U = std::void_t<> >
struct is_array_like : std::false_type
{
};

/**
 * @brief  Check if a given type is a container and has a size which is a
 *         compile time constant. The most common type which fulfils these
 *         requirements is std::array, hence the name 'is_array_like'. 
 *         For a new type to be considered array_like, this functor has to be 
 *         overloaded together with get_size.
 *
 * @tparam T Some type to check array-likeness for.
 */
template < typename T >
struct is_array_like<
    T,
    std::void_t<
        std::enable_if_t< get_size_v< T > !=
                              std::numeric_limits< std::size_t >::max() and
                          is_container_v< T > >,
        int > > : std::true_type
{
};

/// Shorthand for is_array_like<T>::value
template < typename T >
inline constexpr bool is_array_like_v = is_array_like< T >::value;

/**
 * @brief Determine if type T is 'tuplelike'.
 *        This is not intended to be used explicitly, refer to
 *        'has_static_size', which serves this purpose.
 *
 * @tparam T Type to test
 * @tparam s size of the type, determined via 'get_size'
 */
template < typename T, std::size_t s >
struct has_static_size_base : std::true_type
{
};

/**
 * @brief Determine if type T is 'tuplelike' - specialization
 * for scalars. This is not intended to be used explicitly, refer to
 *        'has_static_size', which serves this purpose.
 *
 * @tparam T Type to test
 */
template < typename T >
struct has_static_size_base< T, std::numeric_limits< std::size_t >::max() >
    : std::false_type
{
};

/**
 * @brief Determine if type T is tuple_like, i.e., has a compile time constant size
 *        which is smaller than std::numeric_limits< std::size_t >::max() = 
 *        18446744073709551615.
 *        This size is unreasonable and hence used to check invalidity.
 * @tparam T type to test.
 */
template < typename T >
struct has_static_size : has_static_size_base< T, get_size_v< T > >
{
};

/// Shorthand for has_static_size::value
template < typename T >
inline constexpr bool has_static_size_v = has_static_size< T >::value;

/**
 * @brief Check if a type T is callable, i.e., if it has 
 *
 * @tparam T
 * @tparam std::void_t<>
 */
template < typename T, typename X = std::void_t<> >
class is_callable
{
  public:
    static constexpr bool value = false;
};

/**
 * @brief Determines if the type T implements a call operator with arbitrary
 *        signature. 
 *
 * @tparam T Type to check
 */
template < typename T >
class is_callable<
    T,
    std::void_t< std::enable_if_t< std::is_class_v< std::decay_t< T > >, T > > >
{

    struct Fallback
    {
        void
        operator()();
    };

    struct Derived
        : T
        , Fallback
    {
    };

    template < typename U, U >
    struct Check;

    typedef char True[1];  // typedef for an array of size one.
    typedef char False[2]; // typedef for an array of size two.

    template < typename S >
    static constexpr True&
    f(...);

    template < typename S >
    static constexpr False&
    f(Check< void (Fallback::*)(), &S::operator() >*);

  public:
    static constexpr bool value = (sizeof(f< Derived >(nullptr)) == 1);
};

/// Shorthand for is_callable<T>::value
template < typename T >
constexpr bool is_callable_v = is_callable< T >::value;

/**
 * @brief Represent a type that does nothing and represents nothing, hence can
 * be used in metaprogramming whenever no action is desired.
 *
 */
struct Nothing
{
};

}
}

#endif
