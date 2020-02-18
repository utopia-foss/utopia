/**
 * @brief This file provides a class which is responsible for the automatic
 *        conversion between C/C++ types and HDF5 type identifiers.
 *
 * @file hdftypefactory.hh
 */
#ifndef UTOPIA_DATAIO_HDFTYPEFACTORY_HH
#define UTOPIA_DATAIO_HDFTYPEFACTORY_HH

#include "utopia/data_io/hdfutilities.hh"
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <iostream>
#include <type_traits>
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

namespace Detail
{

template < typename T >
hid_t inline get_type()
{
    return 0;
}
// bunch of overloads of get_type for getting hdf5 types for different c++
// types
template <>
hid_t
get_type< float >()
{
    return H5T_NATIVE_FLOAT;
}

template <>
hid_t
get_type< double >()
{
    return H5T_NATIVE_DOUBLE;
}
template <>
hid_t
get_type< long double >()
{
    return H5T_NATIVE_LDOUBLE;
}
template <>
hid_t
get_type< int >()
{
    return H5T_NATIVE_INT;
}

template <>
hid_t
get_type< short int >()
{
    return H5T_NATIVE_SHORT;
}
template <>
hid_t
get_type< long int >()
{
    return H5T_NATIVE_LONG;
}
template <>
hid_t
get_type< long long int >()
{
    return H5T_NATIVE_LLONG;
}
template <>
hid_t
get_type< unsigned int >()
{
    return H5T_NATIVE_UINT;
}
template <>
hid_t
get_type< unsigned short int >()
{
    return H5T_NATIVE_UINT16;
}
template <>
hid_t
get_type< std::size_t >()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t
get_type< unsigned long long >()
{
    return H5T_NATIVE_ULLONG;
}

template <>
hid_t
get_type< bool >()
{
    return H5T_NATIVE_HBOOL;
}
template <>
hid_t
get_type< char >()
{
    return H5T_NATIVE_CHAR;
}

}

/**
 * @brief Class which handles the conversion of C-types into hdf5types.
 *
 *
 */
template < typename T >
class HDFTypeFactory
{
  private:
    /// H5 identifier
    hid_t _type = -1;

    /// Indentify if current type is mutable
    /** @note Unfortunately, there is no HDF5-intrinsic way to check this
     */
    bool _mutable = false;

    /// Enumeration telling what class the type belongs to
    H5T_class_t _classid;

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
     * @brief returns a HDF5 type created from a given C++ primitive type
     *
     * @param[in]  size  The size
     *
     * @tparam     T     type to convert ot a hdf5 type, for instance 'int'
     *                   or 'std::vector<std::list<double>>'
     *
     * @return     hid_t HDF5 library type corresponding to the given c++ type.
     */
    inline hid_t
    get_id() const
    {
        return _type;
    }

    /**
     * @brief Get if the type is mutable or not
     *
     * @return true
     * @return false
     */
    inline bool
    is_mutable() const
    {
        return _mutable;
    }

    /**
     * @brief Get the type category of the held type, i.e., scala, string,
     * varlen,...
     *
     * @return auto type category of held type
     */
    inline auto
    category() const
    {
        return _classid;
    }

    /**
     * @brief Size of the type held in bytes
     *
     * @return std::size_t
     */
    inline std::size_t
    size() const
    {
        return H5Tget_size(_type);
    }

    /**
     * @brief Check if the held type identifier is still valid. Primitive types
     * are valid by definition, because we have no control over them and hence
     * they cannot be invalidated
     *
     * @return true
     * @return false
     */
    bool
    is_valid() const
    {
        // this distinction is important because identifiers like
        if (not(_classid == H5T_VLEN or _classid == H5T_ARRAY or
                _classid == H5T_STRING))
        {
            return true;
        }
        else
        {
            return check_validity(H5Iis_valid(_type), "typefactory");
        }
    }

    /**
     * @brief Construct HDFTypeFactory from the given arguments by move,
     * deleted, because apparently incompatible with HDF5 C backend
     *
     */
    HDFTypeFactory(HDFTypeFactory&& other)
    {

        _type          = std::move(other._type);
        _mutable       = std::move(other._mutable);
        _classid       = std::move(other._classid);
        other._type    = -1;
        other._mutable = false;
        other._classid = H5T_NO_CLASS;
    }

    /**
     * @brief Move assign the typefactory
     *
     * @return HDFTypeFactory&
     */
    HDFTypeFactory&
    operator=(HDFTypeFactory&& other)
    {

        _type          = std::move(other._type);
        _mutable       = std::move(other._mutable);
        _classid       = std::move(other._classid);
        other._type    = -1;
        other._mutable = false;
        other._classid = H5T_NO_CLASS;

        return *this;
    }

    /**
     * @brief Construct HDFTypeFactory from the given arguments by copy
     *
     */
    HDFTypeFactory(const HDFTypeFactory& other)
    {

        _type = other._type; // H5Tcopy(other._type);
        if (other._mutable)
        {
            H5Iinc_ref(_type);
        }
        _mutable = true;
        _classid = other._classid;
    }

    /**
     * @brief Copy assign typefactory
     *
     * @return HDFTypeFactory&
     */
    HDFTypeFactory&
    operator=(const HDFTypeFactory& other)
    {

        _type = other._type; // H5Tcopy(other._type);
        if (other._mutable)
        {
            H5Iinc_ref(_type);
        }
        _mutable = true;
        _classid = other._classid;

        return *this;
    }

    /**
     * @brief Destroy the HDFTypeFactory object
     *
     */
    ~HDFTypeFactory()
    {
        // everything that is obtained via H5Tcopy, H5Topen or H5Tcreate
        // needs to be released explicitly
        if (is_valid() and _mutable)
        {
            if (H5Iget_ref(_type) > 1)
            {
                H5Idec_ref(_type);
            }
            else
            {
                H5Tclose(_type);
            }
        }
        _type    = -1;
        _mutable = false;
        _classid = H5T_NO_CLASS;
    }

    /**
     * @brief Construct HDFTypeFactory from by default
     *
     */
    HDFTypeFactory() = default;

    /**
     * @brief Construct HDFTypeFactory from the given arguments
     *
     * @param size Size of the type underlying the type to be created, or an
     * HDFdataset or attribute to determine the held datatype of
     */
    template < typename V >
    HDFTypeFactory(V&& object_or_size,
                   std::enable_if_t< not std::is_same_v< HDFTypeFactory< T >,
                                                         std::decay_t< V > >,
                                     int > = 0)
    {
        // number passed
        if constexpr (std::is_convertible_v< std::decay_t< V >, std::size_t >)
        {
            // include const char* which is a  c-string
            if constexpr (Utils::is_container_v< T >)
            {
                if (object_or_size == 0ul)
                {
                    _type = H5Tvlen_create(
                        Detail::get_type< typename T::value_type >());
                }
                else
                {
                    hsize_t dim[1] = { object_or_size };
                    _type          = H5Tarray_create(
                        Detail::get_type< typename T::value_type >(), 1, dim);
                }
                _mutable = true;
            }
            else if constexpr (Utils::is_string_v< T >)
            {
                if (object_or_size == 0)
                {
                    _type = H5Tcopy(H5T_C_S1);
                    H5Tset_size(_type, H5T_VARIABLE);
                }
                else
                {
                    _type = H5Tcopy(H5T_C_S1);
                    H5Tset_size(_type, object_or_size);
                }

                _mutable = true;
            }
            else
            {
                _type    = Detail::get_type< T >();
                _mutable = false;
            }
        }
        // HDFobject passed
        else
        {
            if constexpr (std::is_pointer_v< std::decay_t< V > >)
            {
                _type = object_or_size->get_type();
                H5Tlock(_type); // make sure that library does memory management
            }
            else
            {
                _type = object_or_size.get_type();
                H5Tlock(_type); // make sure that library does memory management
            }
            _mutable = false;
        }
        _classid = H5Tget_class(_type);
    }
};

/**
 * @brief Check equality of argument typefactories. Two typefactories are
 * considered equal when they refer to the same HDF5 type, e.g., H5T_INTEGER, or
 * H5T_VLEN.
 *
 * @tparam T
 * @tparam U
 * @param lhs
 * @param rhs
 * @return true
 * @return false
 */
template < typename T, typename U >
bool
operator==(const HDFTypeFactory< T >& lhs, const HDFTypeFactory< U >& rhs)
{
    auto equal = H5Tequal(lhs.get_id(), rhs.get_id()) &&
                 lhs.category() == rhs.category();
    if (equal > 0)
    {
        return true;
    }
    else if (equal == 0)
    {
        return false;
    }
    else
    {
        throw std::runtime_error("Error when comparing dataspaces");
        return false;
    }
}

template < typename T, typename U >
bool
operator!=(const HDFTypeFactory< T >& lhs, const HDFTypeFactory< U >& rhs)
{
    return not(lhs == rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif
