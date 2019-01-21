#ifndef UTOPIA_CORE_GRID_HH
#define UTOPIA_CORE_GRID_HH

// NOTE This file's final name will be grid.hh

#include "space.hh"
#include "neighborhoods_new.hh" // NOTE Final name will be neighborhood.hh
#include "types.hh"


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

// ---------------------------------------------------------------------------

/// The base class for all grid discretizations used by the CellManager
template<class Space>
class Grid {
public:
    /// Type of the neighborhood functions
    using NBFunc = std::function<IndexContainer(IndexType&)>;

protected:
    // -- Members -- //
    /// The space that is to be discretized
    std::shared_ptr<Space> _space;

    /// The rectangular (multi-index) shape of the discretization
    const GridShapeType<Space::dim> _shape;

    /// The number of cells required by this discretization
    const IndexType _num_cells;

public:
    /// The structure of the specialization
    const std::string structure;
    // TODO consider making static somehow (no priority right now)

public:
    // -- Constructor and Destructor -- //
    /// Construct a discretization for the given space using the specified
    /// grid shape
    Grid (std::shared_ptr<Space> space,
          const GridShapeType<Space::dim> shape,
          const std::string grid_structure)
    :
        _space(space),
        _shape(shape),
        _num_cells(__calc_num_cells()),
        structure(grid_structure)
    {}

    /// Virtual destructor to allow polymorphic destruction
    virtual ~Grid() = default;


    // -- Public interface -- //
    /// The neighborhood function
    NBFunc neighbors_of;

    /// Select which neighborhood function is to be used 
    void select_neighborhood_func(std::string name) {
        try {
            neighbors_of = __select_nb_func(name);
        }
        catch (std::exception& e) {
            throw std::invalid_argument("Failed to retrieve neighborhood "
                "function for '" + structure + "'' grid! " + e.what());
        }
    }


    // -- Getters -- //
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


    /// Return a neighborhood function by name; called from public interface
    virtual NBFunc __select_nb_func(std::string name) = 0;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

/// A grid discretization using rectangular cells
template<class Space>
class RectangularGrid
    : public Grid<Space>
{
private:
    // -- RectangularGrid-specific members -- //

public:
    RectangularGrid (std::shared_ptr<Space> space,
                     const GridShapeType<Space::dim> shape)
    :
        Grid<Space>(space, shape, "rectangular")
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
    /// Choose from the available neighborhood functions for this grid
    NBFunc __select_nb_func(std::string name) {
        return Neighborhoods::NextNeighbor<RectangularGrid<Space>>;
    }
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

/// A grid discretization using hexagonal cells
template<class Space>
class HexagonalGrid
    : public Grid<Space>
{
private:
    // -- HexagonalGrid-specific members -- //

public:
    HexagonalGrid (std::shared_ptr<Space> space,
                   const GridShapeType<Space::dim> shape)
    :
        Grid<Space>(space, shape, "hexagonal")
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
    /// Choose from the available neighborhood functions for this grid
    NBFunc __select_nb_func(std::string name) {
        throw std::invalid_argument("No neighborhood functions available for "
                                    "HexagonalGrid!");
    }

};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

/// A grid discretization using triangular cells
template<class Space>
class TriangularGrid
    : public Grid<Space>
{
private:
    // -- TriagonalGrid-specific members -- //

public:
    TriangularGrid (std::shared_ptr<Space> space,
                    const GridShapeType<Space::dim> shape)
    :
        Grid<Space>(space, shape, "triangular")
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
    /// Choose from the available neighborhood functions for this grid
    NBFunc __select_nb_func(std::string name) {
        throw std::invalid_argument("No neighborhood functions available for "
                                    "HexagonalGrid!");
    }

};



// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRID_HH
