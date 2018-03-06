#ifndef CELL_HH
#define CELL_HH


namespace Utopia
{       
///this class implements an entity on a grid
/**
 *Cell contains the position and the boolian _boundary.
 *Also Cell inherits the state, tag and index from Entity.
 */
template<typename T, bool sync ,typename PositionType, 
         class Tags, typename IndexType>
class Cell: public Entity<T, sync, Tags,IndexType>
{
public:    
    //\return position of cell center
    const PositionType& position() const {return _position;}
    //\return true if located at boundary
    inline bool is_boundary() const {return _boundary;}
    
    /// constructor of Cell
    Cell(T t, PositionType pos,const bool boundary, IndexType index) :
         Entity<T,sync,Tags, IndexType> (t,index)
         , _position(pos), _boundary(boundary)
    {}
private:
    //! Position of the cell center
    const PositionType _position;
    
    //! Does this cell lie on the grid boundary?
    const bool _boundary;    
};
    

} // namespace Utopia

#endif // CELL_HH
