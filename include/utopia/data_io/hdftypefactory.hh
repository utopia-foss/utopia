/**
 * @brief This file provides a class which is responsible for the automatic
 *        conversion between C/C++ types and HDF5 type identifiers.
 *
 * @file hdftypefactory.hh
 */
#ifndef UTOPIA_DATAIO_HDFTYPEFACTORY_HH
#define UTOPIA_DATAIO_HDFTYPEFACTORY_HH

#include <iostream>
#include <variant>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <utopia/core/utils.hh>

namespace Utopia
{
namespace DataIO
{

/*!
 * \addtogroup DataIO
 * \{
 */

/*!
 * \addtogroup HDF5
 * \{
 */

/**
 * @brief Class which handles the conversion of C-types into hdf5types.
 *
 *
 */
class HDFTypeFactory
{
  private:
    template < typename T >
    hid_t static inline __get_type__()
    {
        return 0;
    }

  public:
    // typedef for variant type to store attributes in
    // all supported types are assembled into one variant here
    using Variant = std::variant< float,
                                  double,
                                  long double,
                                  int,
                                  short int,
                                  long int,
                                  long long int,
                                  unsigned int,
                                  unsigned short int,
                                  std::size_t,
                                  unsigned long long,
                                  bool,
                                  char,
                                  std::vector< float >,
                                  std::vector< double >,
                                  std::vector< long double >,
                                  std::vector< int >,
                                  std::vector< short int >,
                                  std::vector< long int >,
                                  std::vector< long long int >,
                                  std::vector< unsigned int >,
                                  std::vector< unsigned short int >,
                                  std::vector< std::size_t >,
                                  std::vector< unsigned long long >,
                                  // std::vector<bool>,
                                  std::vector< char >,
                                  std::vector< std::string >,
                                  std::string,
                                  const char* >;
    /**
     * @brief returns a HDF5 type from a given C++ primitive type
     *
     * @param[in]  size  The size
     *
     * @tparam     T     type to convert ot a hdf5 type, for instance 'int'
     *                   or 'std::vector<std::list<double>>'
     *
     * @return     hid_t HDF5 library type corresponding to the given c++ type.
     */
    template < typename T >
    static inline hid_t
    type([[maybe_unused]] std::size_t size = 0)
    {
        // include const char* which is a  c-string
        if constexpr (Utils::is_container_v< T >)
        {
            if (size == 0)
            {
                return H5Tvlen_create(__get_type__< typename T::value_type >());
            }
            else
            {
                hsize_t dim[1] = { size };
                hid_t   type   = H5Tarray_create(
                    __get_type__< typename T::value_type >(), 1, dim);

                return type;
            }
        }
        else if constexpr (Utils::is_string_v< T >)
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
            return __get_type__< T >();
        }
    }
};

// bunch of overloads of __get_type__ for getting hdf5 types for different c++
// types
template <>
hid_t
HDFTypeFactory::__get_type__< float >()
{
    return H5T_NATIVE_FLOAT;
}
template <>
hid_t
HDFTypeFactory::__get_type__< double >()
{
    return H5T_NATIVE_DOUBLE;
}
template <>
hid_t
HDFTypeFactory::__get_type__< long double >()
{
    return H5T_NATIVE_LDOUBLE;
}
template <>
hid_t
HDFTypeFactory::__get_type__< int >()
{
    return H5T_NATIVE_INT;
}

template <>
hid_t
HDFTypeFactory::__get_type__< short int >()
{
    return H5T_NATIVE_SHORT;
}
template <>
hid_t
HDFTypeFactory::__get_type__< long int >()
{
    return H5T_NATIVE_LONG;
}
template <>
hid_t
HDFTypeFactory::__get_type__< long long int >()
{
    return H5T_NATIVE_LLONG;
}
template <>
hid_t
HDFTypeFactory::__get_type__< unsigned int >()
{
    return H5T_NATIVE_UINT;
}
template <>
hid_t
HDFTypeFactory::__get_type__< unsigned short int >()
{
    return H5T_NATIVE_UINT16;
}
template <>
hid_t
HDFTypeFactory::__get_type__< std::size_t >()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t
HDFTypeFactory::__get_type__< unsigned long long >()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t
HDFTypeFactory::__get_type__< bool >()
{
    return H5T_NATIVE_HBOOL;
}
template <>
hid_t
HDFTypeFactory::__get_type__< char >()
{
    return H5T_NATIVE_CHAR;
}
/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif
