#ifndef HDFATTRIBUTE_HH
#define HDFATTRIBUTE_HH
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"

#include <cstring>
#include <functional>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <memory>
#include <string>

namespace Utopia
{
namespace DataIO
{
/**
 * @brief Class for hdf5 attribute, which can be attached to groups and
 * datasets.

 */
template <typename HDFObject>
class HDFAttribute
{
private:
    // helper for making attribute

    /**
     * @brief private helper function for creation of attribute
     *
     * @tparam result_type
     * @return hid_t
     */
    template <typename result_type>
    hid_t __create_attribute__(hsize_t typesize = 0)
    {
        hid_t dspace = H5Screate_simple(_shape.size(), _shape.data(), nullptr);

        hid_t atr = H5Acreate2(_parent_object->get_id(), _name.c_str(),
                               HDFTypeFactory::type<result_type>(typesize),
                               dspace, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(dspace);
        return atr;
    }

    // Function for writing containers as attribute.
    // Only buffers if necessary, i.e. if non-vector containers or
    // nested containers or containers of strings have to be written.
    template <typename Type>
    herr_t __write_container__(Type attribute_data)
    {
        using value_type_1 = typename Type::value_type;
        using base_type = remove_qualifier_t<value_type_1>;
        // we can write directly if we have a plain vector, no nested or stringtype.
        if constexpr (std::is_same_v<Type, std::vector<value_type_1>> &&
                      !is_container_v<value_type_1> && !is_string_v<value_type_1>)
        {
            // check if attribute has been created, else do
            if (_attribute == -1)
            {
                _attribute = __create_attribute__<base_type>(0);
            }

            return H5Awrite(_attribute, HDFTypeFactory::type<base_type>(0),
                            attribute_data.data());
        }
        // when stringtype or containertype is stored in a container, then
        // we have to buffer. bufferfactory handles how to do this in detail
        else
        {
            if (_attribute == -1)
            {
                _attribute = __create_attribute__<base_type>(0);
            }

            auto buffer = HDFBufferFactory::buffer(
                std::begin(attribute_data), std::end(attribute_data),
                [](auto& value) -> value_type_1& { return value; });

            return H5Awrite(_attribute, HDFTypeFactory::type<base_type>(0),
                            buffer.data());
        }
    }

    // Function for writing stringtypes, char*, const char*, std::string
    template <typename Type>
    herr_t __write_stringtype__(Type attribute_data)
    {
        // Since std::string cannot be written directly,
        // (only const char*/char* can), a buffer pointer has been added
        // to handle writing in a clearer way and with less code
        auto len = 0;
        const char* buffer = nullptr;

        if constexpr (std::is_pointer_v<Type>) // const char* or char* -> strlen needed
        {
            len = std::strlen(attribute_data);
            buffer = attribute_data;
        }
        else // simple for strings
        {
            len = attribute_data.size();
            buffer = attribute_data.c_str();
        }

        // check if attribute has been created, else do
        if (_attribute == -1)
        {
            _attribute = __create_attribute__<const char*>(len);
        }
        // use that strings store data in consecutive memory
        return H5Awrite(_attribute, HDFTypeFactory::type<const char*>(len), buffer);
    }

    // Function for writing pointer types, shape of the array has to be given
    // where shape means the same as in python
    template <typename Type>
    herr_t __write_pointertype__(Type attribute_data)
    {
        // result types removes pointers, references, and qualifiers
        using basetype = remove_qualifier_t<Type>;
        std::cout << _name << ", type for pointer is  " << typeid(basetype).name()
                  << ", int type is " << typeid(int).name() << std::endl;

        if (_attribute == -1)
        {
            _attribute = __create_attribute__<basetype>();
        }

        return H5Awrite(_attribute, HDFTypeFactory::type<basetype>(), attribute_data);
    }

    // function for writing a scalartype.
    template <typename Type>
    herr_t __write_scalartype__(Type attribute_data)
    {
        // because we just write a scalar, the shape tells basically that
        // the attribute is pointlike: 1D and 1 entry.
        using basetype = remove_qualifier_t<Type>;
        if (_attribute == -1)
        {
            _attribute = __create_attribute__<Type>();
        }

        return H5Awrite(_attribute, HDFTypeFactory::type<basetype>(), &attribute_data);
    }

    // Container reader.
    // We could want to read into a predefined buffer for some reason (frequent
    // reads), and thus this and the following functions expect an argument
    // 'buffer' to store their data in. The function 'read(..)' is then
    // overloaded to allow for automatic buffer creation or a buffer argument.
    template <typename Type>
    herr_t __read_container__(Type& buffer)
    {
        using value_type_1 = remove_qualifier_t<typename Type::value_type>;

        // when the value_type of Type is a container again, we want nested
        // arrays basically. Therefore we have to check if the desired type Type
        // is suitable to hold them, read the nested data into a hvl_t
        // container, assuming that they are varlen because this is the more
        // general case, and then turn them into the desired type again...
        if constexpr (is_container_v<value_type_1>)
        {
            using value_type_2 = remove_qualifier_t<typename value_type_1::value_type>;

            // if containers inside are not vectors, throw exception, we
            // cannot read into anything else
            if constexpr (!std::is_same_v<std::vector<value_type_2>, value_type_1>)
            {
                throw std::runtime_error("Can only read data from " + _name +
                                         " into vector containers!");
            }
            else // if we have vectors, the reading process is straight forward
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);

                // assume array data to always be varlen, because
                // this project can write only varlen data
                std::vector<hvl_t> temp_buffer(buffer.size());

                herr_t err = H5Aread(_attribute, type, temp_buffer.data());

                // turn the varlen buffer into the desired type.
                // Cumbersome, but necessary...
                for (std::size_t i = 0; i < buffer.size(); ++i)
                {
                    buffer[i].resize(temp_buffer[i].len);
                    for (std::size_t j = 0; j < temp_buffer[i].len; ++j)
                    {
                        buffer[i][j] = static_cast<value_type_2*>(temp_buffer[i].p)[j];
                    }
                }

                // return shape and buffer. Expect to use structured bindings to
                // extract that later
                return err;
            }
        }
        else // no nested container, but one containing simple types
        {
            if constexpr (!std::is_same_v<std::vector<value_type_1>, Type>)
            {
                throw std::runtime_error("Can only read data from " + _name +
                                         " into vector containers!");
            }
            else
            {
                // when strings are desired to be stored as value_types of the
                // container, we need to treat them a bit differently,
                // because hdf5 cannot read directly to them.
                if constexpr (is_string_v<value_type_1>)
                {
                    // get type the attribute has internally
                    hid_t type = H5Aget_type(_attribute);
                    std::vector<char*> temp_buffer(buffer.size());

                    herr_t err = H5Aread(_attribute, type, temp_buffer.data());

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
                    // get type the attribute has internally
                    hid_t type = H5Aget_type(_attribute);

                    return H5Aread(_attribute, type, buffer.data());
                }
            }
        }
    }

    // read attirbute data which contains a single string.
    // this is always read into std::strings, and hence
    // we can use 'resize'
    template <typename Type>
    auto __read_stringtype__(Type& buffer)
    {
        // get type the attribute has internally
        hid_t type = H5Aget_type(_attribute);

        // resize buffer to the size of the type
        buffer.resize(H5Tget_size(type));

        // read data
        return H5Aread(_attribute, type, buffer.data());
    }

    // read pointertype. Either this is given by the user, or
    // it is assumed to be 1d, thereby flattening Nd attributes
    template <typename Type>
    auto __read_pointertype__(Type buffer)
    {
        return H5Aread(_attribute, H5Aget_type(_attribute), buffer);
    }

    // read scalar type, trivial
    template <typename Type>
    auto __read_scalartype__(Type& buffer)
    {
        return H5Aread(_attribute, H5Aget_type(_attribute), &buffer);
    }

protected:
    /**
     * @brief id of attribute itself
     *
     */
    hid_t _attribute;

    /**
     * @brief name of the attribute
     *
     */
    std::string _name;

    /**
     * @brief size of the attributes dataspace
     *
     */
    std::vector<hsize_t> _shape;
    /**
     * @brief reference to id of parent object: dataset or group
     *
     */
    HDFObject* _parent_object;

public:
    /**
     * @brief Get underlying id of attribute
     *
     * @return hid_t underlying HDF5 id.
     */
    hid_t get_id()
    {
        return _attribute;
    }
    /**
     * @brief get Attribute name
     *
     * @return std::string
     */
    std::string get_name()
    {
        return _name;
    }

    /**
     * @brief Get the hdf5 object to which the attribute belongs
     *
     * @return weak pointer to HDFObject
     */
    HDFObject& get_parent()
    {
        return *_parent_object;
    }

    /**
     * @brief closes the attribute
     *
     */
    void close()
    {
        if (_attribute != -1)
        {
            H5Aclose(_attribute);
            _attribute = -1;
        }
    }

    /**
     * @brief Get the shape object
     *
     * @return std::vector<hsize_t>
     */
    auto get_shape()
    {
        if (!H5Iis_valid(_attribute)){
            throw std::runtime_error("Trying to get shape from invalid attribute " + _name);
        }
        // get dataspace: 'Form' of the data stored in the attribute
        hid_t dspace = H5Aget_space(_attribute);

        // get shape -> same thing as numpy array's shape
        _shape.resize(H5Sget_simple_extent_ndims(dspace));
        H5Sget_simple_extent_dims(dspace, _shape.data(), nullptr);
        H5Sclose(dspace);
        return _shape;
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
     *        - if Type is a container other than vector: Will throw runtime_error
     *        - if Type is a vector or vector of vectors: Will return this, filled with data
     *        - if Type is a stringtype, i.e. char*, const char*, std::string: Will return std::string
     *        - if Type is a plain type, will return plain type
     *        - if Type as a pointer type, it will return a shared_ptr, i.e. Type = double*, return will be std::shared_ptr<double>;

     *
     * @tparam Type which can hold elements in the attribute and which will be returned
     * @return tuple containing (shape, data)
     */
    template <typename Type>
    auto read()
    {
        // FIXME: I do not currently like the approach taken here!
        // FIXME: Find a way to retain the data's shape in the returned data
        // FIXME: Find a way to get around the exceptions taken for the
        //        fucking string data
        if (!H5Iis_valid(_attribute))
        {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute named '" +
                _name + "'");
        }

        _shape = get_shape();

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
        if constexpr (is_container_v<Type>)
        {
            Type buffer(size);
            herr_t err = __read_container__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into container types");
            }

            return std::make_tuple(_shape, buffer);
        }
        else if constexpr (is_string_v<Type>) // we can have string types too, i.e. char*, const char*, std::string
        {
            std::string buffer; // resized in __read_stringtype__ because this as a scalar
            herr_t err = __read_stringtype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into stringtype");
            }

            return std::make_tuple(_shape, buffer);
        }
        else if constexpr (std::is_pointer_v<Type> && !is_string_v<Type>)
        {
            auto buffer = std::make_shared<Type>(size);
            herr_t err = __read_pointertype__(buffer.get());
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into pointertype");
            }
            return std::make_tuple(_shape, buffer);
        }
        else // reading scalar types is simple enough
        {
            Type buffer(0);
            herr_t err = __read_scalartype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into scalar");
            }
            return std::make_tuple(_shape, buffer);
        }
    }

    /**
     * @brief Reads data from attribute into a predefined buffer, and returns the data
     *         and its shape in the form of a hsize_t vector.
     *         User is responsible for providing a buffer which can hold the data
     *         and has the correct shape!
     *
     * @tparam Type which can hold elements in the attribute and which will be returned
     */
    template <typename Type>
    void read(Type& buffer)
    {
        _shape = get_shape();

        if (!H5Iis_valid(_attribute))
        {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute named '" +
                _name + "'");
        }

        // type to read in is a container type, which can hold containers
        // themselvels or just plain types.
        if constexpr (is_container_v<Type>)
        {
            herr_t err = __read_container__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into container types");
            }
        }
        else if constexpr (is_string_v<Type>) // we can have string types too, i.e. char*, const char*, std::string
        {
            // resized in __read_stringtype__ because this as a scalar
            herr_t err = __read_stringtype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into stringtype");
            }
        }
        else if constexpr (std::is_pointer_v<Type> && !is_string_v<Type>)
        {
            std::cout << "reading pointer" << std::endl;
            herr_t err = __read_pointertype__(buffer);

            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into pointertype");
            }
        }
        else // reading scalar types is simple enough
        {
            herr_t err = __read_scalartype__(buffer);
            if (err < 0)
            {
                throw std::runtime_error("Error in reading data from " + _name +
                                         " into scalar");
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute.
     *
     * @tparam Type  Automatically determined type of the data to write
     * @param attribute_data  Data to write
     * @param shape Layout of the data, i.e. {20, 50} would indicate a 2d array like int a[20][50];
     *              The parameter has only to be given if the data to be written is given as
     *               plain pointers, because the shape cannot be determined automatically then.
     * @param typelen If the elements of the data to be written are arrays of equal length, and the data should be
     *                written as 1d attribute, then we can give the length of the arrays
     *                in order to speed up the memory allocation and avoid unecessary buffering.
     *                This can be useful for grids for instance.
     *
     */
    template <typename Type>
    void write(Type attribute_data, std::vector<hsize_t> shape = {})
    {
        // check if we have a container. Writing containers requires further
        // t tests, plain old data can be written right away
        if constexpr (is_container_v<Type>) // container type
        {
            // get the shape from the data. This function is written such
            // that it writes always 1d, unless a pointer type with different
            // shape is given.
            if (_shape.size() == 0)
            {
                _shape = {attribute_data.size()};
            }
            else
            {
                _shape = shape;
            }
            herr_t err = __write_container__(attribute_data);
            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occured while writing a containertype to "
                    "attribute " +
                    _name + "!");
            }
        }
        // write string types, i.e. const char*, char*, std::string
        // These are not containers but not normal scalars either,
        // so they have to be treated separatly
        else if constexpr (is_string_v<Type>)
        {
            _shape = {1};
            herr_t err = __write_stringtype__(attribute_data);
            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occured while writing a stringtype to "
                    "attribute " +
                    _name + "!");
            }
        }
        // We can also write pointer types, but then the shape of the array
        // has to be given explicitly. This does not handle stringtypes,
        // even though const char* /char* are pointer types
        else if constexpr (std::is_pointer_v<Type> && !is_string_v<Type>)
        {
            if (shape.size() == 0)
            {
                throw std::runtime_error(
                    "Attribute: " + _name +
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
                    "An error occured while writing a pointertype/plain array "
                    "to "
                    "attribute " +
                    _name + "!");
            }
        }
        // plain scalar types are treated in a straight forward way
        else
        {
            _shape = {1};
            herr_t err = __write_scalartype__(attribute_data);
            if (err < 0)
            {
                throw std::runtime_error(
                    "An error occured while writing a scalar "
                    "to "
                    "attribute " +
                    _name + "!");
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute
     *
     * @tparam Iter Iterator type
     * @tparam Adaptor Adaptor which allows for extraction of a value from compound types, i.e. classes or structs
     * @param begin Start of iterator range
     * @param end End of iterator range
     * @param adaptor Function which turns the elements of the iterator into the objects to be written.
     *        Should (auto&) argument (simplest), but else must have (typename Iter::value_type&) argument
     */
    template <typename Iter, typename Adaptor>
    void write(Iter begin,
               Iter end,
               Adaptor adaptor = [](auto& value) { return value; },
               std::vector<hsize_t> shape = {})
    {
        using Type = remove_qualifier_t<decltype(adaptor(*begin))>;
        // if we copy only the content of [begin, end), then simple vector
        // copy suffices
        if constexpr (std::is_same<Type, typename Iter::value_type>::value)
        {
            write(std::vector<Type>(begin, end));
        }
        else
        {
            // buffer data, have to because we cannot write iterator ranges
            write(HDFBufferFactory::buffer(begin, end, adaptor), shape);
        }
    }

    /**
     * @brief Default constructor, deleted because of reference member
     *
     */
    HDFAttribute() = default;

    /**
     * @brief Copy constructor
     *
     * @param other
     */
    HDFAttribute(const HDFAttribute& other)
        : _attribute(other._attribute),
          _name(other._name),
          _shape(other._shape),
          _parent_object(other._parent_object)
    {
    }

    /**
     * @brief Move constructor
     *
     * @param other
     */
    HDFAttribute(HDFAttribute&& other) : HDFAttribute()
    {
        this->swap(other);
    }

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return HDFAttribute&
     */
    HDFAttribute& operator=(HDFAttribute other)
    {
        this->swap(other);
        return *this;
    }

    /**
     * @brief Destructor
     *
     */
    virtual ~HDFAttribute()
    {
        if (H5Iis_valid(_attribute))
        { // FIXME: add check to make sure that
          // it is not
          // more than once closed
            H5Aclose(_attribute);
        }
    }

    /**
     * @brief Constructor for attribute
     *
     * @param object the object to create the attribute at
     * @param name the name of the attribute
     * @param size the size of the attribute if known, if unknown it is 1
     */

    HDFAttribute(HDFObject& object, std::string name)
        : _name(name), _shape(std::vector<hsize_t>()), _parent_object(&object)
    {
        // checks the validity and opens attribute if possible, else
        // postphones until it is written
        if (H5Iis_valid(_parent_object->get_id()) == false)
        {
            throw std::invalid_argument(
                "parent_object of attribute " + _name +
                " is invalid, has it been closed already?");
        }
        else
        {
            if (H5LTfind_attribute(_parent_object->get_id(), _name.c_str()) == 1)
            { // attribute exists
                _attribute = H5Aopen(_parent_object->get_id(), _name.c_str(), H5P_DEFAULT);
            }
            else
            { // attribute does not exist: make
                _attribute = -1;
            }
        }
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
template <typename ParentObject1, typename ParentObject2>
void swap(HDFAttribute<ParentObject1>& lhs, HDFAttribute<ParentObject2>& rhs)
{
    lhs.swap(rhs);
}

} // namespace DataIO
} // namespace Utopia
#endif