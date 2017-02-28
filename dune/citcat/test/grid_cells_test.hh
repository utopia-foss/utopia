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