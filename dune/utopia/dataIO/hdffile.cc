#include "hdffile.hh"

namespace Utopia {
namespace DataIO {

// constructor for the file itself
HDFFile::HDFFile(std::string &&path, std::string &&access_mode) {
    // check each time if the data exists
    if (access_mode == "r") { // read only, file must exist
        _file_id = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    } else if (access_mode == "r+") { // read-write, file must exist
        _file_id = H5Fopen(path.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    } else if (access_mode == "x") { // create file, fails if exist
        _file_id =
            H5Fcreate(path.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
    } else if (access_mode == "w") { // create file, truncate if exists
        _file_id =
            H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    } else {
        throw std::runtime_error(
            "wrong access modifier, has to be r, r+, a, w");
    }

    _base_group = HDFGroup(*this, "/");
}

// copy constructor
HDFFile::HDFFile(const HDFFile &other)
    : _file_id(other._file_id), _base_group(other._base_group) {}

// move constructor
HDFFile::HDFFile(HDFFile &&other) : HDFFile() { this->swap(other); }

// copy and move assignment
HDFFile &HDFFile::operator=(HDFFile other) {
    this->swap(other);
    return *this;
}

// destructor
HDFFile::~HDFFile() {
    flush();
    _base_group.~HDFGroup();
    H5Fclose(_file_id);
}

// for opening groups
HDFGroup &HDFFile::open_group() { return _base_group; }

// flushes the entire virtual file
void HDFFile::flush() { H5Fflush(_file_id, H5F_SCOPE_GLOBAL); }

// swap function for exchanging states
void HDFFile::swap(HDFFile &other) {
    using Utopia::DataIO::swap;
    using std::swap;
    swap(_file_id, other._file_id);
    swap(_base_group, other._base_group);
}
} // namespace DataIO
} // namespace Utopia