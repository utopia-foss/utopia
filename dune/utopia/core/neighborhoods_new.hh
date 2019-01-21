#ifndef UTOPIA_CORE_NEIGHBORHOOD_HH
#define UTOPIA_CORE_NEIGHBORHOOD_HH

// NOTE This file will replace neighborhoods.hh

#include "types.hh"


namespace Utopia {
namespace Neighborhoods {

/// Type of the neighborhood calculating function
template<class Grid>
using NBFunc = std::function<IndexContainer(IndexType&, Grid&)>;


// -- General helper functions -----------------------------------------------


// ---------------------------------------------------------------------------
/**
 *  \addtogroup Neighborhoods
 *  \{
 */

// TODO Categorize correctly in doxygen and write a few sentences here
// TODO Write about the required interface here

// ---------------------------------------------------------------------------
// -- Rectangular ------------------------------------------------------------
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// -- Hexagonal --------------------------------------------------------------
// ---------------------------------------------------------------------------




// ---------------------------------------------------------------------------
// -- Triangular -------------------------------------------------------------
// ---------------------------------------------------------------------------


// end group Neighborhoods
/**
 *  \}
 */

} // namespace Neighborhoods
} // namespace Utopia

#endif // UTOPIA_CORE_NEIGHBORHOOD_HH
