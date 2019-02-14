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

    /// The type of vectors that have a relation to physical space
    using PhysVector = typename Space::PhysVector;

    /// The type of multi-index like arrays, e.g. the grid shape
    using MultiIndex = MultiIndexType<dim>;


private:
    // -- HexagonalGrid-specific members --------------------------------------


public:
    // -- Constructors --------------------------------------------------------
    /// Construct a hexagonal grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    HexagonalGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg)
    {}

    /// Construct a hexagonal grid discretization
    /** \param  space   The space to construct the discretization for; will be
      *                 stored as shared pointer
      * \param  cfg     Further configuration parameters
      */
    HexagonalGrid (Space& space, const DataIO::Config& cfg)
    :
        HexagonalGrid(std::make_shared<Space>(space), cfg)
    {}


    // -- Implementations of virtual base class functions ---------------------
    // .. Number of cells & shape .............................................

    /// Number of hexagonal cells required to fill the physical space
    IndexType num_cells() const override {
        // TODO Implement
        return 0;
    };

    /// The effective cell resolution into each physical space dimension
    const PhysVector effective_resolution() const override {
        // TODO Implement
        PhysVector res_eff;
        res_eff.fill(0.);
        return res_eff;
    }

    /// Shape of the triangular grid
    const MultiIndex shape() const override {
        //TODO Implement properly!
        MultiIndexType<Space::dim> shape;
        shape.fill(0);
        return shape;
    }


    // .. Position-related methods ............................................
    /// Returns the multi index of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const MultiIndex midx_of(const IndexType&) const override {
        throw std::runtime_error("The HexagonalGrid::midx_of method is not "
                                 "yet implemented!");
        return {};
    }

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const PhysVector barycenter_of(const IndexType&) const override {
        throw std::runtime_error("The HexagonalGrid::barycenter_of method "
                                 "is not yet implemented!");
        return {};
    }

    /// Returns the extent of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const PhysVector extent_of(const IndexType&) const override {
        throw std::runtime_error("The HexagonalGrid::extent_of method is not "
                                 "yet implemented!");
        return {};
    }


protected:
    // -- Neighborhood interface ----------------------------------------------
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


    // .. Neighborhood implementations ........................................
    // ...
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_HEXAGONAL_HH
