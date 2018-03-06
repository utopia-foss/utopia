#ifndef HDFFILE_HH
#define HDFFILE_HH
#include "hdfgroup.hh"
#include <hdf5.h>
#include <stdexcept>
#include <string>

namespace Utopia {
namespace DataIO {
// forward declaration of HDFGroup -> exchange later for the real thing.
class HDFGroup;

/**
 * @brief A class for a HDF5 file, holds a single group, the base group "/".
 */
class HDFFile {
private:
protected:
    hid_t _file_id;
    HDFGroup _base_group;

public:
    HDFGroup &open_group();
    void flush();
    void swap(HDFFile &);
    HDFFile() = default;
    HDFFile(std::string &&, std::string &&);
    HDFFile(const HDFFile &);
    HDFFile(HDFFile &&);
    HDFFile &operator=(HDFFile);
    virtual ~HDFFile();
};

void swap(HDFFile &lhs, HDFFile &rhs) { lhs.swap(rhs); }
} // namespace DataIO
} // namespace Utopia
#endif