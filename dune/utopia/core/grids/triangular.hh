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

    /// The type of vectors that have a relation to physical space
    using PhysVector = typename Space::PhysVector;

    /// The type of multi-index like arrays, e.g. the grid shape
    using MultiIndex = MultiIndexType<dim>;


private:
    // -- TriagonalGrid-specific members --------------------------------------


public:
    // -- Constructors --------------------------------------------------------
    /// Construct a triangular grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    TriangularGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg)
    {}


    /// Construct a triangular grid discretization
    /** \param  space   The space to construct the discretization for; will be
      *                 stored as shared pointer
      * \param  cfg     Further configuration parameters
      */
    TriangularGrid (Space& space, const DataIO::Config& cfg)
    :
        TriangularGrid(std::make_shared<Space>(space), cfg)
    {}


    // -- Implementations of virtual base class functions ---------------------
    // .. Number of cells & shape .............................................

    /// Number of triangular cells required to fill the physical space
    IndexType num_cells() const override {
        // TODO Implement properly!
        return 0;
    };

    /// The effective cell resolution into each physical space dimension
    const PhysVector effective_resolution() const override {
        // TODO Implement properly!
        PhysVector res_eff;
        res_eff.fill(0.);
        return res_eff;
    }

    /// Get shape of the triangular grid
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
        throw std::runtime_error("The TriangularGrid::midx_of method is not "
                                 "yet implemented!");
        return {};
    }

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const PhysVector barycenter_of(const IndexType&) const override {
        throw std::runtime_error("The TriangularGrid::barycenter_of method "
                                 "is not yet implemented!");
        return {};
    }

    /// Returns the extent of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const PhysVector extent_of(const IndexType&) const override {
        throw std::runtime_error("The TriangularGrid::extent_of method is not "
                                 "yet implemented!");
        return {};
    }

    /// Returns the vertices of the cell with the given ID
    /** \detail The order of the vertices is not guaranteed.
      * \note   This method does not perform bounds checking of the given ID!
      */
    const std::vector<const PhysVector>
        vertices_of(const IndexType&) const override
    {
        throw std::runtime_error("The TriangularGrid::vertices_of method is "
                                 "not yet implemented!");
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
                + "' available for triangular grid discretization!");
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

#endif // UTOPIA_CORE_GRIDS_TRIANGULAR_HH
