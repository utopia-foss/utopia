#ifndef HDFMOCKCLASSES_HH
#define HDFMOCKCLASSES_HH
#include "hdfattribute.hh"

// this header has to be removed later because it is only there for
// testing hdfattributes, will be later replaced by real stuff
namespace Utopia {
namespace DataIO {
// class HDFFile;
// mock class for group

// class HDFGroup {
// private:
// protected:
//     hid_t _group;
//     std::string _path;

// public:
//     std::string get_path() { return _path; }
//     // id
//     hid_t get_id() { return _group; }

//     template <typename Attrdata>
//     void add_attribute(std::string name, Attrdata attribute_data) {

//         HDFAttribute<HDFGroup, Attrdata> attribute(
//             *this, std::forward<std::string &&>(name));

//         attribute.write(attribute_data);
//     }

//     void close() { H5Gclose(_group); }

//     // default constructor
//     HDFGroup() = default;
//     HDFGroup(const HDFGroup &other)
//         : _group(other._group), _path(other._path) {}

//     HDFGroup &operator=(const HDFGroup &other) {
//         _group = other._group;
//         _path = other._path;
//         return *this;
//     }
//     // constructor
//     template <typename HDFObject>
//     HDFGroup(HDFObject &object, std::string name) : _path(name) {
//         if (std::is_same<HDFObject, HDFFile>::value) {
//             if (_path == "/") {
//                 _group = H5Gopen(object.get_id(), "/", H5P_DEFAULT);

//             } else {
//                 if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) >
//                     0) {
//                     _group =
//                         H5Gopen(object.get_id(), _path.c_str(), H5P_DEFAULT);
//                 } else {
//                     _group = H5Gcreate(object.get_id(), _path.c_str(),
//                                        H5P_DEFAULT, H5P_DEFAULT,
//                                        H5P_DEFAULT);
//                 }
//             }
//         } else if (std::is_same<HDFObject, HDFGroup>::value) {
//             if (H5Lexists(object.get_id(), _path.c_str(), H5P_DEFAULT) > 0) {
//                 _group = H5Gopen(object.get_id(), _path.c_str(),
//                 H5P_DEFAULT);
//             } else {
//                 _group = H5Gcreate(object.get_id(), _path.c_str(),
//                 H5P_DEFAULT,
//                                    H5P_DEFAULT, H5P_DEFAULT);
//             }
//         }
//     }

//     // destructor
//     ~HDFGroup() {
//         if (H5Iis_valid(_group) == 0) {
//             H5Gclose(_group);
//         }
//     }
// };

// // mock class for file
// class HDFFile {
// protected:
//     hid_t _file;
//     std::shared_ptr<HDFGroup> _base_group;

// public:
//     // id
//     hid_t get_id() { return _file; }

//     // basegroup getter
//     HDFGroup &get_basegroup() { return *_base_group; }

//     void close() {
//         H5Fflush(_file, H5F_SCOPE_GLOBAL);
//         H5Fclose(_file);
//     }

//     // Constructor
//     HDFFile(std::string name, std::string access)
//         : _file([&]() {
//               if (access == "w") {
//                   return H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
//                                    H5P_DEFAULT);
//               } else if (access == "r") {
//                   return H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
//               } else {
//                   throw std::runtime_error("wrong access specifier");
//               }
//           }()),
//           _base_group(std::make_shared<HDFGroup>(*this, "/")) {}

//     // Destructor
//     ~HDFFile() {
//         H5Fflush(_file, H5F_SCOPE_GLOBAL);
//         if (H5Iis_valid(_file) == true) {
//             H5Fclose(_file);
//         }
//     }
// };
} // namespace DataIO
} // namespace Utopia
#endif