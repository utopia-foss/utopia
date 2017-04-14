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

/// Compare the neighborhood implementations for two manager types
template<typename M1, typename M2>
void compare_neighborhoods (const M1& m1, const M2& m2)
{
	for(std::size_t i=0; i<m1._cells.size(); ++i){
		const auto nb1 = Citcat::Neighborhoods::NextNeighbor<M1>::neighbors(m1,m1._cells[i]);
		const auto nb2 = Citcat::Neighborhoods::NextNeighbor<M2>::neighbors(m2,m2._cells[i]);
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

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid<3>(10);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

		using M1 = typename Citcat::GridManager<decltype(grid)::element_type,true,false,decltype(cells)::value_type::element_type>;
		using M2 = typename Citcat::GridManager<decltype(grid)::element_type,false,false,decltype(cells)::value_type::element_type>;
		using M3 = typename Citcat::GridManager<decltype(grid)::element_type,true,true,decltype(cells)::value_type::element_type>;
		M1 m1(grid);
		M2 m2(grid);
		M3 m3(grid);
		m1._cells = cells;
		m2._cells = cells;
		m3._cells = cells;
		cells.clear(); // ensure that original container is empty

		m1.grid_cells() = std::array<unsigned int,3>({10,10,10});
		m2.grid_cells() = std::array<unsigned int,3>({10,10,10});
		m3.grid_cells() = std::array<unsigned int,3>({10,10,10});

		assert_cells_on_grid(grid,m1._cells);
		assert_cells_on_grid(grid,m2._cells);
		assert_cells_on_grid(grid,m3._cells);

		// check periodic boundaries
		check_grid_neighbors_count(m3);

		// check neighborhood implementations
		compare_neighborhoods(m1,m2);

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