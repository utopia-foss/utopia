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
// FIXME Using tuple_size here is not optimal, but it gives a unique
//       representation of the tuple
// See: https://ts-gitlab.iup.uni-heidelberg.de/utopia/utopia/merge_requests/109/diffs#note_11774

/**
 * @brief Shorthand for is_array_like
 *
 * @tparam T
 */
template <typename T>
inline constexpr bool is_array_like_v = is_array_like<T>::value;

/**
 * @brief Checks equality of two containers: equal size and equal elements
 *
 * @tparam Container
 * @tparam T
 * @param a
 * @param b
 * @return true
 * @return false
 */
template <template <typename...> class Container, typename T>
bool operator==(Container<T>& lhs, Container<T>& rhs)
{
    if (lhs.size() == rhs.size())
    {
        return false;
    }
    else
    {
        auto lhs_begin = lhs.begin();
        auto rhs_begin = rhs.begin();
        if (std::is_floating_point<T>::value)
        {
            for (; lhs_begin != lhs.end(); ++lhs_begin, ++rhs_begin)
            {
                if (std::abs((*lhs_begin - *rhs_begin) / max(*lhs_begin, *rhs_begin)) < 1e-16)
                {
                    return false;
                }
            }
        }
        else
        {
            for (; lhs_begin != lhs.end(); ++lhs_begin, ++rhs_begin)
            {
                if (*lhs_begin != *rhs_begin)
                {
                    return false;
                }
            }
        }
        return true;
    }
}

/**
 * @brief find_rank recursion base case: returns zero if T is no container or
 * pointer
 *
 * @tparam T Type to get rank for
 * @return constexpr std::size_t 0
 */
template <typename T, std::enable_if_t<!is_container_v<T> and !std::is_pointer_v<T>, int> = 0>
inline constexpr std::size_t find_rank()
{
    return 0;
}

/**
 * @brief Get the rank of a pointer or nested container:
 *        T = double*** -> find_rank<T>() = 3
 *
 *
 * @tparam T Type to get rank for
 * @return constexpr std::size_t the dimension of the object
 */
template <typename T, std::enable_if_t<is_container_v<T>, int> = 0>
inline constexpr std::size_t find_rank()
{
    return 1 + find_rank<typename T::value_type>();
}

/**
 * @brief Get the rank of a pointer or nested container:
 *        T = double*** -> find_rank<T>() = 3
 *
 * @tparam T Type to get rank for
 * @return constexpr std::size_t
 */
template <typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
inline constexpr std::size_t find_rank()
{
    return 1 + find_rank<std::remove_pointer_t<T>>();
}

/**
 * @brief shorthand for find_rank
 *
 * @tparam T Type to get rank for
 */
template <typename T>
inline constexpr std::size_t find_rank_v = find_rank<T>();

/**
 * @brief Base function for recursion for finding the sizes of a nested container
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @param loc pointer to a random access container/pointer where the size of the
 *            current dimension should be written to.
 */
template <typename T, std::enable_if_t<!is_container_v<typename T::value_type>, int> = 0>
inline void find_sizes(T& object, std::size_t* loc)
{
    *loc = object.size();
}

/**
 * @brief Function for recursivly finding the sizes of nested containers
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @param loc pointer to a random access container/pointer where the size of the
 *            current dimension should be written to.
 */
template <typename T, std::enable_if_t<is_container_v<T> and is_container_v<typename T::value_type>, int> = 0>
inline void find_sizes(T& object, std::size_t* loc)
{
    *loc = object.size();
    find_sizes(object[0], loc + 1);
}

/**
 * @brief Get the container properties as a tuple of (rank, vector of sizes for each dimension).
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @return auto  tuple of (rank, vector of sizes for each dimension)
 */
template <typename T>
auto container_properties(T& object)
{
    auto r = find_rank_v<T>;
    std::vector<std::size_t> sizes(r);
    find_sizes(object, sizes.data());
    return std::make_tuple(r, sizes);
}

} // namespace DataIO
} // namespace Utopia
#endif
