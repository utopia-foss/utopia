#ifndef SETUP_HH
#define SETUP_HH

namespace Citcat
{
	
/// Functions for building objects and setting up a simulation
namespace Setup
{

	/// Create an unstructured grid from a Gmsh file
	/**
	 *  \tparam dim Spatial dimension of grid and mesh
	 *  \param filename Name of the Gmsh file with path relative to executable
	 *  \param refinement_level Level of global refinement applied to grid
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<int dim=2>
	std::shared_ptr<Dune::UGGrid<dim>> read_gmsh (const std::string filename, const unsigned int refinement_level=0)
	{
		using Grid = Dune::UGGrid<dim>;
		auto grid = std::make_shared<Grid>();

		Dune::GridFactory<Grid> factory(grid.get());
		Dune::GmshReader<Grid>::read(factory,filename);
		factory.createGrid();

		grid->globalRefine(refinement_level);
		return grid;
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
	std::shared_ptr<DefaultGrid<dim>> create_grid (const std::array<int,dim> cells, std::array<float,dim> range = std::array<float,dim>())
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

		Position deviation;
		for(int i=0; i<dim; ++i)
			deviation[i] = -range.at(i)/(cells.at(i)*2.0);

		// place lower left cell center into origin
		const Position lower_left = deviation;
		const Position upper_right = extensions + deviation;

		return std::make_shared<Grid>(lower_left,upper_right,cells);
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
	decltype(auto) create_grid(const int cells_xyz)
	{
		std::array<int,dim> cells;
		cells.fill(cells_xyz);
		return create_grid<dim>(cells);
	}

	/// Add connections for periodic boundaries to cells on a rectangular grid
	/**
	 *  \param cells Container of cells
	 */
 	template<int dim=2, typename CellContainer>
	void apply_periodic_boundaries (CellContainer& cells)
	{
		using CellPtr = typename CellContainer::value_type;
		using PBA = Low::PeriodicBoundaryApplicator<dim,CellPtr>;

		std::vector<std::pair<CellPtr,CellPtr>> new_connections;

		// get the grid extensions
		std::array<double,dim> extensions;
		extensions.fill(.0);
		std::cout << "Extensions :";
		for(auto&& cell : cells){
			const auto pos = cell->position();
			for(int i = 0; i<dim; ++i){
				extensions.at(i) = std::max(pos[i],extensions.at(i));
				std::cout << extensions.at(i) << ",";
			}
		}
		std::cout << std::endl;

		PBA pba(extensions);

		for(auto&& cell : cells)
		{
			if(!cell->boundary())
				continue;

			std::function<bool(CellPtr)> f_search;

			if(pba.is_corner_cell(cell)){
				std::cout << "Checking Cell: " << cell->index() << ": " << std::endl;
				f_search = std::bind(&PBA::check_corner_cell,&pba,cell,std::placeholders::_1);
			}
			else if(pba.is_edge_cell(cell)){
				f_search = std::bind(&PBA::check_edge_cell,&pba,cell,std::placeholders::_1);
			}
			else if(pba.is_surface_cell(cell)){
				f_search = std::bind(&PBA::check_surface_cell,&pba,cell,std::placeholders::_1);
			}
			else{
				continue;
			}

			auto it = cells.begin();
			const auto end = cells.end();

			while(it!=end){
				it = std::find_if(it,end,f_search);
				if(it!=end){
					new_connections.push_back(std::make_pair(cell,*it));
					it++;
				}
			}
		}

		// apply new connections
		for(auto&& pair : new_connections)
		{
			auto i = std::get<0>(pair);
			auto j = std::get<1>(pair);
			i->add_grid_neighbor(j);
			j->add_grid_neighbor(i);
		}
	}

	/// Create a set of cells on a grid
	/** The cells will only map to the grid, but not share data with it.
	 *  \param grid Shared pointer to the grid from which to build the cells
	 *  \param f_state Function without arguments returning state
	 *  \param f_traits Function without arguments returning traits
	 *  \return Container with created cells
	*/
	template<typename State=int, typename Traits=int, typename GridType,
		typename S, typename T=std::function<Traits(void)>>
	decltype(auto) create_cells_on_grid(const std::shared_ptr<GridType> grid,
		S f_state, T f_traits=[](){return 0;})
	{
		//static_assert(std::is_convertible<State,typename std::result_of<S>::type>::value,"This function does not return a variable of type 'State'!");
		//static_assert(std::is_convertible<Traits,typename std::result_of<T>::type>::value,"This functions does not return a variable of type 'Traits'!");

		using GridTypes = GridTypeAdaptor<GridType>;
		using Position = typename GridTypes::Position;
		static constexpr int dim = GridTypes::dim;
		using GV = typename GridTypes::GridView;
		using Mapper = typename GridTypes::Mapper;
		using Index = typename GridTypes::Index;

		GV gv(*grid);
		Mapper mapper(gv);
		CellContainer<Cell<State,Traits,Position,Index>> cells;
		cells.reserve(mapper.size());

		// loop over all entities and create cells
		for(auto it=gv.template begin<0>(); it!=gv.template end<0>(); ++it)
		{
			const Position pos = it->geometry().center();
			const Index id = mapper.index(*it);
			bool boundary = false;
			for(auto iit=gv.ibegin(*it); iit!=gv.iend(*it); ++iit)
			{
				if(!iit->neighbor())
				{
					boundary = true;
					break;
				}
			}
			cells.push_back(std::make_shared<Cell<State,Traits,Position,Index>>(f_state(),f_traits(),pos,id,boundary));
		}

		std::map<Index,std::shared_ptr<Cell<State,Traits,Position,Index>>> map;
		for(const auto& i: cells) 
			map.emplace(i->index(),i);

		// add grid neighbors
		for(auto it=gv.template begin<0>(); it!=gv.template end<0>(); ++it)
		{
			// find current cell
			const Index id = mapper.index(*it);
			auto cell = map.find(id)->second;
			// loop over intersections of grid entity
			for(auto iit=gv.ibegin(*it); iit!=gv.iend(*it); ++iit)
			{
				if(!iit->neighbor()) continue;
				const Index id_nb = mapper.index(iit->outside());
				auto cell_nb = map.find(id_nb)->second;
				cell->add_grid_neighbor(cell_nb);
			}
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

} // namespace Setup

} // namespace Citcat

#endif // SETUP_HH
