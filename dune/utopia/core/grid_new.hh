#ifndef UTOPIA_CORE_GRID_HH
#define UTOPIA_CORE_GRID_HH

// NOTE This file's final name will be grid.hh

#include "space.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

template<class Space>
class Grid {

};


template<class Space>
class RectangularGrid
    : public Grid<Space>
{

};


template<class Space>
class HexagonalGrid
    : public Grid<Space>
{

};


template<class Space>
class TriangularGrid
    : public Grid<Space>
{

};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRID_HH
