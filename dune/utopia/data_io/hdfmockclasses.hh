#ifndef HDFMOCKCLASSES_HH
#define HDFMOCKCLASSES_HH
#include "hdfattribute.hh"
#include "hdfgroup.hh"

// this header has to be removed later because it is only there for 
// testing hdfattributes, will be later replaced by real stuff
namespace Utopia{
    namespace DataIO{
class HDFGroup;

// mock class for file
class HDFFile {
protected:
    hid_t _file;
    std::shared_ptr<HDFGroup> _base_group;

public:
    // id
    hid_t get_id() { return _file; }

    // basegroup getter
    HDFGroup &get_basegroup() { return *_base_group; }

    void close() {
        H5Fflush(_file, H5F_SCOPE_GLOBAL);
        H5Fclose(_file);
    }

    // Constructor
    HDFFile(std::string name, std::string access)
        :_file([&](){if(access == "w"){
               return  H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            }
            else if(access == "r"){
                 return H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            }
            else{
                throw std::runtime_error("wrong access specifier");
            }}()),_base_group(std::make_shared<HDFGroup>(*this, "/")) {}

    // Destructor
    ~HDFFile() {
        H5Fflush(_file, H5F_SCOPE_GLOBAL);
        H5Fclose(_file);
    }
};
}
}
#endif