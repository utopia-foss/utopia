#ifndef HDFATTRIBUTE_HH
#define HDFATTRIBUTE_HH
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"

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
 * @brief      Class for hdf5 attribute, which can be attached to groups and
 *             datasets.
 *
 * @tparam     HDFObject  the object class
 */
template <typename HDFObject>
class HDFAttribute
{
private:
    /**
     * @brief      private helper function for creation of attribute
     *
     * @param[in]  typesize     The typesize
     *
     * @tparam     result_type  Type of the resulting attribute
     *
     * @return     hid_t
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
     * @brief      ID of attribute itself
     */
    hid_t _attribute;

    /**
     * @brief      name of the attribute
     */
    std::string _name;

    /**
     * @brief      size of the attributes dataspace
     */
    hsize_t _size;
    /**
     * @brief      reference to id of parent object: dataset or group
     */
    std::reference_wrapper<HDFObject> _parent_object;

public:
    /**
     * @brief      Get underlying ID of attribute
     *
     * @return     hid_t underlying HDF5 ID
     */
    hid_t get_id()
    {
        return _attribute;
    }
    /**
     * @brief      get attribute name
     *
     * @return     std::string the name of the attribute
     */
    std::string get_name()
    {
        return _name;
    }

    /**
     * @brief      Get the hdf5 object to which the attribute belongs
     *
     * @return     weak pointer to HDFObject
     */
    HDFObject& get_parent()
    {
        return _parent_object.get();
    }

    /**
     * @brief      closes the attribute
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
     * @brief      For reading an attribute back
     *
     * @tparam     Type  the data type of the attribute
     *
     * @return     the content of the attribute
     */
    template <typename Type>
    auto read()
    {
        if (_attribute == -1)
        {
            throw std::runtime_error("cannot read the non-existent or closed "
                                     "attribute '" + _name + "'");
        }

        if constexpr (is_container_type<Type>::value)
        {
            if constexpr (std::is_same_v<Type, std::string>)
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);
                _size = H5Tget_size(type);

                // create a buffer string of the appropriate size
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
                throw std::runtime_error("cannot read the non-existent or "
                                         "closed attribute '" + _name + "'");
            }
            else
            {
                // get type the attribute has internally
                hid_t type = H5Aget_type(_attribute);

                // read data into it and return
                Type data;
                H5Aread(_attribute, type, &data);
                return data;
            }
        }
    }

    /**
     * @brief      Function for writing data to the dataset
     *
     * @tparam     Type            the data type of the attribute
     *
     * @param      attribute_data  the data to write into the attribute
     */
    template <typename Type>
    void write(Type& attribute_data)
    {
        // using result_type = typename HDFTypeFactory::result_type<Type>::type;

        // Distinguish different attribute data types
        // When the data is a vector string, we can write directly, otherwise
        // we have to buffer
        if constexpr (is_container_type<Type>::value)
        {  // is a container type -> distinguish strings and other types

            // If not yet existing, create the attribute
            if (_attribute == -1)
            {
                _attribute = __make_attribute__<Type>(attribute_data.size());
            }

            if constexpr (std::is_same_v<Type, std::string>)
            { // is a string -> can write directly
                H5Awrite(_attribute,
                         HDFTypeFactory::type<Type>(attribute_data.size()),
                         attribute_data.data());
            }
            else
            { // not a vector or string -> we first have to buffer
                // create the buffer
                auto buffer = HDFBufferFactory::buffer(
                    std::begin(attribute_data),
                    std::end(attribute_data),
                    [](auto& value) { return value; }
                );

                // If not yet existing, create the attribute
                // FIXME is this needed here? Should have been created already
                //       above, right?
                if (_attribute == -1)
                {
                    _attribute = __make_attribute__<Type>(attribute_data.size());
                }

                // Write, using the buffer
                H5Awrite(_attribute,
                         HDFTypeFactory::type<Type>(attribute_data.size()),
                         buffer.data());
            }
        }
        else
        { // is not a container type -> can write directly
            // If not yet existing, create the attribute
            if (_attribute == -1)
            {
                _attribute = __make_attribute__<Type>();
            }

            // Write the data to the attribute
            H5Awrite(_attribute,
                     HDFTypeFactory::type<Type>(),
                     &attribute_data);
        }
    }

    /**
     * @brief      Default constructor
     */
    HDFAttribute() = delete;

    /**
     * @brief      Copy constructor
     *
     * @param      other  The other
     */
    HDFAttribute(const HDFAttribute& other)
        : _attribute(other._attribute),
          _name(other._name),
          _size(other._size),
          _parent_object(other._parent_object)
    {
    }

    /**
     * @brief      Move constructor
     *
     * @param      other  The other
     */
    HDFAttribute(HDFAttribute&& other)
        : _attribute(std::move(other._attribute)),
          _name(std::move(other._name)),
          _size(std::move(other._size)),
          _parent_object(std::move(other._parent_object))
    {
    }

    /**
     * @brief      Assignment operator
     *
     * @param      other  The other
     *
     * @return     HDFAttribute&
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
     * @brief      Move assignment operator
     *
     * @param      other The other
     *
     * @return     HDFAttribute&
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
     * @brief      Destructor
     */
    virtual ~HDFAttribute()
    {
        if (H5Iis_valid(_attribute))
        { // FIXME: add check to make sure that
          // it is not closed more than once
            H5Aclose(_attribute);
        }
    }

    /**
     * @brief      Constructor for attribute
     *
     * @param      object  the object to create the attribute at
     * @param      name    the name of the attribute
     * @param      size    the size of the attribute if known, else 1
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
                _attribute = H5Aopen(object.get_id(), _name.c_str(),
                                     H5P_DEFAULT);
            }
            else
            { // attribute does not exist: make
                _attribute = -1;
            }
        }
    }
};

} // namespace DataIO
} // namespace Utopia
#endif
