#ifndef AGENT_HH
#define AGENT_HH

namespace Citcat {

template<class Agent, class Manager>
void remove (const std::shared_ptr<Agent> agent, Manager& manager)
{
	auto& agents = manager.agents();
	const auto it = std::find(agents.cbegin(),agents.cend(),agent);
	if(it==agents.cend()){
		DUNE_THROW(Dune::Exception,"Agent is not managed by this manager");
	}
	agents.erase(it);
}

template<class Agent, class Manager>
bool add (const std::shared_ptr<Agent> agent, Manager& manager)
{
	auto& agents = manager.agents();
	if(std::find(agents.cbegin(),agents.cend(),agent)==agents.cend()){
		agents.push_back(agent);
		return true;
	}
	return false;
}

template<std::size_t i, typename Extensions, typename GridCells>
std::enable_if_t<i==0,std::pair<double,double>> cell_limits_per_index (const std::size_t index, const Extensions& extensions, const GridCells& grid_cells)
{
	std::size_t offset = index % shift<1>(grid_cells);
	const double ext_per_cell = extensions[i] / grid_cells[i];
	return std::make_pair( offset * ext_per_cell , (offset + 1) * ext_per_cell );
}

template<std::size_t i, typename Extensions, typename GridCells>
std::enable_if_t<i!=0,std::pair<double,double>> cell_limits_per_index (const std::size_t index, const Extensions& extensions, const GridCells& grid_cells)
{
	std::size_t offset = index / shift<i>(grid_cells);
	const double ext_per_cell = extensions[i] / grid_cells[i];
	return std::make_pair( offset * ext_per_cell , (offset + 1) * ext_per_cell );
}

/// Return all agents on a cell of a structured grid
template<class Cell, class Manager,
	bool enabled = Manager::is_structured()>
auto find_agents_on_cell (const std::shared_ptr<Cell> cell, const Manager& manager)
	-> std::enable_if_t<enabled,
		std::vector<std::shared_ptr<typename Manager::Agent>> >
{
	std::vector<std::shared_ptr<typename Manager::Agent>> ret;
	const auto id = cell->index();
	const auto& extensions = manager.extensions();
	const auto& grid_cells = manager.grid_cells();

	// deduce cell boundaries
	constexpr int dim = Manager::Traits::dim;
	std::array<std::pair<double,double>,dim> limits;

	if(dim == 3){
		limits[2] = cell_limits_per_index<2>(id,extensions,grid_cells);
		// 'normalize'
		const auto id_nrm = id % shift<2>(grid_cells);
		limits[1] = cell_limits_per_index<1>(id_nrm,extensions,grid_cells);
	}
	else{
		limits[1] = cell_limits_per_index<1>(id,extensions,grid_cells);
	}
	limits[0] = cell_limits_per_index<0>(id,extensions,grid_cells);

	// find agents inside cell boundaries
	for(auto agent : manager.agents()){
		const auto& pos = agent->position();
		if(std::equal(pos.begin(),pos.end(),limits.begin(),
			[](const auto val, const auto& lim){
				return lim.first <= val && val < lim.second;
			}))
		{
			ret.push_back(agent);
		}
	}

	return ret;
}

/// Return all agents on a cell of any grid
template<class Cell, class Manager,
	bool enabled = Manager::is_structured()>
auto find_agents_on_cell (const std::shared_ptr<Cell> cell, const Manager& manager)
	-> std::enable_if_t<!enabled,
		std::vector<std::shared_ptr<typename Manager::Agent>> >
{
	using Coordinate = typename Manager::Traits::Coordinate;
	constexpr int dim = Manager::Traits::dim;
	std::vector<std::shared_ptr<typename Manager::Agent>> ret;

	// get reference element for cell
	auto it = elements(manager.grid_view()).begin();
	std::advance(it,cell->index());
	const auto& geo = it->geometry();
	const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());

	// check location by transforming coordinates
	for(auto agent : manager.agents()){
		const auto pos_local = geo.local(agent->position());
		if(ref.checkInside(pos_local)){
			ret.push_back(agent);
		}
	}

	return ret;
}

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

/// Move agent to a new location
template<typename Position, class Agent, class Manager,
	bool enabled = Manager::is_periodic()>
std::enable_if_t<enabled,void> move_to (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
	const auto& ext = manager.extensions();
	auto pos_transf = pos;
	std::transform(pos.begin(),pos.end(),ext.begin(),pos_transf.begin(),
		[](const auto& a, const auto& b){
			const int shift = a / b > 0.0 ? - int(a / b) : - ( int(a / b) - 1 );
			return a + shift * b;
		});
	agent->position() = pos_transf;
}

/// Move agent to a new location
template<typename Position, class Agent, class Manager,
	bool enabled = Manager::is_periodic()>
std::enable_if_t<!enabled,void> move_to (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
	const auto& ext = manager.extensions();
	std::vector<bool> diff(Manager::Traits::dim);
	std::transform(pos.begin(),pos.end(),ext.begin(),diff.begin(),
		[](const auto& a, const auto& b){
			return a > b;
		});
	if(std::any_of(diff.begin(),diff.end(),[](const auto a){ return a; })){
		DUNE_THROW(Dune::Exception,"Position is out of grid boundaries");
	}
	agent->position() = pos;
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

} // namespace Citcat

#endif // AGENT_HH