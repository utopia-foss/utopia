#include <cassert>
#include <dune/common/exceptions.hh>
#include <dune/citcat/citcat.hh>

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
	for(const auto cell : manager.cells()){
		const auto neighbors = Citcat::Neighborhoods::NextNeighbor::neighbors(cell,manager);
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
	for(std::size_t i=0; i<m1.cells().size(); ++i){
		const auto nb1 = Citcat::Neighborhoods::NextNeighbor::neighbors(m1.cells()[i],m1);
		const auto nb2 = Citcat::Neighborhoods::NextNeighbor::neighbors(m2.cells()[i],m2);
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
	auto cells = Citcat::Setup::create_cells_on_grid(grid);

	// structured, non-periodic
	auto m1 = Citcat::Setup::create_manager<true,false>(grid,cells);
	// unstructured, non-periodic
	auto m2 = Citcat::Setup::create_manager<false,false>(grid,cells);
	// structured, periodic
	auto m3 = Citcat::Setup::create_manager<true,true>(grid,cells);

	cells.clear(); // ensure that original container is empty

	// assert correct initialization on grid
	assert_cells_on_grid(m1.grid(),m1.cells());
	assert_cells_on_grid(m2.grid(),m2.cells());
	assert_cells_on_grid(m3.grid(),m3.cells());

	// compare neighborhood implementations (structured,unstructured)
	compare_neighborhoods(m1,m2);

	// check periodic boundaries
	check_grid_neighbors_count(m3);

}