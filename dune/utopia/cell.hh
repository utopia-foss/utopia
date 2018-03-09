#ifndef CELL_HH
#define CELL_HH

#include <dune/utopia/neighborhoods.hh>
#include <dune/utopia/entity.hh>

namespace Utopia
{
///this class implements an entity on a grid
/**
 *Cell contains the position and the boolian _boundary.
 *Also Cell inherits the state, tag and index from Entity.
 */
template<
    typename T, bool sync, class Tags, typename PositionType,
    typename IndexType, std::size_t custom_neighborhood_count = 0>
class Cell:
    public Entity<
        Cell<T, sync, Tags, PositionType, IndexType, custom_neighborhood_count>,
        T, sync, Tags, IndexType, custom_neighborhood_count>
{
public:
    //\return position of cell center
    const PositionType& position() const {return _position;}
    //\return true if located at boundary
    inline bool is_boundary() const {return _boundary;}
    
    /// constructor of Cell
    Cell(T t, PositionType pos, const bool boundary, IndexType index) :
        Entity<Cell, T, sync, Tags, IndexType, custom_neighborhood_count>
            (t, index),
        _position(pos), _boundary(boundary)
    { }

private:
    //! Position of the cell center
    const PositionType _position;

    //! Does this cell lie on the grid boundary?
    const bool _boundary;    
};
    

} // namespace Utopia

#endif // CELL_HH
