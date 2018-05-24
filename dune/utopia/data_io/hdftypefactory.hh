#ifndef HDFTYPEFACTORY_HH
#define HDFTYPEFACTORY_HH
#include "hdfutilities.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>
namespace Utopia
{
namespace DataIO
{
/**
 * @brief Class which handles the conversion of C-types into hdf5types.
 *
 *
 */
class HDFTypeFactory
{
private:
    template <typename T>
    hid_t static inline __get_type__()
    {
        return 0;
    }

public:
    /**
     * @brief struct for getting a plain type from a return type of a function
     *
     * @tparam T
     * @tparam T
     */
    template <typename T, typename U = T>
    struct result_type
    {
        using type = T;
    };

    template <typename T>
    struct result_type<T*>
    {
        // recursion for getting the base type for
        // multidim-pointers
        using type = typename std::remove_all_extents<T>::type;
    };

    template <typename T>
    struct result_type<T&>
    {
        using type = T;
    };

    /**
     * @brief returns a HDF5 type from a given C++ primitive type
     *
     * @tparam T
     * @return hid_t
     */
    template <typename T>
    static inline hid_t type([[maybe_unused]] std::size_t size = 0)
    {
        // include const char* which is a  c-string
        if constexpr (is_container_type<T>::value)
        {
            if (size == 0)
            {
                return H5Tvlen_create(__get_type__<typename T::value_type>());
            }
            else
            {
                hsize_t dim[1] = {size};
                hid_t type =
                    H5Tarray_create(__get_type__<typename T::value_type>(), 1, dim);

                return type;
            }
        }
        else
        {
            if constexpr (is_stringtype<T>::value)
            {
                if (size == 0)
                {
                    hid_t type = H5Tcopy(H5T_C_S1);
                    H5Tset_size(type, H5T_VARIABLE);
                    return type;
                }
                else
                {
                    hid_t type = H5Tcopy(H5T_C_S1);
                    H5Tset_size(type, size);
                    return type;
                }
            }
            else
            {
                return __get_type__<T>();
            }
        }
    }
};

// bunch of overloads of __get_type__ for getting hdf5 types for different c++
// types
template <>
hid_t HDFTypeFactory::__get_type__<float>()
{
    return H5T_NATIVE_FLOAT;
}
template <>
hid_t HDFTypeFactory::__get_type__<double>()
{
    return H5T_NATIVE_DOUBLE;
}
template <>
hid_t HDFTypeFactory::__get_type__<long double>()
{
    return H5T_NATIVE_LDOUBLE;
}
template <>
hid_t HDFTypeFactory::__get_type__<int>()
{
    return H5T_NATIVE_INT;
}

template <>
hid_t HDFTypeFactory::__get_type__<short int>()
{
    return H5T_NATIVE_SHORT;
}
template <>
hid_t HDFTypeFactory::__get_type__<long int>()
{
    return H5T_NATIVE_LONG;
}
template <>
hid_t HDFTypeFactory::__get_type__<long long int>()
{
    return H5T_NATIVE_LLONG;
}
template <>
hid_t HDFTypeFactory::__get_type__<unsigned int>()
{
    return H5T_NATIVE_UINT;
}
template <>
hid_t HDFTypeFactory::__get_type__<unsigned short int>()
{
    return H5T_NATIVE_UINT16;
}
template <>
hid_t HDFTypeFactory::__get_type__<std::size_t>()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t HDFTypeFactory::__get_type__<unsigned long long>()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t HDFTypeFactory::__get_type__<bool>()
{
    return H5T_NATIVE_HBOOL;
}
template <>
hid_t HDFTypeFactory::__get_type__<char>()
{
    return H5T_NATIVE_CHAR;
}

} // namespace DataIO
} // namespace Utopia
#endif