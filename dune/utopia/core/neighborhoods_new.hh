#ifndef UTOPIA_CORE_NEIGHBORHOOD_HH
#define UTOPIA_CORE_NEIGHBORHOOD_HH

// NOTE This file will replace neighborhoods.hh

#include "types.hh"


namespace Utopia {
namespace Neighborhoods {

/// Type of the neighborhood calculating function
template<class Grid>
using NBFuncID = std::function<IndexContainer(const IndexType&, const Grid&)>;


// -- General helper functions -----------------------------------------------


// ---------------------------------------------------------------------------
/**
 *  \addtogroup Neighborhoods
 *  \{
 */

// TODO Categorize correctly in doxygen and write a few sentences here
// TODO Write about the required interface here
// TODO Ensure the returned index container is moved, not copied!
// TODO Consider using free functions (instead of lambdas)
// TODO Check if Space dimensionality can be used for constexpr statements


/// Always returns an empty neighborhood
template<class Grid>
NBFuncID<Grid> AllAlone = [](const IndexType&, const Grid&)
{
    IndexContainer idcs{};
    idcs.reserve(0);
    return idcs;
};

// ---------------------------------------------------------------------------
// -- Rectangular ------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Rectangular {

template<class Grid>
NBFuncID<Grid> Nearest = [](const IndexType&, const Grid&)
{
    IndexContainer idcs{};

    // TODO

    return idcs;
};
    
} // namespace Rectangular

// ---------------------------------------------------------------------------
// -- Hexagonal --------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Hexagonal {

} // namespace Hexagonal

// ---------------------------------------------------------------------------
// -- Triangular -------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Triangular {

} // namespace Triangular

// end group Neighborhoods
/**
 *  \}
 */

} // namespace Neighborhoods
} // namespace Utopia

#endif // UTOPIA_CORE_NEIGHBORHOOD_HH
