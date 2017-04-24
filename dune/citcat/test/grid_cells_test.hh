#include <cassert>
#include <dune/common/exceptions.hh>
#include <dune/citcat/citcat.hh>

/// Check if the cells on a periodic grid have the correct number of neighbors
template<int dim, typename CellContainer>
void check_grid_neighbors_count(CellContainer& cells)
{
	const int nb_count = (dim == 2 ? 4 : 6);

	bool exception = false;
	for(auto&& cell : cells){
		if(cell->grid_neighbors_count() != nb_count){
			std::cerr << "Cell No. " << cell->index()
				<< " has " << cell->grid_neighbors_count()
				<< " neighbors!" << std::endl;
			exception = true;
		}
	}
	if(exception){
		DUNE_THROW(Dune::Exception,"Wrong number of neighbors!");
	}
}

template<typename Grid, typename CellContainer>
void assert_cells_on_grid(std::shared_ptr<Grid> grid, CellContainer& cells)
{
	using GridTypes = Citcat::GridTypeAdaptor<Grid>;
	using Mapper = typename GridTypes::Mapper;
	auto gv = grid->leafGridView();
	Mapper mapper(gv);

	for(const auto& e : elements(gv)){
		const auto id = mapper.index(e);

		// check if entity is contained via index
		auto it = std::find_if(cells.cbegin(),cells.cend(),
			[id](const auto c){return c->index() == id;});
		assert(it != cells.cend());

		// check if position is correct
		const auto cell = *it;
		assert(cell->position() == e.geometry().center());

		// check if boundary info is correct
		bool boundary = false;
		for(const auto& is : intersections(gv,e)){
			if(!is.neighbor()){
				boundary = true;
				break;
			}
		}
		assert(cell->boundary() == boundary);
	}
}

/// Assure that periodic grid has the correct NextNeighbor count
template<typename Manager>
void check_grid_neighbors_count (const Manager& manager)
{
	const int nb_count = (Manager::Traits::dim == 2 ? 4 : 6);

	bool exception = false;
	for(const auto cell : manager._cells){
		const auto neighbors = Citcat::Neighborhoods::NextNeighbor::neighbors(manager,cell);
		if(neighbors.size() != nb_count){
			std::cerr << "Cell No. " << cell->index()
				<< " has " << neighbors.size()
				<< " neighbors!" << std::endl;
			exception = true;
		}
	}
	if(exception){
		DUNE_THROW(Dune::Exception,"Wrong number of neighbors!");
	}
}

/// Compare the neighborhood implementations for two manager types
template<typename M1, typename M2>
void compare_neighborhoods (const M1& m1, const M2& m2)
{
	for(std::size_t i=0; i<m1._cells.size(); ++i){
		const auto nb1 = Citcat::Neighborhoods::NextNeighbor::neighbors(m1,m1._cells[i]);
		const auto nb2 = Citcat::Neighborhoods::NextNeighbor::neighbors(m2,m2._cells[i]);
		// check size
		assert(nb1.size() == nb2.size());
		// check actual neighbors
		for(auto a : nb1){
			assert(std::find_if(nb2.begin(),nb2.end(),
					[&a](auto b){ return a == b; })
				!= nb2.end());
		}
	}
}

/// Compare custom and true neighborhood for a cell which is not at the boundary
template<typename Manager>
void compare_custom_and_true_neighborhoods(const Manager& manager)
{
	for(auto cell : manager._cells){
		if(cell->boundary()){
			continue;
		}
		const auto nb1 = Citcat::Neighborhoods::NextNeighbor::neighbors(manager,cell);
		const auto nb2 = Citcat::Neighborhoods::Custom<0>::neighbors(cell);
		// check size
		assert(nb1.size() == nb2.size());
		// check actual neighbors
		for(auto a : nb1){
			assert(std::find_if(nb2.begin(),nb2.end(),
					[&a](auto b){ return a == b; })
				!= nb2.end());
		}
	}
}

/// Perform a test: Assure that cells are instantiated correctly and neighborhood implementations mirror each other
template<int dim>
void cells_on_grid_test (const unsigned int cells_per_dim)
{
	auto grid = Citcat::Setup::create_grid<dim>(cells_per_dim);
	auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

	// structured, non-periodic
	using M1 = typename Citcat::GridManager<typename decltype(grid)::element_type,true,false,typename decltype(cells)::value_type::element_type>;
	// unstructured, non-periodic
	using M2 = typename Citcat::GridManager<typename decltype(grid)::element_type,false,false,typename decltype(cells)::value_type::element_type>;
	// structured, periodic
	using M3 = typename Citcat::GridManager<typename decltype(grid)::element_type,true,true,typename decltype(cells)::value_type::element_type>;
	M1 m1(grid);
	M2 m2(grid);
	M3 m3(grid);
	m1._cells = cells;
	m2._cells = cells;
	m3._cells = cells;
	cells.clear(); // ensure that original container is empty

	// insert cell counts by hand
	std::array<unsigned int,dim> grid_cells;
	std::fill(grid_cells.begin(),grid_cells.end(),cells_per_dim);
	m1.grid_cells() = grid_cells;
	m2.grid_cells() = grid_cells;
	m3.grid_cells() = grid_cells;

	// assert correct initialization on grid
	assert_cells_on_grid(grid,m1._cells);
	assert_cells_on_grid(grid,m2._cells);
	assert_cells_on_grid(grid,m3._cells);

	// compare neighborhood implementations (structured,unstructured)
	compare_neighborhoods(m1,m2);

	// compare saved vs calculated neighborhoods
	compare_custom_and_true_neighborhoods(m1);
	compare_custom_and_true_neighborhoods(m2);

	// check periodic boundaries
	check_grid_neighbors_count(m3);
	Citcat::Setup::apply_periodic_boundaries(m3._cells);
	compare_custom_and_true_neighborhoods(m3);
}