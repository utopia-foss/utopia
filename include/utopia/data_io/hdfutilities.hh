/**
 * @brief This file provides metafunctions for automatically determining the
 *        nature of a C/C++ types at compile time (container or not, string or
 *         not, fixed-size array ), and getting the base type of pointer and
 *         cv-qualified types.
 * @file hdfutilities.hh
 */
#ifndef UTOPIA_DATAIO_HDFUTILITIES_HH
#define UTOPIA_DATAIO_HDFUTILITIES_HH

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <hdf5.h>

// Functions for determining if a type is an STL-container are provided here.
// This is used if we wish to make hdf5 types for storing such data in an
// hdf5 dataset.
namespace Utopia {
namespace DataIO {

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

/*
README: get_size supplies the condition which is tested for a type to be
        considered array_like. It establishes that the type in question
        has compile time constant size.
*/
/**
 * @brief Prototype for get_size functor for constexpr size containers
 *
 * @tparam T
 */
template <typename T>
struct get_size;
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

/*
README: is_array_like checks if a get_size version for the given type T exists.
        Hence, if a new type shall be considered array_like, get_size has to
        be overloaded for it.
*/
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
 * @brief Specialization for std::array
 *
 * @tparam T element type
 * @tparam N number of elements to store
 */
template <typename T, std::size_t N>
struct is_array_like<std::array<T, N>, std::void_t<decltype(get_size<std::array<T, N>>::value)>>
    : std::true_type
{
};

/**
 * @brief Shorthand for is_array_like
 *
 * @tparam T
 */
template <typename T>
inline constexpr bool is_array_like_v = is_array_like<T>::value;

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
    if constexpr (std::is_same_v<Container<T>, std::vector<bool>>)
    {
        os << "[";
        for (std::size_t i = 0; i < v.size() - 1; ++i)
        {
            os << v[i] << ",";
        }
        os << v.back() << " ]";
    }
    else
    {
        os << "[";
        for (typename Container<T>::const_iterator ii = v.begin();
             ii != v.begin() + v.size(); ++ii)
        {
            os << *ii << ",";
        }
        os << v.back() << " ]";
    }
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
    if constexpr (std::is_same_v<Container<T>, std::vector<bool>>)
    {
        os << "[";
        for (std::size_t i = 0; i < v.size() - 1; ++i)
        {
            os << v[i] << ",";
        }
        os << v.back() << " ]";
    }
    else
    {
        os << "[";
        for (typename Container<T>::const_iterator ii = v.begin();
             ii != v.begin() + v.size(); ++ii)
        {
            os << *ii << ",";
        }
        os << v.back() << " ]";
    }
    return os;
}

/**
 * @brief Check for validity of a hdf5 htri_t type or similar
 * @param [in] valid parameter to check
 * @param [in] object name Name of object to be referenced in thrown exceptions
              if valid <= 0
 * @return  The valid argument for further use
 * @details This function is necessary because for instance H5Iis_valid does not
 *           return a boolean (non existant in C), but a value which is > 0 if
 *           everything is fine, < 0 if some error occurred during checking and
 *           0 if the object to check is invalid. This has to be taken into account
 *           in order to be able to track bugs or wrong usage properly.
 *           See here: https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.6/hdf5-1.6.7/src/unpacked/src/H5public.h
 *           which yields the following snippet:
 *          // Boolean type.  Successful return values are zero (false) or positive
 *          // (true). The typical true value is 1 but don't bet on it.  Boolean
 *          // functions cannot fail.  Functions that return `htri_t' however return zero
 *          // (false), positive (true), or negative (failure). The proper way to test
 *          // for truth from a htri_t function is:
 *
 * 	         if ((retval = H5Tcommitted(type))>0) {
 *	             printf("data type is committed\n");
 *	         } else if (!retval) {
 * 	                printf("data type is not committed\n");
 *	         } else {
 * 	             printf("error determining whether data type is committed\n");
 *	         }
 */
template <typename T>
bool check_validity(T valid, std::string object_name)
{
    if (valid > 0)
    {
        return true;
    }

    else if (valid < 0)
    {
        throw std::runtime_error("Object " + object_name + ": " +
                                 "Error in validity check");
        return false;
    }
    else
    {
        return false;
    }
}

/// Checks iteratively if each segment of a path exists
/** \param loc_id  Identifier of the file or group to query.
  * \param path    The path of the link to check. This can be a relative or
  *                an absolute path, but (as with H5Lexists) it can NOT use
  *                the ``../`` syntax to go to the parent object. For such
  *                cases, an absolute path needs to be given.
  * \param link_property_list  Link access property list identifier.
  *
  * \note This happens according to the recommended way to use H5Lexists, which
  *       only checks for the existence of the final path segment.
  *
  * \detail For example, a path /foo/bar/baz is checked in the following way:
  *             /
  *             /foo
  *             /foo/bar
  *             /foo/bar/baz
  *         If any of the segements does not yet exist, the function returns
  */
htri_t path_exists(hid_t loc_id,
                   std::string path,
                   hid_t link_property_list = H5P_DEFAULT) {
    // Position of the segment cursor; all characters before are checked
    // For absolute paths, search always needs to start behind index 1
    std::size_t seg_pos = (path.find("/") == 0) ? 1 : 0;

    // A buffer for the return value of H5Lexists
    htri_t rv;

    // Go over all segments until the whole string is
    while (seg_pos != std::string::npos) {
        // Find the position of the next "/", strictly after the current
        // cursor position
        seg_pos = path.find("/", seg_pos+1);

        // Check for existence of the subpath. If seg_pos is string::npos,
        // the substring is the full path and this is the last loop iteration.
        rv = H5Lexists(loc_id, path.substr(0, seg_pos).c_str(),
                       link_property_list);

        // If this segment does not exists, need to return already
        if (rv <= 0) {
            return rv;
        }
    }

    // Checked the full path. Can return the last return value now:
    return rv;
}


} // namespace DataIO
} // namespace Utopia
#endif
