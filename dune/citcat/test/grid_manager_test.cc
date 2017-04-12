#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

#include "grid_cells_test.hh"

/// Assure that periodic grid has the correct NextNeighbor count
template<typename Manager>
void check_grid_neighbors_count (const Manager& manager)
{
	const int nb_count = (Manager::Traits::dim == 2 ? 4 : 6);

	bool exception = false;
	for(const auto cell : manager._cells){
		const auto neighbors = Citcat::Neighborhoods::NextNeighbor<Manager>::neighbors(manager,cell);
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

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid<3>(10);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

		using Manager = typename Citcat::GridManager<decltype(grid)::element_type,true,true,decltype(cells)::value_type::element_type>;
		Manager manager(grid);
		manager._cells = cells;
		cells.clear(); // ensure that original container is empty

		manager.grid_cells() = std::array<unsigned int,3>({10,10,10});

		assert_cells_on_grid(grid,manager._cells);
		check_grid_neighbors_count(manager);

		return 0;
	}
	catch(Dune::Exception c){
		std::cerr << c << std::endl;
		throw;
	}
	catch(...){
		std::cerr << "Unknown exception thrown!" << std::endl;
		throw;
	}
}