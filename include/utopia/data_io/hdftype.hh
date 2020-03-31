/**
 * @brief This file provides a class which is responsible for the automatic
 *        conversion between C/C++ types and HDF5 type identifiers.
 *
 * @file hdftype.hh
 */
#ifndef UTOPIA_DATAIO_HDFTYPEFACTORY_HH
#define UTOPIA_DATAIO_HDFTYPEFACTORY_HH

#include <variant>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <utopia/core/utils.hh>

#include "hdfobject.hh"
#include "hdfutilities.hh"

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
class HDFType final : public HDFObject< HDFCategory::datatype >
{
  private:
    // identify if the type is mutable or not. Unfortunatelly there is no
    // HDF5-intrinsic way to check this...
    bool _mutable = false;

    // enumeration telling what class the type belongs to
    H5T_class_t _classid;

  public:
    using Base = HDFObject< HDFCategory::datatype >;

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
    type_category() const
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
        return H5Tget_size(get_C_id());
    }

    /**
     * @brief Open the HDF5 type associated with an HDFObject, i.e., a
     *        dataset or an attribute.
     *
     * @tparam T
     * @param object
     */
    template < typename T >
    void
    open(T&& object)
    {
        this->_log->debug("Opening HDFType from existing object {}",
                          object.get_path());

        if (is_valid())
        {
            throw std::runtime_error(
                "Error, cannot open HDFType while it's still bound to another "
                "valid type object, close it first");
        }

        bind_to(open_type(object),
                &H5Tclose, // we lock the type, hence no H5Tclose needed
                "datatype of " + object.get_path());

        _mutable = true;
        _classid = H5Tget_class(get_C_id());
    }

    /**
     * @brief Create an HDF datatype corresponding to the C datatype given as
     * template argument
     *
     * @tparam T
     * @param object_or_size size of the datatype in bytes, if not given,
     * automatically determined: containers and strings become variable length
     * size, everything else becomes scalar
     */
    template < typename T >
    void
    open(std::string name, hsize_t typesize)
    {
        this->_log->debug("Opening HDFType from scratch");

        if (is_valid())
        {
            throw std::runtime_error(
                "Error, cannot open HDFType '" + name + "' while it's still bound "
                "to another valid type object! Close it first.");
        }

        // include const char* which is a  c-string
        if constexpr (Utils::is_container_v< T >)
        {
            if (typesize == 0ul)
            {
                bind_to(H5Tvlen_create(
                            Detail::get_type< typename T::value_type >()),
                        &H5Tclose,
                        name);
            }
            else
            {
                hsize_t dim[1] = { typesize };
                bind_to(
                    H5Tarray_create(
                        Detail::get_type< typename T::value_type >(), 1, dim),
                    &H5Tclose,
                    name);
            }
            _mutable = true;
        }
        else if constexpr (Utils::is_string_v< T >)
        {
            hid_t type = H5Tcopy(H5T_C_S1);

            if (typesize == 0)
            {
                H5Tset_size(type, H5T_VARIABLE);
            }
            else
            {
                H5Tset_size(type, typesize);
            }
            bind_to(std::move(type), &H5Tclose, name);

            _mutable = true;
        }
        else
        {
            // native type objects like H5T_NATIVE_INT are interpreted as
            // invalid by H5Iis_valid, hence at this point the `bind_to`
            // function is not used, because it checks validity
            _id.open(Detail::get_type< T >(),
                     [](hid_t) -> herr_t { return 0; });
            _path    = name;
            _mutable = false;
        }

        _classid = H5Tget_class(get_C_id());
    }

    /**
     * @brief Construct close from the given arguments
     *
     */
     void
    close() 
    {

        // everything that is obtained via H5Tcopy, H5Topen or H5Tcreate
        // needs to be released explicitly. This is given by the _mutable
        // flag
        Base::close();
        _mutable = false;
        _classid = H5T_NO_CLASS;
    }

    /**
     * @brief Check if the held type identifier is still valid. Primitive types
     * are valid by definition, because we have no control over them and hence
     * they cannot be invalidated
     *
     * @return true
     * @return false
     */
    virtual bool
    is_valid() const override
    {
        // this distinction is important because identifiers are not always
        // checkable via H5Iis_valid
        if (not(_classid == H5T_VLEN or _classid == H5T_ARRAY or
                _classid == H5T_STRING) and
            not(get_C_id() == -1))
        {
            return true;
        }
        else
        {
            return Base::is_valid();
        }
    }

    /**
     * @brief Construct HDFType from the given arguments by move,
     * deleted, because apparently incompatible with HDF5 C backend
     *
     */
    HDFType(HDFType&& other) : Base(static_cast< Base&& >(other))
    {
        _mutable = std::move(other._mutable);
        _classid = std::move(other._classid);

        other._mutable = false;
        other._classid = H5T_NO_CLASS;
    }

    /**
     * @brief Move assign the type
     *
     * @return HDFType&
     */
    HDFType&
    operator=(HDFType&& other)
    {
        static_cast<Base&>(*this) = static_cast< Base&& >(other);
        _mutable = std::move(other._mutable);
        _classid = std::move(other._classid);

        other._mutable = false;
        other._classid = H5T_NO_CLASS;

        return *this;
    }

    /**
     * @brief Construct HDFType from the given arguments by copy
     *
     */
    HDFType(const HDFType& other) =  default;

    /**
     * @brief Copy assign type
     *
     * @return HDFType&
     */
    HDFType&
    operator=(const HDFType& other) = default;

    /**
     * @brief Destroy the HDFType object
     *
     */
    virtual ~HDFType()
    {
        close();
    }

    /**
     * @brief Construct HDFType from by default
     *
     */
    HDFType() : Base(), _mutable(false), _classid(H5T_NO_CLASS)
    {
    }

    /**
     * @brief Construct HDFType from the given arguments
     *
     * @param size Size of the type underlying the type to be created, or an
     * HDFdataset or attribute to determine the held datatype of
     */
    template < typename T >
    HDFType(T&& object_or_size,
            std::enable_if_t< not std::is_same_v< HDFType, std::decay_t< T > >,
                              int > = 0)
    {
        open(object_or_size);
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
bool
operator==(const HDFType& lhs, const HDFType& rhs)
{
    auto equal = H5Tequal(lhs.get_C_id(), rhs.get_C_id()) &&
                 lhs.type_category() == rhs.type_category();
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

/**
 * @brief TODO
 *
 * @param lhs
 * @param rhs
 * @return true
 * @return false
 */
bool
operator!=(const HDFType& lhs, const HDFType& rhs)
{
    return not(lhs == rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia

#endif
