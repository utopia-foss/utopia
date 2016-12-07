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
 *  \todo add interface for Individuals
 */
template<typename StateType, typename TraitsType, typename PositionType, typename IndexType>
class Cell : public Entity<StateType,TraitsType>
{
public:
	
	using State = StateType;
	using Traits = TraitsType;

protected:

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

	/// Get connected neighbors of this cell
	/** \return Container of shared pointers to neighbors
	 *    Notice that this container might be empty 
	 */
	CellContainer<Cell<State,Traits,Position,Index>> neighbors() const
	{
		CellContainer<Cell<State,Traits,Position,Index>> ret;
		ret.reserve(_neighbors.size());
		for(const auto& i : _neighbors){
			if(auto x = i.lock())
				ret.push_back(std::move(x));
		}
		return ret;
	}

	/// Get grid neighbors of this cell
	/** \return Container of shared pointers to grid neighbors
	 *    Notice that this container might be empty
	 */
	CellContainer<Cell<State,Traits,Position,Index>> grid_neighbors() const
	{
		CellContainer<Cell<State,Traits,Position,Index>> ret;
		ret.reserve(_grid_neighbors.size());
		for(const auto& i : _grid_neighbors){
			if(auto x = i.lock())
				ret.push_back(std::move(x));
		}
		return ret;
	}

	/// Set new neighbor for this Cell.
	/** Duplicates and pointers to the cell itself will not be inserted.
	 *  \param cell Cell to be inserted as neighbor
	 *  \return Boolean if neighbor was inserted
	 */
	bool add_neighbor(const std::shared_ptr<Cell> cell)
	{
		const auto nb = neighbors();
		if(std::find(nb.begin(),nb.end(),cell)==nb.end()
			&& cell->index()!=index())
		{
			std::weak_ptr<Cell> cell_w = cell;
			_neighbors.push_back(std::move(cell_w));
			return true;
		}
		return false;
	}

	/// Set new grid neighbor for this Cell
	/** Duplicates and pointers to the cell itself will not be inserted.
	 *  \param cell Cell to be inserted as grid neighbor
	 *  \return Boolean if neighbor was inserted
	 */
	bool add_grid_neighbor(const std::shared_ptr<Cell> cell)
	{
		const auto gnb = grid_neighbors();
		if(std::find(gnb.begin(),gnb.end(),cell)==gnb.end()
			&& cell->index()!=index())
		{
			std::weak_ptr<Cell> cell_w = cell;
			_grid_neighbors.push_back(std::move(cell_w));
			return true;
		}
		return false;
	}

	/// Generic neighbor counter
	/** \param f Function object taking one neighbor as parameter.
	 *    Returns bool if neighbor will be counted.
	 *  \return Count for f() returning true
	 */
	int neighbors_count(std::function<bool(std::shared_ptr<const Cell>)> f)
	{
		int count = 0;
		for(const auto& i : neighbors()){
			if(f(i)) count++;
		}
		return count;
	}

	/// Generic grid neighbor counter
	/** \param f Function object taking one neighbor as parameter.
	 *    Returns bool if neighbor will be counted.
	 *  \return Count for f() returning true
	 */
	int grid_neighbors_count(std::function<bool(std::shared_ptr<const Cell>)> f)
	{
		int count = 0;
		for(const auto& i : grid_neighbors()){
			if(f(i)) count++;
		}
		return count;
	}

	/// Return number of neighbors
	inline int neighbors_count() const { return neighbors().size(); }

	/// Return number of grid neighbors
	inline int grid_neighbors_count() const { return grid_neighbors().size(); }


private:
	//! List of connected neighbors
	std::vector<std::weak_ptr<Cell>> _neighbors;
	//! List of neighbors on grid
	std::vector<std::weak_ptr<Cell>> _grid_neighbors;
	//! Position of cell on grid
	const Position _position;
	//! Cell located at grid boundary
	const bool _boundary;
	//! Grid index of cell
	const Index _index;

};

} // namespace Citcat

#endif // CELL_HH
