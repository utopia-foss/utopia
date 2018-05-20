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

// FIXME: writing ndimensional attribute is still missing!

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
        hid_t dspace = H5Screate_simple(1, &_size, nullptr);

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
    hsize_t _size;
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
     * @brief For reading an attribute back
     *
     * @tparam Iter
     * @tparam Adaptor
     * @param begin
     * @param end
     * @param adaptor
     */
    template <typename Type>
    auto read()
    {
        if (_attribute == -1)
        {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute named '" +
                _name + "'");
        }
        if constexpr (is_container_type<Type>::value || std::is_same_v<Type, const char*>)
        {
            if constexpr (std::is_same_v<Type, std::string> ||
                          std::is_same_v<Type, const char*>)
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);
                _size = H5Tget_size(type);
                // make a vector of the corresponding C type

                std::string buffer;
                buffer.resize(_size);
                // read data into it and return

                H5Aread(_attribute, type, buffer.data());
                return buffer;
            }
            else
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);
                _size = H5Tget_size(type);
                // make a vector of the corresponding C type
                std::vector<Type> buffer(_size);

                // read data into it and return

                H5Aread(_attribute, type, buffer.data());
                return buffer;
            }
        }
        else
        {
            if (_attribute == -1)
            {
                throw std::runtime_error(
                    "trying to read a nonexstiant or closed attribute named '" +
                    _name + "'");
            }
            else
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);
                // make a vector of the corresponding C type

                Type data;

                // read data into it and return

                H5Aread(_attribute, type, &data);
                return data;
            }
        }
    }

    /**
     * @brief Function for writing data to the attribute
     *
     * @param attribute_data Data to write
     * @param len of arrays if a container of arrays shall be written which are all the same length
     * @param ptrlen arraysize in case a pointer is given to write
     */
    template <typename Type>
    void write(Type attribute_data,
               [[maybe_unused]] std::size_t arraylen = 0,
               [[maybe_unused]] std::size_t ptrlen = 0)
    {
        // using result_type = typename HDFTypeFactory::result_type<Type>::type;
        // when stuff is vector string we can write directly, otherwise we
        // have to buffer

        // check if we have a container. Writing containers requires further t
        // tests, plain old data can be written right away
        if constexpr (is_container_type<Type>::value)
        {
            _size = attribute_data.size();
            // are consecutive memory types, namely std::vector and ptr types.
            // exclude however containers of containers because they have to
            // be buffered due to hdf5 internals
            if constexpr (std::is_same_v<Type, std::vector<typename Type::value_type>> &&
                          !is_container_type<typename Type::value_type>::value)
            {
                // std::cout << "writing vector for " << _name << std::endl;
                // std::cout << "of size: " << _size << std::endl;
                // check if attribute has been created, else do
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<typename Type::value_type>(arraylen);
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename Type::value_type>(arraylen),
                         attribute_data.data());
            }
            else if constexpr (std::is_pointer_v<Type>)
            {
                if (_attribute == -1)
                {
                    _attribute =
                        __make_attribute__<typename HDFTypeFactory::result_type<Type>::type>(arraylen);
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename HDFTypeFactory::result_type<Type>::type>(arraylen),
                         attribute_data);
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
                    _attribute = __make_attribute__<typename Type::value_type>(arraylen);
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<typename Type::value_type>(arraylen),
                         buffer.data());
            }
        }
        else
        {
            if constexpr (std::is_same_v<Type, std::string>)
            {
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>(attribute_data.size());
                }
                H5Awrite(_attribute, HDFTypeFactory::type<Type>(attribute_data.size()),
                         attribute_data.c_str());
            }
            else if constexpr (std::is_same_v<Type, const char*>)
            {
                // check if attribute has been created, else do
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>(std::strlen(attribute_data));
                }

                H5Awrite(_attribute,
                         HDFTypeFactory::type<Type>(std::strlen(attribute_data)),
                         attribute_data);
            }
            else
            {
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>();
                }

                H5Awrite(_attribute, HDFTypeFactory::type<Type>(), &attribute_data);
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
               [[maybe_unused]] std::size_t arraylen = 0)
    {
        using Type = typename HDFTypeFactory::result_type<decltype(adaptor(*begin))>::type;

        // if we copy only the content of [begin, end), then simple vector copy
        // suffices
        if constexpr (std::is_same<Type, typename Iter::value_type>::value)
        {
            write(std::vector<Type>(begin, end), arraylen);
        }
        else
        {
            // buffer data, have to because we cannot write iterator ranges
            write(HDFBufferFactory::buffer(begin, end, adaptor), arraylen);
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
          _size(other._size),
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
          _size(std::move(other._size)),
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
        _size = other._size;
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
        _size = std::move(other._size);
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
        : _name(name), _size(1), _parent_object(object)
    {
        // checks the validity and opens attribute if possible, else postphones
        // until it is written
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