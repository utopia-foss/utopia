#ifndef HDFATTRIBUTE_HH
#define HDFATTRIBUTE_HH
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"

#include <functional>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <memory>
#include <string>
namespace Utopia {
namespace DataIO {

/**
 * @brief Class for hdf5 attribute, which can be attached to groups and
 * datasets.

 */
template <typename HDFObject, typename Attrtype> class HDFAttribute {
private:
    // helper for making attribute

    /**
     * @brief private helper function for creation of attribute
     *
     * @tparam result_type
     * @return hid_t
     */
    template <typename result_type>
    hid_t __make_attribute__(hsize_t typesize = 0) {
        hid_t dspace = H5Screate_simple(1, &_size, nullptr);
        return H5Acreate2(_parent_object.get_id(), _name.c_str(),
                          HDFTypeFactory::type<result_type>(typesize), dspace,
                          H5P_DEFAULT, H5P_DEFAULT);
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
    std::shared_ptr<HDFObject> _parent_object;

public:
    /**
     * @brief Get underlying id of attribute
     *
     * @return hid_t underlying HDF5 id.
     */
    hid_t get_id() { return _attribute; }
    /**
     * @brief get Attribute name
     *
     * @return std::string
     */
    std::string get_name() { return _name; }

    /**
     * @brief Get the hdf5 object to which the attribute belongs
     *
     * @return weak pointer to HDFObject
     */
    HDFObject &get_parent() { return _parent_object; }

    void close() {
        if (_attribute != -1) {
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
    template <
        typename Type = Attrtype,
        std::enable_if_t<is_container<Type>::value == true &&
                             std::is_same<Type, std::string>::value == false,
                         int> = 0>
    std::vector<Type> read() {
        if (_attribute == -1) {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute");
        } else {
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

    /**
     * @brief Overload for strings
     *
     * @tparam Attrtype
     * @tparam 0
     * @return std::vector<Type>
     */
    template <
        typename Type = Attrtype,
        std::enable_if_t<is_container<Type>::value == true &&
                             std::is_same<Type, std::string>::value == true,
                         int> = 0>
    std::string read() {
        if (_attribute == -1) {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute");
        } else {
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
    }

    /**
     * @brief read primitive type
     *
     * @tparam Attrtype
     * @tparam 0
     * @return Type
     */
    template <typename Type = Attrtype,
              std::enable_if_t<is_container<Type>::value == false, int> = 0>
    Type read() {
        if (_attribute == -1) {
            throw std::runtime_error(
                "trying to read a nonexstiant or closed attribute");
        } else {
            // get type the attribute has internally
            hid_t type = H5Aget_type(_attribute);
            // make a vector of the corresponding C type

            Type data;

            // read data into it and return

            H5Aread(_attribute, type, &data);
            return data;
        }
    }

    /**
     * @brief Function for writing data to the dataset
     *
     * @param begin
     * @param end
     * @param adaptor
     */
    template <typename Type = Attrtype,
              std::enable_if_t<is_container<Type>::value == true, int> = 0>
    void write(Type &attribute_data) {

        // when stuff is vector string we can write directly, otherwise we
        // have to buffer
        if (std::is_same<Type, std::string>::value ||
            std::is_same<Type, std::vector<typename Type::value_type>>::value) {
            if (_attribute == -1) {
                _attribute = __make_attribute__<Type>(attribute_data.size());
            }
            H5Awrite(_attribute,
                     HDFTypeFactory::type<Type>(attribute_data.size()),
                     attribute_data.data());
            // when stuff is not a vector or string we first have to buffer
        } else {

            auto buffer = HDFBufferFactory::buffer(
                std::begin(attribute_data), std::end(attribute_data),
                [](auto &value) { return value; });
            if (_attribute == -1) {
                _attribute =
                    __make_attribute__<typename decltype(buffer)::value_type>(
                        attribute_data.size());
            }
            H5Awrite(
                _attribute,
                HDFTypeFactory::type<typename decltype(buffer)::value_type>(
                    attribute_data.size()),
                attribute_data.data());
        }
    }

    /**
     * @brief overload for primitive types
     *
     * @tparam 0
     * @param attribute_data
     */
    template <typename Type = Attrtype,
              std::enable_if_t<is_container<Type>::value == false, int> = 0>
    void write(Type &attribute_data) {
        if (_attribute == -1) {
            _attribute = __make_attribute__<Type>();
        }
        H5Awrite(_attribute, HDFTypeFactory::type<Type>(), &attribute_data);
    }

    /**
     * @brief Default constructor
     *
     */
    HDFAttribute() = delete;

    /**
     * @brief Copy constructor
     *
     * @param other
     */
    HDFAttribute(const HDFAttribute &other)
        : _attribute(other._attribute), _name(other._name), _size(other._size),
          _parent_object(other._parent_object) {}
    /**
     * @brief Move constructor
     *
     * @param other
     */
    HDFAttribute(HDFAttribute &&other) = delete;

    /**
     * @brief Assignment operator
     *
     * @param other
     * @return HDFAttribute&
     */
    HDFAttribute &operator=(HDFAttribute &&other) = delete;
    /**
     * @brief Destructor
     *
     */
    virtual ~HDFAttribute() {
        // if (H5Iis_valid(_attribute) > 0)
        if (H5Iis_valid(_attribute) ==
            0) { // FIXME: add check to make sure that
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

    HDFAttribute(HDFObject &object, std::string name)
        : _name(name), _size(1),
          _parent_object(std::make_shared<HDFObject>(object)) {

        if (H5LTfind_attribute(object.get_id(), _name.c_str()) ==
            1) { // attribute exists
            _attribute = H5Aopen(object.get_id(), _name.c_str(), H5P_DEFAULT);
        } else { // attribute does not exist: make
            _attribute = -1;
        }
    }
};

} // namespace DataIO
} // namespace Utopia
#endif