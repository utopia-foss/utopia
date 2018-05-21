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
    hid_t __make_attribute__(hsize_t typesize = 0)
    {
        hid_t dspace = H5Screate_simple(_shape.size(), _shape.data(), nullptr);

        hid_t atr = H5Acreate2(_parent_object.get().get_id(), _name.c_str(),
                               HDFTypeFactory::type<result_type>(typesize),
                               dspace, H5P_DEFAULT, H5P_DEFAULT);
        H5Sclose(dspace);
        return atr;
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
    std::reference_wrapper<HDFObject> _parent_object;

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
        return _parent_object.get();
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
     * @brief Reads data from attribute, and returns the data and its shape in
     *        the form of a hsize_t vector.
     *
     *        This function has two quirks:
     *         - N-dimensional data are read into 1d arrays, and the shape has
     *           to be used to regain the original layout via index arithmetic.
     *         - The function will always return a vector of the elements,
     *           even when only one element is in it.
     *
     * @tparam Type Type of the elements in the attribute
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

        // get dataspace
        hid_t dspace = H5Aget_space(_attribute);

        // get shape
        _shape.resize(H5Sget_simple_extent_ndims(dspace));
        H5Sget_simple_extent_dims(dspace, _shape.data(), nullptr);
        H5Sclose(dspace);

        // make buffer large enough to read data
        std::size_t size = 1;
        for (auto& value : _shape)
        {
            size *= value;
        }

        // type to read in is a container type, which can hold containers
        // themselvels or just plain types.
        if constexpr (is_container_type<Type>::value)
        {
            using value_type_1 =
                typename HDFTypeFactory::result_type<typename Type::value_type>::type;

            // container of containers
            if constexpr (is_container_type<value_type_1>::value)
            {
                using value_type_2 =
                    typename HDFTypeFactory::result_type<typename value_type_1::value_type>::type;

                // if containers inside are not vectors, throw exception, we
                // cannot read into anything else
                if constexpr (std::is_same_v<std::vector<value_type_2>, value_type_1> == false)
                {
                    throw std::runtime_error(
                        "Can only read attributes into vectors");
                }
                else // if we have vectors, the reading process is straight forward
                {
                    Type buffer(size);

                    // get type the attribute has internally
                    hid_t type = H5Aget_type(_attribute);

                    // check if type is of variable length, and adjust
                    // read procedure accordingly
                    if (H5Tget_class(type) == H5T_VLEN)
                    {
                        std::vector<hvl_t> temp_buffer(size);

                        H5Aread(_attribute, type, temp_buffer.data());

                        for (std::size_t i = 0; i < size; ++i)
                        {
                            buffer[i].resize(temp_buffer[i].len);
                            for (std::size_t j = 0; j < temp_buffer[i].len; ++j)
                            {
                                buffer[i][j] =
                                    static_cast<value_type_2*>(temp_buffer[i].p)[j];
                            }
                        }

                        return std::make_tuple(_shape, buffer);
                    }
                    else // if not varlen, then use the read typesize and read directly.
                    {
                        auto typesize = H5Tget_size(type);
                        for (auto& cont : buffer)
                        {
                            cont.resize(typesize);
                        }
                        H5Aread(_attribute, type, buffer.data());

                        return std::make_tuple(_shape, buffer);
                    }
                }
            }
            else
            {
                if constexpr (std::is_same_v<std::vector<value_type_1>, Type> == false)
                {
                    throw std::runtime_error(
                        "Can only read attributes into vectors");
                }
                else
                {
                    if constexpr (std::is_same_v<typename Type::value_type, std::string>)
                    {
                        std::vector<char*> tempbuffer(size);

                        // get type the attribute has internally
                        hid_t type = H5Aget_type(_attribute);

                        H5Aread(_attribute, type, tempbuffer.data());

                        // put into strings
                        std::vector<std::string> buffer(size);
                        for (std::size_t i = 0; i < size; ++i)
                        {
                            buffer[i] = tempbuffer[i];
                        }
                        // return
                        return std::make_tuple(_shape, buffer);
                    }
                    else
                    {
                        Type buffer(size);

                        // get type the attribute has internally
                        hid_t type = H5Aget_type(_attribute);

                        H5Aread(_attribute, type, buffer.data());
                        return std::make_tuple(_shape, buffer);
                    }
                }
            }
        }
        else if constexpr (std::is_same_v<Type, std::string> ||
                           std::is_same_v<Type, const char*>)
        {
            // get type the attribute has internally
            hid_t type = H5Aget_type(_attribute);

            // get size type
            size = H5Tget_size(type);

            // read data into it and return
            std::string buffer;
            buffer.resize(size);

            // read data
            H5Aread(_attribute, type, buffer.data());

            // return as tuple
            return std::make_tuple(_shape, buffer);
        }
        else
        {
            // get type the attribute has internally
            hid_t type = H5Aget_type(_attribute);

            Type buffer;
            H5Aread(_attribute, type, &buffer);
            // return as tuple
            return std::make_tuple(_shape, buffer);
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
    void write(Type attribute_data,
               std::vector<hsize_t> shape = {},
               [[maybe_unused]] std::size_t typelen = 0)
    {
        // using result_type = typename
        // HDFTypeFactory::result_type<Type>::type; when stuff is vector
        // string we can write directly, otherwise we have to buffer

        // check if we have a container. Writing containers requires further
        // t tests, plain old data can be written right away
        if constexpr (is_container_type<Type>::value)
        {
            if (shape.size() == 0)
            {
                _shape = {attribute_data.size()};
            }
            else
            {
                _shape = shape;
            }
            // are consecutive memory types, namely std::vector and ptr
            // types. exclude however containers of containers because they
            // have to be buffered due to hdf5 internals
            if constexpr (std::is_same_v<Type, std::vector<typename Type::value_type>> &&
                          !is_container_type<typename Type::value_type>::value &&
                          !std::is_same_v<typename Type::value_type, std::string>)
            {
                // std::cout << "of size: " << _size << std::endl;
                // check if attribute has been created, else do
                if (_attribute == -1)
                {
                    _attribute =
                        __make_attribute__<typename HDFTypeFactory::result_type<typename Type::value_type>::type>(
                            typelen);
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename HDFTypeFactory::result_type<typename Type::value_type>::type>(
                             typelen),
                         attribute_data.data());
            }
            else
            {
                auto buffer = HDFBufferFactory::buffer(
                    std::begin(attribute_data),
                    std::end(attribute_data), [](auto& value) -> typename Type::value_type& {
                        return value;
                    });

                if (_attribute == -1)
                {
                    _attribute =
                        __make_attribute__<typename HDFTypeFactory::result_type<typename Type::value_type>::type>(
                            typelen);
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename HDFTypeFactory::result_type<typename Type::value_type>::type>(
                             typelen),
                         buffer.data());
            }
        }
        else if constexpr (std::is_pointer_v<Type>)
        {
            if constexpr (std::is_same_v<Type, const char*>)
            {
                auto len = std::strlen(attribute_data);
                _shape = {1};
                // check if attribute has been created, else do
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>(len);
                }

                H5Awrite(_attribute, HDFTypeFactory::type<Type>(len), attribute_data);
            }
            else
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
                using basetype = typename HDFTypeFactory::result_type<Type>::type;

                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<basetype>(typelen);
                }

                H5Awrite(_attribute, HDFTypeFactory::type<basetype>(typelen), attribute_data);
            }
        }
        else
        {
            if (shape.size() == 0)
            {
                _shape = {1};
            }
            else
            {
                _shape = shape;
            }

            if constexpr (std::is_same_v<typename HDFTypeFactory::result_type<Type>::type, std::string>)
            {
                if (_attribute == -1)
                {
                    _attribute =
                        __make_attribute__<typename HDFTypeFactory::result_type<Type>::type>(
                            attribute_data.size());
                }
                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename HDFTypeFactory::result_type<Type>::type>(
                             attribute_data.size()),
                         attribute_data.c_str());
            }
            else
            {
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>();
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename HDFTypeFactory::result_type<Type>::type>(),
                         &attribute_data);
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute
     *
     * @tparam Iter Iterator type
     * @tparam Adaptor Adaptor which allows for extraction of a value from compound types, i.e. classes or structs
     * @param begin
     * @param end
     * @param adaptor
     */
    template <typename Iter, typename Adaptor>
    void write(Iter begin,
               Iter end,
               Adaptor adaptor = [](auto& value) { return value; },
               std::vector<hsize_t> shape = {},
               [[maybe_unused]] std::size_t typelen = 0)
    {
        using Type = typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;

        // if we copy only the content of [begin, end), then simple vector
        // copy suffices
        if constexpr (std::is_same<Type, typename Iter::value_type>::value)
        {
            write(std::vector<Type>(begin, end), typelen);
        }
        else
        {
            // buffer data, have to because we cannot write iterator ranges
            write(HDFBufferFactory::buffer(begin, end, adaptor), shape, typelen);
        }
    }

    /**
     * @brief Default constructor, deleted because of reference member
     *
     */
    HDFAttribute() = delete;

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
    HDFAttribute(HDFAttribute&& other)
        : _attribute(std::move(other._attribute)),
          _name(std::move(other._name)),
          _shape(std::move(other._shape)),
          _parent_object(std::move(other._parent_object))
    {
    }

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return HDFAttribute&
     */
    HDFAttribute& operator=(const HDFAttribute& other)
    {
        _attribute = other._attribute;
        _name = other._name;
        _shape = other._shape;
        _parent_object = other._parent_object;
        return *this;
    }

    /**
     * @brief Move assignment operator
     *
     * @param other
     * @return HDFAttribute&
     */
    HDFAttribute& operator=(HDFAttribute&& other)
    {
        _attribute = std::move(other._attribute);
        _name = std::move(other._name);
        _shape = std::move(other._shape);
        _parent_object = std::move(other._parent_object);
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
        : _name(name), _shape(std::vector<hsize_t>()), _parent_object(object)
    {
        // checks the validity and opens attribute if possible, else
        // postphones until it is written
        if (H5Iis_valid(_parent_object.get().get_id()) == false)
        {
            throw std::invalid_argument(
                "parent_object of attribute " + _name +
                "is invalid, has it been closed already?");
        }
        else
        {
            if (H5LTfind_attribute(object.get_id(), _name.c_str()) == 1)
            { // attribute exists
                _attribute = H5Aopen(object.get_id(), _name.c_str(), H5P_DEFAULT);
            }
            else
            { // attribute does not exist: make
                _attribute = -1;
            }
        }
    }
}; // namespace DataIO

} // namespace DataIO
} // namespace Utopia
#endif