#ifndef UTOPIA_CORE_NEIGHBORHOOD_HH
#define UTOPIA_CORE_NEIGHBORHOOD_HH

// NOTE This file will replace neighborhoods.hh

#include "types.hh"


namespace Utopia {
namespace Neighborhoods {

/// Type of the neighborhood calculating function
template<class Grid>
using NBFuncID = std::function<IndexContainer(const IndexType&,
                                              const std::shared_ptr<Grid>&)>;


// -- General helper functions -----------------------------------------------


// ---------------------------------------------------------------------------
/**
 *  \addtogroup Neighborhoods
 *  \{
 */

// TODO Categorize correctly in doxygen and write a few sentences here
// TODO Write about the required interface here
// TODO consider grid specialisation by checking constexpr member of grid?
// TODO Ensure the returned index container is moved, not copied!


/// Always returns an empty neighborhood
template<class Grid>
auto AllAlone = [](const IndexType&, const std::shared_ptr<Grid>&) {
    IndexContainer idcs{};
    idcs.reserve(0);
    return idcs;
};

// ---------------------------------------------------------------------------
// -- Rectangular ------------------------------------------------------------
// ---------------------------------------------------------------------------

template<class Grid>
auto Nearest = [](const IndexType&, const std::shared_ptr<Grid>&) {
    IndexContainer idcs{};
    return idcs;
};

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
