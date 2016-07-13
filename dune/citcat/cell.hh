#ifndef CELL_HH
#define CELL_HH

/// Abstract Cell on grid
/**
 *  \todo add interface for Individuals
 */
template<typename StateType, typename TraitsType>
class Cell : public Entity<StateType,TraitsType>
{
public:
	
	using State = StateType;
	using Traits = TraitsType;

protected:

	using Domain = typename StaticTypes::Grid::Domain;
	using Index = typename StaticTypes::Grid::Index;

public:

	//! Constructor
	Cell(const State& state_, const Traits& traits_, const Domain& pos_, const Index id_, const bool boundary=false, const int tag_=0) :
		Entity<State,Traits>(state_,traits_,tag_), pos(pos_), bnd(boundary), id(id_)
	{ }

	/// Return grid position
	const Domain& position() const { return pos; }
	/// Return grid index
	Index index() const { return id; }

	/// Return true if cell is located at grid boundary.
	/** Notice that there is no boundary for periodic grids.
	 */
	bool boundary() const { return bnd; }

	/// Return connected neighbors
	/** Verify if connections to neighbors are still valid
	 */
	CellContainer<Cell<State,Traits>> neighbors() const
	{
		CellContainer<Cell<State,Traits>> ret;
		ret.reserve(nb.size());
		for(const auto& i : nb){
			if(auto x = i.lock())
				ret.push_back(std::move(x));
		}
		return ret;
	}

	/// Return grid neighbors
	/** Verify if connections to neighbors are still valid
	 */
	CellContainer<Cell<State,Traits>> grid_neighbors() const
	{
		CellContainer<Cell<State,Traits>> ret;
		ret.reserve(gnb.size());
		for(const auto& i : gnb){
			if(auto x = i.lock())
				ret.push_back(std::move(x));
		}
		return ret;
	}

	/// Set new neighbor for this Cell
	/** \return Boolean if neighbor was inserted
	 */
	bool add_neighbor(const std::shared_ptr<Cell> cell_s)
	{
		const auto nb_0 = neighbors();
		//std::shared_ptr<Cell> this_cell = shared_from_this();
		if(std::find(nb_0.begin(),nb_0.end(),cell_s)==nb_0.end()
			&& cell_s->index()!=index())
		{
			std::weak_ptr<Cell> n = cell_s;
			nb.push_back(std::move(n));
			return true;
		}
		return false;
	}

	/// Set new grid neighbor for this Cell
	/** \return Boolean if neighbor was inserted
	 */
	bool add_grid_neighbor(const std::shared_ptr<Cell> cell_s)
	{
		const auto gnb_0 = grid_neighbors();
		//std::shared_ptr<Cell> this_cell = shared_from_this();
		if(std::find(gnb_0.begin(),gnb_0.end(),cell_s)==gnb_0.end()
			&& cell_s->index()!=index())
		{
			std::weak_ptr<Cell> n = cell_s;
			gnb.push_back(std::move(n));
			return true;
		}
		return false;
	}

	/// Generic neighbor counter
	/** \param f Function object taking one neighbor as parameter.
	 *  					Returns bool if neighbor will be counted.
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
	 *  					Returns bool if neighbor will be counted.
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
	int neighbors_count() const { return neighbors().size(); }

	/// Return number of grid neighbors
	int grid_neighbors_count() const { return grid_neighbors().size(); }

	/// Return count of neighbors with state s
	/** \todo add function for returning bool
	 */
/*	int neighbors_with(const State& s) const
	{
		int count = 0;
		for(const auto& i : neighbors()){
			if(i->state()==s) count++;
		}
		return count;
	}*/

	/// Return count of neighbors with trait t
	/** \todo add function for returning bool
	 */
/*	int neighbors_with(const Traits& t) const
	{
		int count = 0;
		for(const auto& i : neighbors()){
			if(i->traits()==t) count++;
		}
		return count;
	}*/

private:
	//! List of connected neighbors \todo Create tuples for connection weight
	std::vector<std::weak_ptr<Cell>> nb;
	//! List of neighbors on grid
	std::vector<std::weak_ptr<Cell>> gnb;
	//! Position of cell
	const Domain pos;
	//! Cell located at grid boundary
	const bool bnd;
	//! Grid index of cell
	const Index id;

};

#endif // CELL_HH
