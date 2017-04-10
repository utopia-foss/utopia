#include <dune/citcat/citcat.hh>

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid(10);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

		using Manager = typename Citcat::GridManager<decltype(grid)::element_type,true,true,decltype(cells)::value_type::element_type>;
		Manager manager(grid);
		manager._cells = cells;
		cells.clear();

		manager.grid_cells() = std::array<unsigned int,2>({10,10});

		auto c1 = manager._cells[0];
		auto neighbors = Citcat::Neighborhoods::NextNeighbor<Manager>::neighbors(manager,c1);

		const auto& pos = c1->position();
		std::cout << "Cell " << c1->index() << " at " << pos[0] << ":" << pos[1] << std::endl;
		for(auto nb : neighbors){
			const auto& pos_nb = nb->position();
			std::cout << "Cell " << nb->index() << " at " << pos_nb[0] << ":" << pos_nb[1] << std::endl;
		}

		//assert_cells_on_grid(grid,manager._cells);
		//check_grid_neighbors_count<2>(cells_2d);

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