#include "hdfattribute.hh"
namespace Utopia {
namespace DataIO {
/**
 * @brief private helper function for creation of attribute
 *
 * @tparam result_type
 * @return hid_t
 */
template <typename result_type> hid_t HDFAttribute::__make_attribute__() {
    hid_t dspace = H5Screate_simple(1, &_size, nullptr);
    return H5Acreate2(*_parent_object, _name.c_str(),
                      HDFTypeFactory::type<result_type>(), dspace, H5P_DEFAULT,
                      H5P_DEFAULT);
}

/**
 * @brief private helper fucntion for writing complicated data
 *
 * @tparam result_type
 * @tparam Iter
 * @tparam Adaptor
 * @param begin
 * @param end
 * @param adaptor
 */
template <typename result_type, typename Iter, typename Adaptor>
void HDFAttribute::__write_data__(Iter begin, Iter end, Adaptor &&adaptor) {
    using value_type = typename Iter::value_type;
    if (std::is_same<Iter, typename std::vector<value_type>::iterator>::value ==
            true &&
        is_container<value_type>::value == false) {
        H5Awrite(_attribute, HDFTypeFactory::type<result_type>(), &(*begin));
    }
    // not a vector of primitive data
    else {
        auto buffer = HDFBufferFactory::buffer(begin, end, adaptor);
        H5Awrite(_attribute,
                 HDFTypeFactory::type<typename decltype(buffer)::value_type>(),
                 buffer.data());
    }
}

/**
 * @brief Get underlying id of attribute
 *
 * @return hid_t underlying HDF5 id.
 */
hid_t HDFAttribute::get_id() { return _attribute; }

/**
 * @brief get Attribute name
 *
 * @return std::string
 */
std::string HDFAttribute::get_name() { return _name; }

/**
 * @brief Function for writing data to the dataset, optionally using an
 *        adaptor function for manipulating the data before writing
 *
 * @tparam Iter Iterator
 * @tparam Adaptor  function (type(typename Iter::value_type))
 * @param begin
 * @param end
 * @param adaptor
 */
template <typename Iter, typename Adaptor>
void HDFAttribute::write(Iter begin, Iter end, Adaptor &&adaptor) {
    _size = std::distance(begin, end);
    using result_type = std::decay_t<decltype(adaptor(*begin))>;

    // attribute does not yet exist
    if (_attribute == -1) {
        // create attribute large enough, then write
        _attribute = __make_attribute__<result_type>();

        __write_data(std::forward<Iter>(begin), std::forward<Iter>(end),
                     std::forward<Adaptor>(adaptor));

    }
    // attribute does exist
    else {
        // check dimension, if not large enough delete attribute, create
        // anew and write
        if (H5Sget_simple_extent_ndims(H5Aget_space(_attribute)) != _size) {
            // delete attribute, and then build a new one with the correct
            // size

            herr_t err = H5Adelete(_attribute, _name.c_str());
            if (err < 0) {
                throw std::runtime_error("something went wrong when deleting "
                                         "attribute of wrong size");
            } else {
                _attribute = __make_attribute__<result_type>();
                __write_data(std::forward<Iter>(begin), std::forward<Iter>(end),
                             std::forward<Adaptor>(adaptor));
            }
        } else {
            __write_data(std::forward<Iter>(begin), std::forward<Iter>(end),
                         std::forward<Adaptor>(adaptor));
        }
    }
}

/**
 * @brief Function for writing a simple attribute type, like a single bool or
 *        a string.
 *
 * @tparam Attrtype
 * @param attribute_data
 */
template <typename Attrtype> void HDFAttribute::write(Attrtype attribute_data) {
    using result_type = Attrtype;
    // attribute does not yet exist
    _size = 1;
    if (_attribute == -1) {
        // create attribute large enough, then write
        _attribute = __make_attribute__<result_type>();

        H5Awrite(_attribute, HDFTypeFactory::type<Attrtype>(), &attribute_data);

    }
    // attribute does exist
    else {
        // check dimension, if not large enough delete attribute, create
        // anew and write
        if (H5Sget_simple_extent_ndims(H5Aget_space(_attribute)) != _size) {
            // delete attribute, and then build a new one with the correct
            // size

            herr_t err = H5Adelete(_attribute, _name.c_str());
            if (err < 0) {
                throw std::runtime_error("something went wrong when deleting "
                                         "attribute of wrong size");
            } else {
                _attribute = __make_attribute__<result_type>();
                H5Awrite(_attribute, HDFTypeFactory::type<Attrtype>(),
                         &attribute_data);
            }
        } else {
            H5Awrite(_attribute, HDFTypeFactory::type<Attrtype>(),
                     &attribute_data);
        }
    }
}

// FIXME: merge this into the above
template <> void HDFAttribute::write<std::string>(std::string attribute_data) {
    using result_type = std::string;

    _size = 1;
    hid_t type = HDFTypeFactory::type<std::string>(attribute_data.size());
    if (_attribute == -1) {
        // create attribute large enough, then write
        _attribute = __make_attribute__<result_type>();

        H5Awrite(_attribute, type, &attribute_data);

    }
    // attribute does exist
    else {
        // check dimension, if not large enough delete attribute, create
        // anew and write
        if (H5Sget_simple_extent_ndims(H5Aget_space(_attribute)) != _size) {
            // delete attribute, and then build a new one with the correct
            // size

            herr_t err = H5Adelete(_attribute, _name.c_str());
            if (err < 0) {
                throw std::runtime_error("something went wrong when deleting "
                                         "attribute of wrong size");
            } else {
                _attribute = __make_attribute__<result_type>();
                H5Awrite(_attribute, type, &attribute_data);
            }
        } else {
            H5Awrite(_attribute, type, &attribute_data);
        }
    }
}

/**
 * @brief Function for exchanging states between two HDFAttribute objects
 *
 * @param other
 */
void HDFAttribute::swap(HDFAttribute &other) {
    using std::swap;
    swap(_attribute, other._attribute);
    swap(_name, other._name);
    swap(_size, other._size);
    swap(_parent_object, other._parent_object);
}

/**
 * @brief Copy constructor
 *
 * @param other
 */
HDFAttribute::HDFAttribute(const HDFAttribute &other)
    : _attribute(other._attribute), _name(other._name), _size(other._size),
      _parent_object(other._parent_object) {}

/**
 * @brief Default constructor
 *
 */
HDFAttribute::HDFAttribute()
    : _attribute(-1), _name(""), _size(0), _parent_object(nullptr) {}

/**
 * @brief Constructor
 *
 * @tparam HDFObject
 * @param object
 * @param name
 * @param size
 */
template <typename HDFObject>
HDFAttribute::HDFAttribute(const HDFObject &object, std::string &&name,
                           hsize_t size)
    : _attribute(-1), _name(name), _size(size),
      _parent_object(&object.get_id()) {
    {

        if (H5LTfind_attribute(object.get_id(), _name.c_str()) ==
            1) { // attribute exists
            _attribute = H5Aopen(object.get_id(), _name.c_str(), H5P_DEFAULT);
        } else { // attribute does not exists, creation postphoned to writing
        }
    }
}

/**
 * @brief Assignment operator
 *
 * @param other
 * @return HDFAttribute&
 */
HDFAttribute &HDFAttribute::operator=(HDFAttribute other) {
    this->swap(other);
    return *this;
}

} // namespace DataIO
} // namespace Utopia