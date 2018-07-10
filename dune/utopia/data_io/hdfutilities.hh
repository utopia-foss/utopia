/**
 * @brief This file provides metafunctions for automatically determining the
 *        nature of a C/C++ type at compile time (container or not, string or
 * not), and getting the base type of pointer and cv-qualified types.
 * @file hdfutilities.hh
 */
#ifndef HDFUTILITIES_HH
#define HDFUTILITIES_HH

#include <array>
#include <string>
#include <type_traits>
// Functions for determining if a type is an STL-container are provided here.
// This is used if we wish to make hdf5 types for storing such data in an
// hdf5 dataset.
namespace Utopia
{
namespace DataIO
{
/**
 * @brief Helper function for removing pointer qualifiers from a type recursivly
 *        - recursion base case which provides a type equal to T
 * @tparam T
 * @tparam 0
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
 * @tparam T
 * @tparam 0
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
 * @brief Oveload of 'remove_pointer' metafunction for array types (stack allocated)
 *
 * @tparam T
 */
template <typename T>
struct remove_pointer<T, std::enable_if_t<std::is_array_v<T>, std::void_t<>>>
{
    using type = typename remove_pointer<std::remove_all_extents_t<T>>::type;
};

// remove qualifiers. FIXME: this is not optimal currently, because it does not
// work recursivly

/**
 * @brief Function for removing the qualifiers from
 *
 * @tparam T
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
 *        it is either std::string, const char* or char* - prototype
 *
 * @tparam T
 */
template <typename T>
struct is_string_helper : public std::false_type
{
};

/**
 * @brief Specialization of 'is_string_helper' for std::string
 *
 * @tparam
 */
template <>
struct is_string_helper<std::string> : public std::true_type
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
 * @brief Metafunction for checking if a type is a containertype, which does
 *        not include string types
 *
 * @tparam T
 */
template <typename T>
struct is_container<T, std::void_t<typename remove_qualifier_t<T>::iterator, std::enable_if_t<!is_string_v<T>, int>>>
    : public std::true_type
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
 * @brief Prototype for is_array_like
 *
 * @tparam T
 * @tparam std::void_t<>
 */
template <typename T, typename U = std::void_t<>>
struct is_array_like : std::false_type
{
};

/**
 * @brief Metafunction for checking if a container is a std::array
 *        by checking if std::tuple_size can be applied to it.
 *
 *
 * @tparam T
 */
template <typename T>
struct is_array_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type
{
};

/**
 * @brief Shorthand for is_array_like
 *
 * @tparam T
 */
template <typename T>
inline constexpr bool is_array_like_v = is_array_like<T>::value;

} // namespace DataIO
} // namespace Utopia
#endif
