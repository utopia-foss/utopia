#ifndef DATAIO_UTILS_HH
#define DATAIO_UTILS_HH

#include "types.hh"

namespace Utopia {
namespace DataIO {

/// Convenience function for access to config items
template<typename ReturnType, typename Entry>
ReturnType as_(Entry entry) {
    return entry.template as<ReturnType>();
}

} // namespace DataIO
} // namespace Utopia
#endif // DATAIO_UTILS_HH
