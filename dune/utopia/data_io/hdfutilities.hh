/**
 * @brief This file provides metafunctions for automatically determining the
 *        nature of a C/C++ type at compile time (container or not, string or
 * not), and getting the base type of pointer and cv-qualified types.
 * @file hdfutilities.hh
 */
#ifndef HDFUTILITIES_HH
#define HDFUTILITIES_HH

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
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
 * @brief Get the rank of a pointer or nested container:
 *        T = double*** => find_rank<T>() = 3
 *
 *
 * @tparam T Type to get rank for
 * @return constexpr std::size_t the dimension of the object
 */
template <typename T>
inline constexpr std::size_t find_rank()
{
    if constexpr (is_container_v<T>)
    {
        return 1 + find_rank<typename T::value_type>();
    }

    else if constexpr (std::is_pointer_v<T>)
    {
        return 1 + find_rank<std::remove_pointer_t<T>>();
    }
    else if constexpr (std::is_array_v<T>)
    {
        return 1 + find_rank<std::remove_extent_t<T>>();
    }
    else
    {
        return 0;
    }
}

/**
 * @brief shorthand for find_rank
 *
 * @tparam T Type to get rank for
 */
template <typename T>
inline constexpr std::size_t find_rank_v = find_rank<T>();

/**
 * @brief Base function for recursion for finding the sizes of a nested
 * container
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @param loc pointer to a random access container/pointer where the size of the
 *            current dimension should be written to.
 */
template <typename T>
inline void find_sizes(T&& object, std::size_t* loc)
{
    if constexpr (is_container_v<T> and
                  is_container_v<typename remove_qualifier_t<T>::value_type>)
    {
        *loc = object.size();
        find_sizes(object[0], loc + 1);
    }
    else if constexpr (!is_container_v<typename remove_qualifier_t<T>::value_type>)
    {
        *loc = object.size();
    }
    else if constexpr (std::is_array_v<remove_qualifier_t<T>::value_type>)
    {
        *loc = std::distance(std::begin(object), std::end(object));
    }
    else if constexpr (std::is_pointer_v<remove_qualifier_t<T>::value_type>)
    {
        throw std::invalid_argument(
            "Pure pointer type found in T, supply shape by hand");
    }
    else
    {
        throw std::invalid_argument("Unknown type T supplied to find_sizes");
    }
}

/**
 * @brief Get the container properties as a tuple of (rank, vector of sizes for
 * each dimension).
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @return auto  tuple of (rank, vector of sizes for each dimension)
 */
template <typename T>
auto container_properties(T&& object)
{
    auto r = find_rank_v<remove_qualifier_t<T>>;
    std::vector<std::size_t> sizes(r);
    find_sizes(std::forward<T&&>(object), sizes.data());
    return std::make_tuple(r, sizes);
}

/**
 * @brief Function for recursivly finding the sizes of nested containers
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @param loc pointer to a random access container/pointer where the size of the
 *            current dimension should be written to.
 */
template <typename T>
inline void find_sizes(const T& object, std::size_t* loc)
{
    if constexpr (is_container_v<T> and
                  is_container_v<typename remove_qualifier_t<T>::value_type>)
    {
        *loc = object.size();
        find_sizes(object[0], loc + 1);
    }
    else if constexpr (!is_container_v<typename remove_qualifier_t<T>::value_type>)
    {
        *loc = object.size();
    }
    else if constexpr (std::is_array_v<remove_qualifier_t<T>::value_type>)
    {
        *loc = std::distance(std::begin(object), std::end(object));
    }
    else if constexpr (std::is_pointer_v<remove_qualifier_t<T>::value_type>)
    {
        throw std::invalid_argument(
            "Pure pointer type found in T, supply shape by hand");
    }
    else
    {
        throw std::invalid_argument("Unknown type T supplied to find_sizes");
    }
}

/**
 * @brief Get the container properties as a tuple of (rank, vector of sizes for
 * each dimension).
 *
 * @tparam T automatically determined
 * @param object (nested) container for which the sizes are to be determined
 * @return auto  tuple of (rank, vector of sizes for each dimension)
 */
template <typename T>
auto container_properties(const T& object)
{
    auto r = find_rank_v<remove_qualifier_t<T>>;
    std::vector<std::size_t> sizes(r);
    find_sizes(object, sizes.data());
    return std::make_tuple(r, sizes);
}

/**
 * @brief Compares two containers for equality by checking their size and elements for equality
 *
 * @tparam Container  automatically determined template parameters
 * @tparam T  automatically determined template parameters
 * @param lhs first container to test
 * @param rhs second container to test
 * @return true lhs==rhs
 * @return false lhs!=rhs, i.e. at least one element is unequal or size is unequal
 */
template <template <typename...> class Container1,
          template <typename...> class Container2,
          class T,
          std::enable_if_t<is_container_v<Container1<T>> and is_container_v<Container2<T>>, int> = 0>
inline bool operator==(const Container1<T>& lhs, const Container2<T>& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    else
    {
        auto lb = lhs.begin();
        auto rb = rhs.begin();
        if constexpr (std::is_floating_point_v<T>)
        {
            for (; lb != lhs.end(); ++lb, ++rb)
            {
                if (std::fabs(*lb - *rb) / (std::fabs(std::max(*lb, *rb))) > 1e-16)
                {
                    return false;
                }
            }
        }
        else
        {
            for (; lb != lhs.end(); ++lb, ++rb)
            {
                if (*lb != *rb)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * @brief Compares two containers for equality by checking their size and elements for equality
 *
 * @tparam Container  automatically determined template parameters
 * @tparam T  automatically determined template parameters
 * @param lhs first container to test
 * @param rhs second container to test
 * @return true lhs==rhs
 * @return false lhs!=rhs, i.e. at least one element is unequal or size is unequal
 */
template <template <typename...> class Container1,
          template <typename...> class Container2,
          class T,
          std::enable_if_t<is_container_v<Container1<T>> and is_container_v<Container2<T>>, int> = 0>
inline bool operator==(Container1<T>&& lhs, Container2<T>&& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    else
    {
        auto lb = lhs.begin();
        auto rb = rhs.begin();
        if constexpr (std::is_floating_point_v<T>)
        {
            for (; lb != lhs.end(); ++lb, ++rb)
            {
                if (std::fabs(*lb - *rb) / (std::fabs(std::max(*lb, *rb))) > 1e-16)
                {
                    return false;
                }
            }
        }
        else
        {
            for (; lb != lhs.end(); ++lb, ++rb)
            {
                if (*lb != *rb)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * @brief Output operator for std::arrays
 *
 * @tparam T
 * @tparam N
 * @param os
 * @param v
 * @return std::ostream&
 */
template <class T, std::size_t N>
inline std::ostream& operator<<(std::ostream& os, const std::array<T, N>& v)
{
    os << "[";
    for (auto ii = v.begin(); ii != v.end(); ++ii)
    {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

/**
 * @brief Output operator for std::arrays
 *
 * @tparam T
 * @tparam N
 * @param os
 * @param v
 * @return std::ostream&
 */
template <class T, std::size_t N>
inline std::ostream& operator<<(std::ostream& os, std::array<T, N>&& v)
{
    os << "[";
    for (auto ii = v.begin(); ii != v.end(); ++ii)
    {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

/**
 * @brief Output operator for containers
 *
 * @tparam T automatically determined
 * @param os outstream to use
 * @param v container to put out
 * @return std::ostream& outstream used
 */
template <template <typename...> class Container, class T, std::enable_if_t<!is_string_v<Container<T>>, int> = 0>
inline std::ostream& operator<<(std::ostream& os, const Container<T>& v)
{
    os << "[";
    for (typename Container<T>::const_iterator ii = v.begin(); ii != v.end(); ++ii)
    {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

/**
 * @brief Output operator for containers
 *
 * @tparam T automatically determined
 * @param os outstream to use
 * @param v container to put out
 * @return std::ostream& outstream used
 */
template <template <typename...> class Container, class T, std::enable_if_t<!is_string_v<Container<T>>, int> = 0>
inline std::ostream& operator<<(std::ostream& os, Container<T>&& v)
{
    os << "[";
    for (auto ii = v.begin(); ii != v.end(); ++ii)
    {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

} // namespace DataIO
} // namespace Utopia
#endif
