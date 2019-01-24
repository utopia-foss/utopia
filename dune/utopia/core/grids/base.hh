#ifndef UTOPIA_CORE_GRIDS_BASE_HH
#define UTOPIA_CORE_GRIDS_BASE_HH

#include "../types.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// Type of the neighborhood calculating function
template<class Grid>
using NBFuncID = std::function<IndexContainer(const IndexType&, const Grid&)>;


/// The base class for all grid discretizations used by the CellManager
template<class Space>
class Grid {
public:
    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr DimType dim = Space::dim;

    /// The type of the grid shape array
    using GridShape = GridShapeType<dim>;

protected:
    // -- Members -- //
    /// The space that is to be discretized
    std::shared_ptr<Space> _space;

    /// The rectangular (multi-index) shape of the discretization
    const GridShape _shape;

    /// The number of cells required by this discretization
    const IndexType _num_cells;

public:
    // -- Constructors and Destructors -- //
    /// Construct a discretization for the given space using the specified
    /// grid shape
    Grid (std::shared_ptr<Space> space,
          const GridShape& shape)
    :
        _space(space),
        _shape(shape),
        _num_cells(__calc_num_cells())
    {}

    /// Virtual destructor to allow polymorphic destruction
    virtual ~Grid() = default;


    // -- Getters -- //
    /// Get this grid's structure descriptor
    virtual const std::string structure() const = 0;

    /// Get number of cells
    /** \detail This information is used by the CellManager to populate the
      *         cell container with the returned number of cells
      */
    IndexType num_cells() const {
        return _num_cells;
    }
    
    /// Get const reference to grid shape
    const GridShapeType<Space::dim>& shape() const {
        return _shape;
    }

    /// Whether the grid is periodic
    bool is_periodic() const {
        return _space->periodic;
    }


protected:
    // -- Helper functions -- //
    // NOTE Some of these are best be made virtual such that the child
    //      classes can take care of the implementation.
    // NOTE Defining a pure virtual method here and forgetting to implement it
    //      in the derived classes will lead to a loooong list of compiler
    //      errors. For non-pure virtual methods, the errors will emerge during
    //      linking and also be quite cryptic ...

    /// Calculate the number of cells given the current grid shape
    IndexType __calc_num_cells() {  // NOTE Could make this (non-pure) virtual
        return std::accumulate(_shape.begin(), _shape.end(),
                               1, std::multiplies<IndexType>());
    };
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_BASE_HH
