#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

#include "grid_cells_test.hh"

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		const int size_2d(50);

		auto rect_2d = Citcat::Setup::create_grid(size_2d);
		auto cells_2d = Citcat::Setup::create_cells_on_grid(rect_2d,[](){return 0;});

		assert_cells_on_grid(rect_2d,cells_2d);

		Citcat::Setup::apply_periodic_boundaries(cells_2d);
		check_grid_neighbors_count<2>(cells_2d);

		return 0;
	}
	catch(Dune::Exception c){
		std::cerr << c << std::endl;
		return 1;
	}
	catch(...){
		std::cerr << "Unknown exception thrown!" << std::endl;
		return 2;
	}
}