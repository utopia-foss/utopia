#ifndef HDFGROUP_HH
#define HDFGROUP_HH
#include "hdfattribute.hh"

#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>

namespace Utopia {
namespace DataIO {
class HDFFile; // forward declaration because needed
class HDFGroup {
private:
protected:
    hid_t _group;
    std::string _path;
    std::shared_ptr<HDFFile> _parent_file;

public:
    void swap(HDFGroup &other) {
        using std::swap;
        swap(_group, other._group);
        swap(_path, other._path);
        swap(_parent_file, other._parent_file);
    }
    std::string get_path() { return _path; }
    // id
    hid_t get_id() { return _group; }

    auto get_parent_file() { return _parent_file; }
    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data) {

        HDFAttribute<HDFGroup, Attrdata> attribute(
            *this, std::forward<std::string &&>(name));

        attribute.write(attribute_data);
    }

    void close() {
        if (H5Iis_valid(_group) == true) {
            H5Gclose(_group);
            _group = -1;
        }
    }

    // default constructor
    HDFGroup() = default;
    HDFGroup(const HDFGroup &other)
        : _group(other._group), _path(other._path) {}

    HDFGroup(HDFGroup &&other) : HDFGroup() { this->swap(other); }

    HDFGroup &operator=(HDFGroup other) {
        this->swap(other);
        return *this;
    }

    // constructor
    template <typename HDFObject>
    HDFGroup(HDFObject &object, HDFFile &parent_file, std::string name)
        : _path(name), _parent_file(std::make_shared<HDFFile>(parent_file)) {
        if (std::is_same<HDFObject, HDFFile>::value) {
            if (_path == "/") {
                _group = H5Gopen(object.get_id(), "/", H5P_DEFAULT);

            } else {
                if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) >
                    0) {
                    _group =
                        H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
                } else {
                    _group = H5Gcreate(object.get_id(), _path.c_str(),
                                       H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                }
            }
        } else if (std::is_same<HDFObject, HDFGroup>::value) {
            if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) > 0) {
                _group = H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
            } else {
                _group = H5Gcreate(object.get_id(), _path.c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);
            }
        }
    }

    // destructor
    ~HDFGroup() {
        if (H5Iis_valid(_group) == true) {
            H5Gclose(_group);
        }
    }
};

void swap(HDFGroup &lhs, HDFGroup &rhs) { lhs.swap(rhs); }
} // namespace DataIO
} // namespace Utopia
#endif