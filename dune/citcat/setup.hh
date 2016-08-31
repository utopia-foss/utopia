#ifndef SETUP_HH
#define SETUP_HH

namespace Citcat
{
	
/// Functions for building objects and setting up a simulation
namespace Setup
{

	/// Create a grid from a Gmsh file
	/**
	 *  \param filename Name of the Gmsh file with path relative to executable
	 *  \param refinement_level Level of global refinement applied to grid
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	std::shared_ptr<Dune::UGGrid<2>> read_gmsh (const std::string filename, const unsigned int refinement_level=0)
	{
		using Grid = Dune::UGGrid<2>;
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

	/// Build a rectangular 2D grid.
	/** Cell extensions will be 1x1 by default.
	 *  \param cells_x Number of cells in x-direction
	 *  \param cells_y Number of cells in y-direction
	 *  \param range_x Grid extension in x-direction
	 *  \param range_y Grid extension in y-direction
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<typename GridType=DefaultGrid>
	std::shared_ptr<GridType> create_grid(const int cells_x, const int cells_y, float range_x=0.0, float range_y=0.0)
	{
		using GridTypes = GridTypeAdaptor<GridType>;
		using Position = typename GridTypes::Position;
		static constexpr int dim = GridTypes::dim;

		if(range_x==0.0 && range_y==0.0){
			range_x = cells_x;
			range_y = cells_y;
		}
		const Position extensions({range_x,range_y});
		const std::array<int,dim> cells({cells_x,cells_y});

		// place lower left cell center into origin
		const Position deviation({-range_x/(cells_x*2.0),-range_y/(cells_y*2.0)});
		const Position lower_left = deviation;
		const Position upper_right = extensions + deviation;

		return std::make_shared<GridType>(lower_left,upper_right,cells);
	}

	/// Build a rectangular 2D grid
	/** Cell extensions will be 1x1.
	 *  \param cells_xy Number of cells in each direction.
	 *     Total number will be cells^2.
	 *  \return Shared pointer to the grid
	 *  \warning Do not modify the grid after building other structures from it!
	 */
	template<typename GridType=DefaultGrid>
	std::shared_ptr<GridType> create_grid(const int cells_xy)
	{
		return create_grid<GridType>(cells_xy,cells_xy);
	}

	/// Add connections for periodic boundaries to cells on a rectangular grid
	/**
	 *  \param cells Container of cells
	 */
 	template<typename CellContainer>
	void apply_periodic_boundaries (CellContainer& cells)
	{
		using CellPtr = typename CellContainer::value_type;
		std::vector<std::pair<CellPtr,CellPtr>> new_connections;

		for(auto&& cell : cells)
		{
			if(!cell->boundary())
				continue;

			const auto pos = cell->position();
			if(cell->grid_neighbors_count()==2) // Edge cell
			{
				auto it = cells.begin();
				auto end = cells.end();
				// find all adjacent edge cells
				while(it!=end)
				{
					it = std::find_if(it,end,
						[&pos,&cell](const CellPtr& a)
						{
							if(!a->boundary()) return false;
							if(a==cell) return false;
							if(a->grid_neighbors_count()!=2) return false;
							return (a->position()[0] == pos[0]) || 
								(a->position()[1] == pos[1]);
						});
					if(it!=end)
					{
						new_connections.push_back(std::make_pair(cell,*it));
						it++;
					}
				}
			}
			else if(cell->grid_neighbors_count()==3) // border cell
			{
				if(pos[0]==.0) // left border
				{
					// find adjacent border cell
					auto it = std::find_if(cells.begin(),cells.end(),
						[&pos,&cell](const CellPtr& a)
						{
							if(!a->boundary()) return false;
							if(a==cell) return false;
							if(a->grid_neighbors_count()!=3) return false;
							return a->position()[1] == pos[1];
						});
					if(it!=cells.end()){
						new_connections.push_back(std::make_pair(cell,*it));
					}
				}
				else if(pos[1]==.0) // bottom border
				{
					// find adjacent border cell
					auto it = std::find_if(cells.begin(),cells.end(),
						[&pos,&cell](const CellPtr& a)
						{
							if(!a->boundary()) return false;
							if(a==cell) return false;
							if(a->grid_neighbors_count()!=3) return false;
							return a->position()[0] == pos[0];
						});
					if(it!=cells.end()){
						new_connections.push_back(std::make_pair(cell,*it));
					}
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
