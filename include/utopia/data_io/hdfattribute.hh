/**
 * @brief This file implements a C++ class which wraps a C HDF5 attribute to
 * a HDF5-object (group or dataset), and provides the necessary functionality
 * to read from, write to and create such objects.
 *
 * @file hdfattribute.hh
 */
#ifndef UTOPIA_DATAIO_HDFATTRIBUTE_HH
#define UTOPIA_DATAIO_HDFATTRIBUTE_HH

#include <cstring>
#include <functional>
#include <memory>
#include <string>

#include <hdf5.h>
#include <hdf5_hl.h>

#include "hdfbufferfactory.hh"
#include "hdfdataspace.hh"
#include "hdfobject.hh"
#include "hdftype.hh"

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
 * @brief      Class for hdf5 attribute, which can be attached to groups and
 *             datasets.
 *
 * @tparam     HDFObject  the object class
 */
class HDFAttribute final : public HDFObject< HDFCategory::attribute >
{
  private:
    /**
     * @brief      size of the attributes dataspace
     */
    std::vector< hsize_t > _shape;

    /**
     * @brief      Identifier of the parent object
     */
    HDFIdentifier _parent_identifier;

    /**
     * @brief Dataspace object that manages topology of data written and read
     *
     */
    HDFDataspace _dataspace;

    /**
     * @brief type done in the HDFAttribute
     *
     */
    HDFType _type;

    /**
     * @brief      private helper function for creation of attribute
     *
     * @param[in]  typesize     The typesize
     *
     * @tparam     result_type  Type of the resulting attribute
     *
     * @return     hid_t
     */
    template < typename result_type >
    void
    __create_attribute__(hsize_t typesize = 0)
    {
        this->_log->debug("Creating attribute {}", _path);

        _dataspace.open(this->_path + " dataspace", _shape.size(), _shape, {});

        _type.open< result_type >("datatype of " + _path, typesize);

        this->bind_to(H5Acreate2(_parent_identifier.get_id(),
                                 _path.c_str(),
                                 _type.get_C_id(),
                                 _dataspace.get_C_id(), // dspace
                                 H5P_DEFAULT,
                                 H5P_DEFAULT),
                      &H5Aclose,
                      _path);
    }

    /**
     * @brief Function for writing containers as attribute.
     *
     * @details Only buffers if necessary, i.e. if non-vector containers or
     *         nested containers or containers of strings have to be written.
     */
    template < typename Type >
    herr_t
    __write_container__(Type attribute_data)
    {
        this->_log->debug("Writing container data to attribute {} ...", _path);
        using value_type_1 = typename Type::value_type;
        using base_type    = Utils::remove_qualifier_t< value_type_1 >;

        // we can write directly if we have a plain vector, no nested or
        // stringtype.
        if constexpr (std::is_same_v< Type, std::vector< value_type_1 > > and
                      not Utils::is_container_v< value_type_1 > and
                      not Utils::is_string_v< value_type_1 >)
        {
            this->_log->debug("... of simple vector type");

            // check if attribute has been created, else do
            if (get_C_id() == -1)
            {
                __create_attribute__< base_type >(0);
            }

            return H5Awrite(
                get_C_id(), _type.get_C_id(), attribute_data.data());
        }
        // when stringtype or containertype is stored in a container, then
        // we have to buffer. bufferfactory handles how to do this in detail
        else
        {
            this->_log->debug("... of non-trivial container type");
            // if we want to write std::arrays then we can use
            // fixed size array types. Else we use variable length arrays.
            if constexpr (Utils::is_array_like_v< value_type_1 >)
            {
                this->_log->debug("... of fixed size array type");
                constexpr std::size_t s =
                    Utils::get_size< value_type_1 >::value;

                if (get_C_id() == -1)
                {
                    __create_attribute__< base_type >(s);
                }

                auto buffer = HDFBufferFactory::buffer(
                    std::begin(attribute_data),
                    std::end(attribute_data),
                    [](auto& value) -> value_type_1& { return value; });

                return H5Awrite(get_C_id(), _type.get_C_id(), buffer.data());
            }
            else
            {
                this->_log->debug("... of variable length type");
                if (get_C_id() == -1)
                {
                    __create_attribute__< base_type >();
                }

                auto buffer = HDFBufferFactory::buffer(
                    std::begin(attribute_data),
                    std::end(attribute_data),
                    [](auto& value) -> value_type_1& { return value; });

                return H5Awrite(get_C_id(), _type.get_C_id(), buffer.data());
            }
        }
    }

    // Function for writing stringtypes, char*, const char*, std::string
    template < typename Type >
    herr_t
    __write_stringtype__(Type attribute_data)
    {

        this->_log->debug("Writing string data to attribute {} ...", _path);

        // Since std::string cannot be written directly,
        // (only const char*/char* can), a buffer pointer has been added
        // to handle writing in a clearer way and with less code
        auto        len    = 0;
        const char* buffer = nullptr;

        if constexpr (std::is_pointer_v< Type >) // const char* or char* ->
                                                 // strlen needed
        {
            len    = std::strlen(attribute_data);
            buffer = attribute_data;
        }
        else // simple for strings
        {
            len    = attribute_data.size();
            buffer = attribute_data.c_str();
        }

        // check if attribute has been created, else do
        if (get_C_id() == -1)
        {
            __create_attribute__< const char* >(len);
        }

        // use that strings store data in consecutive memory
        return H5Awrite(get_C_id(), _type.get_C_id(), buffer);
    }

    // Function for writing pointer types, shape of the array has to be given
    // where shape means the same as in python
    template < typename Type >
    herr_t
    __write_pointertype__(Type attribute_data)
    {
        this->_log->debug("Writing pointer data to attribute {} ...", _path);

        // result types removes pointers, references, and qualifiers
        using basetype = Utils::remove_qualifier_t< Type >;

        if (get_C_id() == -1)
        {
            __create_attribute__< basetype >();
        }

        return H5Awrite(get_C_id(), _type.get_C_id(), attribute_data);
    }

    // function for writing a scalartype.
    template < typename Type >
    herr_t
    __write_scalartype__(Type attribute_data)
    {
        this->_log->debug("Writing scalar data to attribute {} ...", _path);

        // because we just write a scalar, the shape tells basically that
        // the attribute is pointlike: 1D and 1 entry.
        if (get_C_id() == -1)
        {
            __create_attribute__< std::decay_t< Type > >();
        }

        return H5Awrite(get_C_id(), _type.get_C_id(), &attribute_data);
    }

    // Container reader.
    // We could want to read into a predefined buffer for some reason (frequent
    // reads), and thus this and the following functions expect an argument
    // 'buffer' to store their data in. The function 'read(..)' is then
    // overloaded to allow for automatic buffer creation or a buffer argument.
    template < typename Type >
    herr_t
    __read_container__(Type& buffer)
    {

        this->_log->debug("Reading container data from attribute {} ...",
                          _path);

        using value_type_1 =
            Utils::remove_qualifier_t< typename Type::value_type >;

        // when the value_type of Type is a container again, we want nested
        // arrays basically. Therefore we have to check if the desired type
        // Type is suitable to hold them, read the nested data into a hvl_t
        // container, assuming that they are varlen because this is the more
        // general case, and then turn them into the desired type again...
        if constexpr (Utils::is_container_v< value_type_1 >)
        {
            using value_type_2 =
                Utils::remove_qualifier_t< typename value_type_1::value_type >;

            // if we have nested containers of depth larger than 2, throw a
            // runtime error because we cannot handle this
            // TODO extend this to work more generally
            if constexpr (Utils::is_container_v< value_type_2 >)
            {
                throw std::runtime_error(
                    "Cannot read data into nested containers with depth > 3 "
                    "in attribute " +
                    _path + " into vector containers!");
            }

            // everything is fine.

            // check if type given in the buffer is std::array.
            // If it is, the user knew that the data stored there
            // has always the same length, otherwise she does not
            // know and thus it is assumed that the data is variable
            // length.
            if constexpr (Utils::is_array_like_v< value_type_1 >)
            {
                this->_log->debug("... of fixed size array type");
                return H5Aread(get_C_id(), _type.get_C_id(), buffer.data());
            }
            else
            {
                this->_log->debug("... of variable size type");

                std::vector< hvl_t > temp_buffer(buffer.size());

                herr_t err =
                    H5Aread(get_C_id(), _type.get_C_id(), temp_buffer.data());

                // turn the varlen buffer into the desired type
                // Cumbersome, but necessary...

                this->_log->debug("... turning read data into desired type");

                for (std::size_t i = 0; i < buffer.size(); ++i)
                {
                    buffer[i].resize(temp_buffer[i].len);
                    for (std::size_t j = 0; j < temp_buffer[i].len; ++j)
                    {
                        buffer[i][j] =
                            static_cast< value_type_2* >(temp_buffer[i].p)[j];
                    }
                }

                // return shape and buffer. Expect to use structured bindings
                // to extract that later
                return err;
            }
        }
        else // no nested container, but one containing simple types
        {
            if constexpr (!std::is_same_v< std::vector< value_type_1 >, Type >)
            {
                throw std::runtime_error("Can only read data from " + _path +
                                         " into vector containers!");
            }
            else
            {
                // when strings are desired to be stored as value_types of the
                // container, we need to treat them a bit differently,
                // because hdf5 cannot read directly to them.
                if constexpr (Utils::is_string_v< value_type_1 >)
                {
                    this->_log->debug("... of string type");

                    std::vector< char* > temp_buffer(buffer.size());

                    herr_t err = H5Aread(
                        get_C_id(), _type.get_C_id(), temp_buffer.data());

                    // turn temp_buffer into the desired datatype and return
                    for (std::size_t i = 0; i < buffer.size(); ++i)
                    {
                        buffer[i] = temp_buffer[i];
                    }
                    // return
                    return err;
                }
                else // others are straight forward
                {
                    this->_log->debug("... of scalar type");

                    return H5Aread(get_C_id(), _type.get_C_id(), buffer.data());
                }
            }
        }
    }

    // read attirbute data which contains a single string.
    // this is always read into std::strings, and hence
    // we can use 'resize'
    template < typename Type >
    auto
    __read_stringtype__(Type& buffer)
    {

        this->_log->debug("Reading string type from attribute {}", _path);

        // resize buffer to the size of the type
        buffer.resize(_type.size());

        // read data
        return H5Aread(get_C_id(), _type.get_C_id(), buffer.data());
    }

    // read pointertype. Either this is given by the user, or
    // it is assumed to be 1d, thereby flattening Nd attributes
    template < typename Type >
    auto
    __read_pointertype__(Type buffer)
    {
        this->_log->debug("Reading pointer type from attribute {}", _path);

        return H5Aread(get_C_id(), _type.get_C_id(), buffer);
    }

    // read scalar type, trivial
    template < typename Type >
    auto
    __read_scalartype__(Type& buffer)
    {
        this->_log->debug("Reading scalar type from attribute {}", _path);

        return H5Aread(get_C_id(), _type.get_C_id(), &buffer);
    }

  public:
    /**
     * @brief alias base class
     *
     */
    using Base = HDFObject< HDFCategory::attribute >;

    /**
     * @brief Exchange states between caller and argument
     *
     * @param other
     */
    void
    swap(HDFAttribute& other)
    {
        using std::swap;
        using Utopia::DataIO::swap;
        swap(static_cast< Base& >(*this), static_cast< Base& >(other));
        swap(_shape, other._shape);
        swap(_parent_identifier, other._parent_identifier);
        swap(_dataspace, other._dataspace);
    }

    /**
     * @brief Get the file-dataspace object
     *
     * @return HDFDataspace
     */
    HDFDataspace
    get_filespace()
    {
        return _dataspace;
    }

    /**
     * @brief Get the type object
     *
     * @return hid_t
     */
    auto
    get_type()
    {
        return _type;
    }

    /**
     * @brief      Get the hdf5 identifier to which the attribute belongs
     *
     * @return     weak pointer to HDFObject
     */
    HDFIdentifier
    get_parent_id()
    {
        return _parent_identifier;
    }

    /**
     * @brief      closes the attribute
     */
     void
    close() 
    {
        _type.close();
        _dataspace.close();
        _shape.resize(0); 
        _parent_identifier.close();

        _dataspace.close();

        if (is_valid())
        {
            Base::close();
        }
    }

    /**
     * @brief Get the shape object
     *
     * @return std::vector<hsize_t>
     */
    auto
    get_shape()
    {
        // the below is done to retain vector type for _shape member,
        // which otherwise would break interface due to type change.
        auto shape = _dataspace.size();
        _shape.assign(shape.begin(), shape.end());

        return _shape;
    }

    /**
     * @brief Open a new attribute on HDFObject 'parent' with name 'name'
     *
     * @param parent HDFObject to open the attribute on
     * @param name  The attribute's name
     */
    template < HDFCategory cat >
    void
    open(const HDFObject< cat >& parent, std::string name)
    {
        this->_log->debug(
            "Opening attribute named {} in object {}", name, parent.get_path());
        open(parent.get_id_object(), name);
    }

    /**
     * @brief Open a new attribute on HDFObject 'parent' with name 'name'
     *
     * @param parent HDFIdentifier to open the attribute on
     * @param name  The attribute's name
     */
    void
    open(const HDFIdentifier& parent, std::string name)
    {
        // update members
        _parent_identifier = parent;

        _path              = name;

        if (parent.is_valid())
        {

            if (H5LTfind_attribute(_parent_identifier.get_id(),
                                   _path.c_str()) == 1)
            {
                this->_log->debug("... attribute exists already, opening");
                // attribute exists, open
                bind_to(H5Aopen(_parent_identifier.get_id(),
                                _path.c_str(),
                                H5P_DEFAULT),
                        &H5Aclose,
                        name);
                
                _type.open(*this);

                // README (regarding usage of this in constructors): the usage
                // of *this is save here, because we always have a valid object
                // of type HDFDataspace even in ctor given that this is neither
                // a derived class nor we call a virtual function
                _dataspace.open(*this);
                get_shape();
            }
            else
            {
                this->_log->debug("... attribute does not yet exist, have to "
                                  "wait for incoming data to build it");

                this->_id = HDFIdentifier();
            }
        }
        else
        {
            throw std::invalid_argument(
                "Parent object of attribute '" + _path +
                "' is invalid! Has it been closed already or not been opened "
                "yet?");
        }
    }

    /**
     * @brief Reads data from attribute, and returns the data and its shape in
     *        the form of a hsize_t vector.
     *
     *        This function has a quirk:
     *         - N-dimensional data are read into 1d arrays, and the shape has
     *           to be used to regain the original layout via index arithmetic.
     *
     *        Depending on the type 'Type', the data will be returned as:
     *        - if Type is a container other than vector: Will throw
     runtime_error
     *        - if Type is a vector or vector of vectors: Will return this,
     filled with data
     *        - if Type is a stringtype, i.e. char*, const char*, std::string:
     Will return std::string
     *        - if Type is a plain type, will return plain type
     *        - if Type as a pointer type, it will return a shared_ptr, i.e.
     Type = double*, return will be std::shared_ptr<double>;

     *
     * @tparam Type which can hold elements in the attribute and which will be
     returned
     * @return tuple containing (shape, data)
     */
    template < typename Type >
    auto
    read()
    {
        this->_log->debug("Reading attribute {}", _path);

        if (_shape.size() == 0)
        {
            get_shape();
        }

        // Read can only be done in 1d, and this loop takes care of
        // computing a 1d size which can accomodate all elements.
        // This is then passed to the container holding the elements in the end.
        std::size_t size = 1;
        for (auto& value : _shape)
        {
            size *= value;
        }

        // type to read in is a container type, which can hold containers
        // themselvels or just plain types.
        if constexpr (Utils::is_container_v< Type >)
        {
            Type   buffer(size);
            herr_t err = __read_container__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into container types");
            }

            return std::make_tuple(_shape, buffer);
        }
        else if constexpr (Utils::is_string_v< Type >) // we can have string
                                                       // types too, i.e. char*,
                                                       // const char*,
                                                       // std::string
        {
            std::string buffer; // resized in __read_stringtype__ because this
                                // as a scalar
            herr_t err = __read_stringtype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into stringtype");
            }

            return std::make_tuple(_shape, buffer);
        }
        else if constexpr (std::is_pointer_v< Type > &&
                           !Utils::is_string_v< Type >)
        {
            auto   buffer = std::make_shared< Type >(size);
            herr_t err    = __read_pointertype__(buffer.get());
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into pointertype");
            }
            return std::make_tuple(_shape, buffer);
        }
        else // reading scalar types is simple enough
        {
            Type   buffer(0);
            herr_t err = __read_scalartype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into scalar");
            }
            return std::make_tuple(_shape, buffer);
        }
    }

    /**
     * @brief Reads data from attribute into a predefined buffer, and returns
     * the data and its shape in the form of a hsize_t vector. User is
     * responsible for providing a buffer which can hold the data and has the
     * correct shape!
     *
     * @tparam Type which can hold elements in the attribute and which will be
     * returned
     */
    template < typename Type >
    void
    read(Type& buffer)
    {
        this->_log->debug("Reading attribute {} into given buffer", _path);

        if (_shape.size() == 0)
        {
            get_shape();
        }

        // type to read in is a container type, which can hold containers
        // themselvels or just plain types.
        if constexpr (Utils::is_container_v< Type >)
        {
            // needed to make sure that the buffer has the correct size

            std::size_t size = 1;
            for (auto& value : _shape)
            {
                size *= value;
            }

            if (buffer.size() != size)
            {
                buffer.resize(size);
            }

            herr_t err = __read_container__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into container types");
            }
        }
        else if constexpr (Utils::is_string_v< Type >) // we can have string
                                                       // types too, i.e. char*,
                                                       // const char*,
                                                       // std::string
        {
            // resized in __read_stringtype__ because this as a scalar
            herr_t err = __read_stringtype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into stringtype");
            }
        }
        else if constexpr (std::is_pointer_v< Type > &&
                           !Utils::is_string_v< Type >)
        {
            herr_t err = __read_pointertype__(buffer);

            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into pointertype");
            }
        }
        else // reading scalar types is simple enough
        {
            herr_t err = __read_scalartype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _path +
                                         " into scalar");
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute.
     *
     * @tparam Type  Automatically determined type of the data to write
     * @param attribute_data  Data to write
     * @param shape Layout of the data, i.e. {20, 50} would indicate a 2d array
     * like int a[20][50]; The parameter has only to be given if the data to be
     * written is given as plain pointers, because the shape cannot be
     * determined automatically then.
     * @param typelen If the elements of the data to be written are arrays of
     * equal length, and the data should be written as 1d attribute, then we can
     * give the length of the arrays in order to speed up the memory allocation
     * and avoid unecessary buffering. This can be useful for grids for
     * instance.
     */
    template < typename Type >
    void
    write(Type attribute_data, std::vector< hsize_t > shape = {})
    {
        this->_log->debug("Writing data to attribute {}", _path);

        // check if we have a container. Writing containers requires further
        // t tests, plain old data can be written right away

        if constexpr (Utils::is_container_v< Type >) // container type
        {
            // get the shape from the data. This function is written such
            // that it writes always 1d, unless a pointer type with different
            // shape is given.
            if (_shape.size() == 0)
            {
                _shape = { attribute_data.size() };
            }
            else
            {
                _shape = shape;
            }
            herr_t err = __write_container__(attribute_data);

            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occurred while writing a containertype to "
                    "attribute " +
                    _path + "!");
            }
        }
        // write string types, i.e. const char*, char*, std::string
        // These are not containers but not normal scalars either,
        // so they have to be treated separatly
        else if constexpr (Utils::is_string_v< Type >)
        {
            _shape = { 1 };

            herr_t err = __write_stringtype__(attribute_data);

            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occurred while writing a stringtype to "
                    "attribute " +
                    _path + "!");
            }
        }
        // We can also write pointer types, but then the shape of the array
        // has to be given explicitly. This does not handle stringtypes,
        // even though const char* /char* are pointer types
        else if constexpr (std::is_pointer_v< Type > &&
                           !Utils::is_string_v< Type >)
        {
            if (shape.size() == 0)
            {
                throw std::runtime_error(
                    "Attribute: " + _path +
                    ","
                    "The shape parameter has to be given for pointers "
                    "because "
                    "it cannot be determined automatically");
            }
            else
            {
                _shape = shape;
            }

            herr_t err = __write_pointertype__(attribute_data);

            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occurred while writing a pointertype/plain array "
                    "to "
                    "attribute " +
                    _path + "!");
            }
        }
        // plain scalar types are treated in a straight forward way
        else
        {
            _shape = { 1 };

            herr_t err = __write_scalartype__(attribute_data);
            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occurred while writing a scalar "
                    "to "
                    "attribute " +
                    _path + "!");
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute
     *
     * @tparam Iter Iterator type
     * @tparam Adaptor Adaptor which allows for extraction of a value from
     * compound types, i.e. classes or structs
     * @param begin Start of iterator range
     * @param end End of iterator range
     * @param adaptor Function which turns the elements of the iterator into the
     * objects to be written. Should (auto&) argument (simplest), but else must
     * have (typename Iter::value_type&) argument
     */
    template < typename Iter, typename Adaptor >
    void
    write(
        Iter    begin,
        Iter    end,
        Adaptor adaptor = [](auto& value) { return value; },
        [[maybe_unused]] std::vector< hsize_t > shape = {})
    {
        this->_log->debug("Writing data from iterator range to attribute {}",
                          _path);

        using Type = Utils::remove_qualifier_t< decltype(adaptor(*begin)) >;
        // if we copy only the content of [begin, end), then simple vector
        // copy suffices
        if constexpr (std::is_same< Type, typename Iter::value_type >::value)
        {
            write(std::vector< Type >(begin, end));
        }
        else
        {

            std::vector< Type > buff(std::distance(begin, end));
            std::generate(buff.begin(), buff.end(), [&begin, &adaptor]() {
                return adaptor(*(begin++));
            });

            write(buff, shape);
        }
    }

    /**
     * @brief Default constructor, deleted because of reference member
     *
     */
    HDFAttribute() = default;

    /**
     * @brief      Copy constructor
     *
     * @param      other  The other
     */
    HDFAttribute(const HDFAttribute& other) = default;

    /**
     * @brief      Move constructor
     *
     * @param      other  The other
     */
    HDFAttribute(HDFAttribute&& other) = default;

    /**
     * @brief      Copy assignment operator
     *
     * @param      other  Object to copy assign from
     *
     * @return     HDFAttribute&
     */
    HDFAttribute&
    operator=(const HDFAttribute& other) = default;

    /**
     * @brief Attribute move assignment operator
     *
     * @param other Object to move assing from
     * @return HDFAttribute&
     */
    HDFAttribute&
    operator=(HDFAttribute&& other) = default;
    /**
     * @brief Destructor
     *
     */
    virtual ~HDFAttribute()
    {
        close();
    }

    /**
     * @brief      Constructor for attribute
     *
     * @param      object  the object to create the attribute at
     * @param      name    the name of the attribute
     * @param      size    the size of the attribute if known, else 1
     */
    template < HDFCategory cat >
    HDFAttribute(const HDFObject< cat >& object, std::string name) :
        _shape(std::vector< hsize_t >())
    {
        open(object, name);
    }
};

/**
 * @brief Swaps the states of Attributes rhs and lhs.
 *
 * @tparam ParentObject1
 * @tparam ParentObject2
 * @param lhs First Attribute to swap
 * @param rhs Second Attribute to swap
 */
void
swap(HDFAttribute& lhs, HDFAttribute& rhs)
{
    lhs.swap(rhs);
}

/*! \} */ // end of group HDF5
/*! \} */ // end of group DataIO

} // namespace DataIO
} // namespace Utopia
#endif
