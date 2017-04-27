#ifndef AGENT_HH
#define AGENT_HH

namespace Citcat {

/// Find parent cell for any grid by performing a grid search
template<class Agent, class Manager, bool enabled = Manager::is_structured()>
auto find_cell (const std::shared_ptr<Agent> agent, const Manager& manager)
	-> std::enable_if_t<!enabled,std::shared_ptr<typename Manager::Cell>>
{
	using Coordinate = typename Manager::Traits::Coordinate;
	constexpr int dim = Manager::Traits::dim;
	const auto& gv = manager.grid_view();

	const auto& pos_global = agent->position();
	const auto it = std::find_if(
		elements(gv).begin(),
		elements(gv).end(),
		[&pos_global](const auto& entity)
		{
			// perform coordinate transformation
			const auto& geo = entity.geometry();
			const auto pos_local = geo.local(pos_global);
			const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());
			return ref.checkInside(pos_local);
		}
	);

	if(it == elements(gv).end()){
		DUNE_THROW(Dune::Exception,"Agent is not inside the grid!");
	}

	const auto id = manager.mapper().index(*it);
	return manager.cells()[id];
}

template<std::size_t i, typename Position, typename Extensions, typename GridCells>
std::enable_if_t<i==0,std::size_t> grid_index (const Position& pos, const Extensions& ext, const GridCells& grid_cells)
{
	std::size_t id_inc = pos[i] / ( ext[i] / grid_cells[i] );
	return id_inc;
}

template<std::size_t i, typename Position, typename Extensions, typename GridCells>
std::enable_if_t<i!=0,std::size_t> grid_index (const Position& pos, const Extensions& ext, const GridCells& grid_cells)
{
	std::size_t id_inc = pos[i] / ( ext[i] / grid_cells[i] );
	return id_inc * shift<i>(grid_cells) + grid_index<i-1>(pos,ext,grid_cells);
}

/// Find parent cell in structured grid using spacing information
template<class Agent, class Manager, bool enabled = Manager::is_structured()>
auto find_cell (const std::shared_ptr<Agent> agent, const Manager& manager)
	-> std::enable_if_t<enabled,std::shared_ptr<typename Manager::Cell>>
{
	const auto& position = agent->position();
	const auto& extensions = manager.extensions();
	const auto& grid_cells = manager.grid_cells();

	std::size_t index;
	if(Manager::Traits::dim == 3)
	{
		index = grid_index<2>(position,extensions,grid_cells);
	}
	else{
		index = grid_index<1>(position,extensions,grid_cells);
	}

	return manager.cells()[index];
}


/// This class implements a moving Agent on a grid
/** An object of this class saves a pointer to the cell it resides on.
 *  Attachment to the grid is achieved via attachment to a Cell. This is also
 *  useful for data analysis later on.
 *
 *  \tparam StateType Data type of state
 *  \tparam TraitsType Data type of traits
 *  \tparam CellType Data type of the CA cells
 */
template<typename StateType, typename TraitsType, typename PositionType>
class Agent :
	public Entity<StateType,TraitsType>
{

public:
	
	using State = StateType;
	using Traits = TraitsType;
	using Position = PositionType;

// Data members
private:
	//!< Global position on the grid
	Position _position;

public:
	/// Constructor
	/** \param state Initial state
	 *  \param traits Initial traits
	 *  \param position Initial position
	 *  \param tag Tracking tag
	 *  \note There is no check whether the given parent matches the position!
	 */
	Agent (const State state, const Traits traits, const Position position,
		const int tag = 0) :
		Entity<State,Traits>(state,traits,tag),
		_position(position)
	{ }

	/// Return const reference to the position of this agent
	const Position& position () const { return _position; }
	/// Return reference to the position of this agent
	Position& position () { return _position; }

};


/// Find all agents that have a certain cell as parent
/** \param agents Container of pointers to agents
 *  \param cell The cell for which to search
 *  \return std::vector with all appropriate agents
 */
 /*
template<typename AgentContainer, typename Cell>
auto find_agents_on_cell (const AgentContainer& agents, std::shared_ptr<Cell> cell)
	-> std::vector<typename AgentContainer::value_type>
{
	std::vector<typename AgentContainer::value_type> ret;
	for (const auto& agent : agents){
		if (agent->parent() == cell)
			ret.push_back(agent);
	}
	return ret;
}
*/


} // namespace Citcat

#endif // AGENT_HH