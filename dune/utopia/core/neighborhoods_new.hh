#ifndef UTOPIA_CORE_NEIGHBORHOOD_HH
#define UTOPIA_CORE_NEIGHBORHOOD_HH

// NOTE This file will replace neighborhoods.hh

#include "types.hh"

namespace Utopia {
namespace Neighborhoods {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// Type of the neighborhood calculating function
template<class Cell, class CellManager>
using NBFunc = std::function<CellContainer<Cell>(Cell&, CellManager&)>;





// end group CellManager
/**
 *  \}
 */

} // namespace Neighborhoods
} // namespace Utopia

#endif // UTOPIA_CORE_NEIGHBORHOOD_HH
