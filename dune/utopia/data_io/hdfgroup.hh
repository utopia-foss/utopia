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

public:
    void swap(HDFGroup &other) {
        using std::swap;
        swap(_group, other._group);
        swap(_path, other._path);
    }
    std::string get_path() { return _path; }
    // id
    hid_t get_id() { return _group; }

    template <typename Attrdata>
    void add_attribute(std::string name, Attrdata attribute_data) {

        HDFAttribute<HDFGroup, Attrdata> attribute(
            *this, std::forward<std::string &&>(name));

        attribute.write(attribute_data);
    }

    void close() { H5Gclose(_group); }

    // default constructor
    HDFGroup() = default;
    HDFGroup(const HDFGroup &other)
        : _group(other._group), _path(other._path) {}

    HDFGroup &operator=(const HDFGroup &other) {
        _group = other._group;
        _path = other._path;
        return *this;
    }
    // constructor
    template <typename HDFObject>
    HDFGroup(HDFObject &object, std::string name) : _path(name) {
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
        if (H5Iis_valid(_group) == 0) {
            H5Gclose(_group);
        }
    }
};

void swap(HDFGroup &lhs, HDFGroup &rhs) { lhs.swap(rhs); }
} // namespace DataIO
} // namespace Utopia
#endif