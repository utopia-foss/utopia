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
    static constexpr std::size_t dim = Space::dim;

private:
    // -- HexagonalGrid-specific members -- //

public:
    /// Construct a hexagonal grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    HexagonalGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg)
    {}


    // -- Custom implementations of virtual base class functions -- //

    /// Number of hexagonal cells required to fill the physical space
    IndexType num_cells() const override {
        return 0; // TODO Implement
    };

    /// The effective cell resolution into each physical space dimension
    const std::array<double, dim> effective_resolution() const override {
        // TODO Implement
        std::array<double, dim> res_eff;
        res_eff.fill(0.);
        return res_eff;
    }

    /// Shape of the triangular grid
    const GridShapeType<Space::dim> shape() const override {
        //TODO Implement properly!
        GridShapeType<Space::dim> shape;
        shape.fill(0);
        return shape;
    }


protected:
    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode) const override {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' available for hexagonal grid discretization!");
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

#endif // UTOPIA_CORE_GRIDS_HEXAGONAL_HH
