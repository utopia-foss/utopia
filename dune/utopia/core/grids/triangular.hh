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

private:
    // -- TriagonalGrid-specific members -- //

public:
    /// Construct a triangular grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    TriangularGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg)
    {}


    // -- Custom implementations of virtual base class functions -- //

    /// Number of triangular cells required to fill the physical space
    IndexType num_cells() const override {
        // TODO Implement properly!
        return 0;
    };

    /// The effective cell resolution into each physical space dimension
    const std::array<double, dim> effective_resolution() const override {
        // TODO Implement properly!
        std::array<double, dim> res_eff;
        res_eff.fill(0.);
        return res_eff;
    }

    /// Get shape of the triangular grid
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
