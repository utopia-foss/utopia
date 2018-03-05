#ifndef CELL_HH
#define CELL_HH


namespace Utopia
{       
///this class implements an entity on a grid
/**
 *Cell contains the position and the boolian _boundary.
 *Also Cell inherits the state, position and index from Entity.
 */
template<class StateContainer,typename PositionType,
         class Tags, typename IndexType>
class Cell:public Entity<StateContainer,Tags,IndexType>
{
public:    
    //\return position of cell center
    const PositionType& position(){return _position;}
    //\return true if located at boundary
    const inline bool is_boundary(){return _boundary;}
    
    /// constructor of Cell
    Cell(StateContainer state, PositionType pos, bool boundary, 
         Tags tag, IndexType index) :
         Entity<StateContainer,Tags, IndexType>(state,tag,index)
            , _position(pos), _boundary(boundary)
    { }
private:
    //! Position of the cell center
    const PositionType _position;
    
    //! Does this cell lie on the grid boundary?
    const bool _boundary;    
};
    

} // namespace Utopia

#endif // CELL_HH
