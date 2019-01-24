#ifndef UTOPIA_CORE_GRIDS_HEXAGONAL_HH
#define UTOPIA_CORE_GRIDS_HEXAGONAL_HH

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using hexagonal cells
template<class Space>
class HexagonalGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr DimType dim = Space::dim;

    /// The type of the grid shape array
    using GridShape = GridShapeType<dim>;

private:
    // -- HexagonalGrid-specific members -- //

public:
    /// Construct a hexagonal grid discretization
    HexagonalGrid (std::shared_ptr<Space> space,
                   const GridShape shape)
    :
        Base(space, shape)
    {}

    /// The structure descriptor of this grid
    const std::string structure() const {
        return "hexagonal";
    }


protected:
    // -- Custom implementations of virtual base class functions -- //
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_HEXAGONAL_HH
