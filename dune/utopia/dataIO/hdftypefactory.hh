#ifndef HDFTYPEFACTORY_HH
#define HDFTYPEFACTORY_HH
#include "hdfutilities.hh"
#include <hdf5.h>
#include <hdf5_hl.h>
namespace Utopia {
namespace DataIO {

class HDFTypeFactory {
private:
    template <typename T> hid_t static inline __get_type__() { return 0; }

public:
    // overload for primitive types
    template <
        typename T,
        std::enable_if_t<is_container<typename std::decay_t<T>>::value == false,
                         int> = 0>
    static inline hid_t type(std::size_t);

    // overload for variable length types, includes array types
    template <
        typename T, std::size_t size = 0,
        std::enable_if_t<is_container<typename std::decay_t<T>>::value == true,
                         int> = 1>
    static inline hid_t type(std::size_t);
};
} // namespace DataIO
} // namespace Utopia
#endif