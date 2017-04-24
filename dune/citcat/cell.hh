#ifndef CELL_HH
#define CELL_HH

namespace Citcat
{

/// This class implements an Entity on a grid.
/** An object of this class can save pointers to grid neighbors and
 *  user-defined neighbors of the same Cell type. Technically, it is
 *  not attached to the grid but maps to a grid entity via a Dune::Mapper
 *  and a grid index. This index and the position on the grid are saved
 *  as class members for convenience.
 *  
 *  \tparam StateType Data type of states
 *  \tparam TraitsType Data type of traits
 *  \tparam PositionType Data type of position vectors
 *  \tparam IndexType Data type of grid indices
 *  \tparam custom_neighborhood_count Number of custom neighborhoods
 *  	this Cell holds
 */
template<
	typename StateType,
	typename TraitsType,
	typename PositionType,
	typename IndexType,
	unsigned int custom_neighborhood_count = 0>
class Cell : public Entity<StateType,TraitsType>
{
public:
	
	using State = StateType;
	using Traits = TraitsType;
	using Position = PositionType;
	using Index = IndexType;

public:

	/// Construct a cell, implementing an entity on a grid
	/**
	 *  \param state Initial state
	 *  \param traits Initial traits
	 *  \param pos Position of cell center on grid
	 *  \param index Index on grid assigned by grid mapper
	 *  \param boundary Boolean if cell is located at grid boundary
	 *  \param tag Cell tracking tag
	 */
	Cell(const State state, const Traits traits, const Position pos, const Index index, const bool boundary=false, const int tag=0) :
		Entity<State,Traits>(state,traits,tag), _position(pos), _boundary(boundary), _index(index)
	{ }

	/// Return position on grid
	inline const Position& position() const { return _position; }
	/// Return grid index
	inline Index index() const { return _index; }

	/// Return true if cell is located at grid boundary.
	/** Notice that this still remains true if periodic boundary conditions are applied.
	 */
	inline bool boundary() const { return _boundary; }

	/// Return const reference to neighborhoods
	/*
	const std::array<Neighborhoods::CustomNeighborhood<Cell>,custom_neighborhood_count>& neighborhoods () const { return _neighborhoods; }
*/
	/// Return reference to neighborhoods
	std::array<std::vector<std::shared_ptr<Cell>>,custom_neighborhood_count>& neighborhoods () { return _neighborhoods; }

public:
	//! Custom neighborhood storage
	//std::array<Neighborhoods::CustomNeighborhood<Cell>,custom_neighborhood_count> _neighborhoods;
	std::array<std::vector<std::shared_ptr<Cell>>,custom_neighborhood_count> _neighborhoods;
private:
	//! Position of cell on grid
	const Position _position;
	//! Cell located at grid boundary
	const bool _boundary;
	//! Grid index of cell
	const Index _index;

};

} // namespace Citcat

#endif // CELL_HH
