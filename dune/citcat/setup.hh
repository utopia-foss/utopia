#ifndef SETUP_HH
#define SETUP_HH

/// Class for building objects.
class Setup
{
private:

	using Grid = typename StaticTypes::Grid::Type;
	using GV = typename StaticTypes::Grid::GridView;
	using Domain = typename StaticTypes::Grid::Domain;
	using Mapper = typename StaticTypes::Grid::Mapper;
	using Index = typename StaticTypes::Grid::Index;
	using ElementIterator = typename StaticTypes::Grid::ElementIterator;

	static const int dim = StaticTypes::Grid::dim;

public:
	/// Default constructor.
	Setup () = default;

	/// Create a simulation object from a grid and a set of cells
	template<typename GridType, typename CellContainer>
	static decltype(auto) create_sim_cells (const std::shared_ptr<GridType>& grid, CellContainer& cells)
	{
		using DataType = SimulationWrapper<GridType,CellContainer,StaticTypes::EmptyContainer>; 
		StaticTypes::EmptyContainer individuals;
		DataType data(grid,cells,individuals);
		Simulation<DataType> sim(data);
		return sim;
	}

	/// Build a grid and return a shared pointer to it
	/** Cell extensions will be 1x1.
	 * \param cells_xy Number of cells in each direction.
	 *   Total number will be cells^2.
	 */
	template<typename GridType=Grid>
	static decltype(auto) create_grid(const int cells_xy)
	{
		return create_grid(cells_xy,cells_xy);
	}

	/// Build a grid and return a shared pointer to it.
	/** Cell extensions will be 1x1 by default.
	 *  \param cells_x Number of cells in x-direction
	 *  \param cells_y Number of cells in y-direction
	 *  \param range_x Grid extension in x-direction
	 *  \param range_y Grid extension in y-direction
	 */
	template<typename GridType=Grid>
	static std::shared_ptr<GridType> create_grid(const int cells_x, const int cells_y, float range_x=0.0, float range_y=0.0)
	{
		if(range_x==0.0 && range_y==0.0){
			range_x = cells_x;
			range_y = cells_y;
		}
		const Domain extensions({range_x,range_y});
		const std::array<int,dim> cells({cells_x,cells_y});

		// place lower left cell center into origin
		const Domain deviation({-range_x/(cells_x*2.0),-range_y/(cells_y*2.0)});
		const Domain lower_left = deviation;
		const Domain upper_right = extensions + deviation;

		return std::make_shared<GridType>(lower_left,upper_right,cells);
	}

	/// Add connections for periodic boundaries to cells on a rectangular grid
	/**
	 *  \param cells Container of cells
	 */
 	template<typename CellContainer>
	static void apply_periodic_boundaries (CellContainer& cells)
	{
		using CellPtr = typename CellContainer::value_type;
		std::vector<std::pair<CellPtr,CellPtr>> new_connections;

		// loop over all cells
		for(auto&& cell : cells)
		{
			if(!cell->boundary())
				continue;

			const Domain pos = cell->position();
			if(cell->grid_neighbors_count()==2) // Edge cell
			{
				auto it = cells.begin();
				auto end = cells.end();
				// find all adjaced edge cells
				while(it!=end)
				{
					it = std::find_if(it,end,
						[&pos,&cell](const CellPtr& a)
						{
							if(!a->boundary())
								return false;
							if(a==cell)
								return false;
							if(a->grid_neighbors_count()!=2)
								return false;
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
			//std::cout << "-> Cell " << i->index() << " will be connected with cell " << j->index() << std::endl;
		}
	}

	/// Create a set of cells on the grid
	/** \param f_state Function without arguments returning state
	 *  \param f_traits Function without arguments returning traits
	*/
	template<typename State=int, typename Traits=int, typename GridType, typename S, typename T=std::function<Traits(void)>>
	static CellContainer<Cell<State,Traits>> create_cells_on_grid(const std::shared_ptr<GridType> grid, S f_state, T f_traits=[](){return 0;})
	{
		//static_assert(std::is_convertible<State,typename std::result_of<S>::type>::value,"This function does not return a variable of type 'State'!");
		//static_assert(std::is_convertible<Traits,typename std::result_of<T>::type>::value,"This functions does not return a variable of type 'Traits'!");

		GV gv(*grid);
		Mapper mapper(gv);
		CellContainer<Cell<State,Traits>> cells;
		cells.reserve(mapper.size());

		// loop over all entities and create cells
		for(auto it=gv.template begin<0>(); it!=gv.template end<0>(); ++it)
		{
			const Domain pos = it->geometry().center();
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
			cells.push_back(std::make_shared<Cell<State,Traits>>(f_state(),f_traits(),pos,id,boundary));
		}

		std::map<Index,std::shared_ptr<Cell<State,Traits>>> map;
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
	/** \param state Default cell state
	 *  \param traits Default cell traits
	 */
	template<typename State=int, typename Traits=int, typename GridType>
	static decltype(auto) create_cells_on_grid(const std::shared_ptr<GridType>& grid, const State& state, const Traits& traits)
	{
		std::function<State(void)> f_state = [&state](){ return state; };
		std::function<Traits(void)> f_traits = [&traits](){ return traits; };
		return create_cells_on_grid<State,Traits,GridType>(grid,f_state,f_traits);
	}
};

#endif // SETUP_HH
