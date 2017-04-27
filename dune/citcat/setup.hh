#ifndef SETUP_HH
#define SETUP_HH

namespace Citcat
{

/// Return the extensions of a grid
template<class Grid>
auto determine_extensions (const std::shared_ptr<Grid> grid)
	-> std::array<typename GridTypeAdaptor<Grid>::Coordinate,
		GridTypeAdaptor<Grid>::dim>
{
	static constexpr int dim = GridTypeAdaptor<Grid>::dim;
	using Coordinate = typename GridTypeAdaptor<Grid>::Coordinate;

	auto gv = grid->leafGridView();
	std::array<Coordinate,dim> ret;

	std::fill(ret.begin(),ret.end(),0.0);
	for(const auto& v : vertices(gv)){
		const auto pos = v.geometry().center();
		for(int i = 0; i<dim; ++i){
			ret.at(i) = std::max(pos[i],ret.at(i));
		}
	}
	return ret;
}
	
/// Functions for building objects and setting up a simulation
namespace Setup
{

	template<bool structured, bool periodic, typename GridType, typename CellType>
	GridManager<GridType,structured,periodic,CellType,int> create_manager (
		const GridWrapper<GridType>& wrapper,
		const CellContainer<CellType>& cells )
	{
		return GridManager<GridType,structured,periodic,CellType,int>(
			wrapper,cells);
	}

	template<bool structured, bool periodic, typename GridType, typename CellType,typename AgentType>
	GridManager<GridType,structured,periodic,CellType,AgentType> create_manager (
		const GridWrapper<GridType>& wrapper,
		const CellContainer<CellType>& cells,
		const AgentContainer<AgentType>& agents)
	{
		return GridManager<GridType,structured,periodic,CellType,AgentType>(wrapper,cells,agents);
	}

	/// Create an unstructured grid from a Gmsh file
	/**
	 *  \tparam dim Spatial dimension of grid and mesh
	 *  \param filename Name of the Gmsh file with path relative to executable
	 *  \param refinement_level Level of global refinement applied to grid
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<int dim=2>
	GridWrapper<Dune::UGGrid<dim>> read_gmsh (const std::string filename, const unsigned int refinement_level=0)
	{
		using Grid = Dune::UGGrid<dim>;
		auto grid = std::make_shared<Grid>();

		Dune::GridFactory<Grid> factory(grid.get());
		Dune::GmshReader<Grid>::read(factory,filename);
		factory.createGrid();

		grid->globalRefine(refinement_level);

		GridWrapper<Grid> gw = {grid,determine_extensions(grid)};
		return gw;
	}

	/// Create a simulation object from a grid and a set of cells
	/** This function places the data inside a SimulationWrapper for
	 *  convenience and sets up a simulation from this wrapper.
	 *  The individuals container is replaced by the
	 *  Citcat::EmptyContainer template.
	 *  \param grid Shared pointer to the grid from which the cells were built
	 *  \param cells Cell container
	 *  \return Simulation object
	 */
	template<typename GridType, typename CellContainer>
	decltype(auto) create_sim_cells (const std::shared_ptr<GridType>& grid, CellContainer& cells)
	{
		using DataType = SimulationWrapper<GridType,CellContainer,EmptyContainer>; 
		EmptyContainer individuals;
		DataType data(grid,cells,individuals);
		Simulation<DataType> sim(data);
		return sim;
	}

	/// Build a rectangular grid.
	/** Cells will be rectangular/cubic. Cell edge length defaults to 1 if
	 *  'range' parameter is omitted.
	 *  \tparam dim Spatial dimension of the grid. Default is 2.
	 *  \param cells Array of size dim containing the number of cells
	 *    in each dimension
	 *  \param range Array of size dim containing the grid extensions
	 *    in each dimension
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<int dim=2>
	GridWrapper<DefaultGrid<dim>>
		create_grid (const std::array<unsigned int,dim> cells, std::array<float,dim> range = std::array<float,dim>())
	{
		using Grid = DefaultGrid<dim>;
		using GridTypes = GridTypeAdaptor<Grid>;
		using Position = typename GridTypes::Position;

		bool automated_range = false;
		for(auto it = range.cbegin(); it != range.cend(); ++it){
			if(*it == 0.0){
				automated_range = true;
				break;
			}
		}

		if(automated_range){
			std::copy(cells.cbegin(),cells.cend(),range.begin());
		}

		Position extensions;
		std::copy(range.cbegin(),range.cend(),extensions.begin());

		// convert unsigned int to int for YaspGrid constructor
		std::array<int,dim> cells_i;
		std::copy(cells.cbegin(),cells.cend(),cells_i.begin());
		auto grid = std::make_shared<Grid>(extensions,cells_i);

		GridWrapper<Grid> gw = {grid,determine_extensions(grid),cells};
		return gw;
	}

	/// Build a rectangular grid
	/** Cells will be rectangular/cubic with edge length 1.
	 *  \tparam dim Spatial dimension of the grid. Default is 2.
	 *  \param cells_xyz Number of cells in each direction.
	 *     Total number will be cells^dim.
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<int dim=2>
	decltype(auto) create_grid(const unsigned int cells_xyz)
	{
		std::array<unsigned int,dim> cells;
		cells.fill(cells_xyz);
		return create_grid<dim>(cells);
	}

	/// Create a set of cells on a grid
	/** The cells will only map to the grid, but not share data with it.
	 *  \param grid Shared pointer to the grid from which to build the cells
	 *  \param f_state Function without arguments returning state
	 *  \param f_traits Function without arguments returning traits
	 *  \return Container with created cells
	*/
	template<
		typename State = int,
		typename Traits = int,
		std::size_t custom_neighborhood_count = 0,
		typename GridType, typename S, typename T=std::function<Traits(void)>
	>
	decltype(auto) create_cells_on_grid(const GridWrapper<GridType> grid_wrapper,
		S f_state, T f_traits=[](){return 0;})
	{

		using GridTypes = GridTypeAdaptor<GridType>;
		using Position = typename GridTypes::Position;
		using GV = typename GridTypes::GridView;
		using Mapper = typename GridTypes::Mapper;
		using Index = typename GridTypes::Index;

		using CellType = Cell<State,Traits,Position,Index,custom_neighborhood_count>;

		auto grid = grid_wrapper._grid;
		GV gv(*grid);
		Mapper mapper(gv);
		CellContainer<CellType> cells;
		cells.reserve(mapper.size());

		// loop over all entities and create cells
		for(const auto& e : elements(gv))
		{
			const Position pos = e.geometry().center();
			const Index id = mapper.index(e);

			// check if entity is at boundary
			bool boundary = false;
			for(const auto& is : intersections(gv,e)){
				if(!is.neighbor()){
					boundary = true;
					break;
				}
			}

			cells.emplace_back(std::make_shared<CellType>
				(f_state(),f_traits(),pos,id,boundary));
		}

		cells.shrink_to_fit();
		return cells;
	}

	/// Create a set of cells with fixed states and traits on a grid.
	/** \param grid Shared pointer to the grid
	 *  \param state Default cell state
	 *  \param traits Default cell traits
	 *  \return Container with created cells
	 */
	template<typename State=int, typename Traits=int, typename GridType>
	decltype(auto) create_cells_on_grid(const std::shared_ptr<GridType>& grid,
		const State& state, const Traits& traits)
	{
		std::function<State(void)> f_state = [&state](){ return state; };
		std::function<Traits(void)> f_traits = [&traits](){ return traits; };
		return create_cells_on_grid<State,Traits,GridType>(grid,f_state,f_traits);
	}

	/// Randomly distribute agents on a grid and connect them to parent cells
	/**
	 *  \param grid Shared pointer to the grid
	 *  \param cells Container of cells
	 *  \param count Number of agents to create
	 *  \param state_initial Initial state of all agents
	 *  \param traits_initial Initial traits of all agents
	 *  \return Container with created agents
	 */
	template<typename State=int, typename Traits=int, typename GridType,
		typename CellContainer>
	decltype(auto) create_agents_on_grid_cells(
		const std::shared_ptr<GridType> grid,
		const CellContainer& cells,
		const std::size_t count,
		const State state_initial,
		const Traits traits_initial)
	{
		// fetch some types
		using GridTypes = GridTypeAdaptor<GridType>;
		static constexpr int dim = GridTypes::dim;
		using GV = typename GridTypes::GridView;
		using Coordinate = typename GridTypes::Coordinate;
		using Position = typename GridTypes::Position;
		using Cell = typename CellContainer::value_type::element_type;
		using Dist = typename std::uniform_real_distribution<Coordinate>;

		GV gv(*grid);
		AgentContainer<Agent<State,Traits,Cell>> agents;

		// find maximum extensions of the grid
		const Coordinate eps = 1e-1;
		std::array<Coordinate,dim> extensions;
		extensions.fill(.0);
		for(const auto& v : vertices(gv)){
			const auto pos = v.geometry().center();
			for(int i = 0; i<dim; ++i){
				extensions.at(i) = std::max(pos[i],extensions.at(i));
			}
		}

		// set up random number generator for positions
		std::ranlux24_base gen(123456);
		std::vector<Dist> dist;
		for(int i = 0; i<dim; ++i){
			dist.push_back(Dist(0.0 + eps , extensions.at(i) - eps));
		}

		// create agents
		for(std::size_t i = 0; i<count; ++i)
		{
			std::shared_ptr<Cell> ptr;
			Position pos;
			for(int i = 0; i<dim; ++i){
				pos[i] = dist[i](gen);
			}
			auto new_agent = std::make_shared<Agent<State,Traits,Cell>>
				(state_initial,traits_initial,pos,ptr);
			new_agent->find_parent(grid,cells);
			agents.push_back(std::move(new_agent));
		}

		return agents;
	}

	template<typename State=int, typename Traits=int, typename Cell>
	decltype(auto) create_agent_on_cell(
		const std::shared_ptr<Cell> cell,
		const State state_initial,
		const Traits traits_initial,
		const typename Cell::Position& position)
	{
		return std::make_shared<Agent<State,Traits,Cell>>
			(state_initial,traits_initial,position,cell);
	}

} // namespace Setup

} // namespace Citcat

#endif // SETUP_HH
