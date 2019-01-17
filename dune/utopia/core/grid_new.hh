#ifndef UTOPIA_CORE_GRID_HH
#define UTOPIA_CORE_GRID_HH

// NOTE This file's final name will be grid.hh

#include "space.hh"
#include "types.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

// ---------------------------------------------------------------------------
template<class Space>
class Grid {
protected:
    // -- Members -- //
    std::shared_ptr<Space> _space;

    const GridShapeType<Space::dim> _shape;

    const IndexType _num_cells;

public:
    // -- Constructor and Destructor -- //
    /// Construct a discretization for the given space using the specified
    /// grid shape
    Grid (std::shared_ptr<Space> space,
          const GridShapeType<Space::dim> shape)
    :
        _space(space),
        _shape(shape),
        _num_cells(__calc_num_cells())
    {}

    /// Virtual destructor to allow polymorphic destruction
    virtual ~Grid() = default;


    // -- Getters -- //
    IndexType num_cells() {
        return _num_cells;
    }

    // -- Public interface -- //
    // ...

protected:
    // -- Helper functions -- //
    // NOTE Some of these are best be made virtual such that the child
    //      classes can take care of the implementation.

    /// Calculate the number of cells given the grid shape
    IndexType __calc_num_cells() {  // NOTE Could make this (non-pure) virtual
        return std::accumulate(_shape.begin(), _shape.end(),
                               1, std::multiplies<IndexType>());
    };
};


// ---------------------------------------------------------------------------
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
        Grid<Space>(space, shape)
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
};


// ---------------------------------------------------------------------------
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
        Grid<Space>(space, shape)
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
};


// ---------------------------------------------------------------------------
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
        Grid<Space>(space, shape)
    {}

protected:
    // -- Custom implementations of virtual base class functions -- //
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRID_HH
