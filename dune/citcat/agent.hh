#ifndef AGENT_HH
#define AGENT_HH

namespace Citcat {

/// This class implements a moving Agent on a grid
/** An object of this class saves a pointer to the cell it resides on.
 *  Attachment to the grid is achieved via attachment to a Cell. This is also
 *  useful for data analysis later on.
 *
 *  \tparam StateType Data type of state
 *  \tparam TraitsType Data type of traits
 *  \tparam CellType Data type of the CA cells
 */
template<typename StateType, typename TraitsType, typename CellType>
class Agent :
	public Entity<StateType,TraitsType>
{

// Export data types
protected:

	using Cell = CellType;

public:
	
	using State = StateType;
	using Traits = TraitsType;
	using Position = typename Cell::Position;
	using Index = typename Cell::Index;

// Data members
private:
	//!< Cell in which this agent is located
	std::shared_ptr<Cell> _parent;
	//!< Global position on the grid
	Position _position;

public:
	/// Constructor
	/** \param state Initial state
	 *  \param traits Initial traits
	 *  \param position Initial positon
	 *  \param parent Initial parent
	 *  \param tag Tracking tag
	 *  \note There is no check whether the given parent matches the position!
	 */
	Agent (const State state, const Traits traits, const Position position,
		const std::shared_ptr<Cell> parent, const int tag = 0) :
		Entity<State,Traits>(state,traits,tag),
		_parent(parent),
		_position(position)
	{ }

	/// Return the parent cell of this agent
	std::shared_ptr<Cell> parent () const { return _parent; }

	/// Find the parent cell of this agent according to the current position
	/** The cell is stored as parent of this agent.
	 *  \param grid Grid on which the cells live
	 *  \param cells Container of cells to search for parent
	 *  \return Shared pointer to the parent cell
	 *  \throw Dune::Exception Position does not match any grid entity or cell
	 */
	template<typename GridType, typename CellContainer>
	std::shared_ptr<Cell> find_parent (const std::shared_ptr<GridType> grid,
		const CellContainer& cells)
	{
		static_assert(std::is_same<typename CellContainer::value_type,std::shared_ptr<Cell>>::value,
			"Agent was initialized with other type of Cells");

		using GridTypes = GridTypeAdaptor<GridType>;
		using Coordinate = typename GridTypes::Coordinate;
		constexpr auto dim = GridTypes::dim;
		using GV = typename GridTypes::GridView;
		using Mapper = typename GridTypes::Mapper;

		GV gv(*grid);
		Mapper mapper(gv);

		// iterate over grid elements and check if position is inside
		for (const auto& e : elements(gv))
		{
			const auto geo = e.geometry();
			const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());
			const auto pos_local = geo.local(_position);

			if (ref.checkInside(pos_local))
			{
				const auto id = mapper.index(e);
				auto it = std::find_if(cells.begin(),cells.end(),
					[&id](auto c){ return c->index() == id; });
				if (it != cells.end()) {
					_parent = *it;
					return *it;
				}
			}
		}

		DUNE_THROW(Dune::Exception,"Cell not found!");
	}
};

} // namespace Citcat

#endif // AGENT_HH