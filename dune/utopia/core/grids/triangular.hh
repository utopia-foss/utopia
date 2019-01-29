#ifndef UTOPIA_CORE_GRIDS_TRIANGULAR_HH
#define UTOPIA_CORE_GRIDS_TRIANGULAR_HH

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using triangular cells
template<class Space>
class TriangularGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr std::size_t dim = Space::dim;

    /// The type of the grid shape array
    using GridShape = GridShapeType<dim>;

private:
    // -- TriagonalGrid-specific members -- //

public:
    /// Construct a triangular grid discretization
    TriangularGrid (std::shared_ptr<Space> space,
                    const GridShape shape)
    :
        Base(space, shape)
    {}


protected:
    // -- Custom implementations of virtual base class functions -- //

    /// Calculate the number of cells required to fill the current grid shape
    IndexType calc_num_cells() override {
        // Requires twice as many cells as a rectangular grid
        return 2 * std::accumulate(this->_shape.begin(), this->_shape.end(),
                                   1, std::multiplies<IndexType>());
    };

    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode) override {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' available for triangular grid discretization!");
        }
    }

    // -- Neighborhood interface -- //
    // ...
};

// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_TRIANGULAR_HH
