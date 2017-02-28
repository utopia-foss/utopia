#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

#include "grid_cells_test.hh"

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		const int size_3d(14); // use same problem size as in 2D

		auto rect_3d = Citcat::Setup::create_grid<3>(size_3d);
		auto cells_3d = Citcat::Setup::create_cells_on_grid(rect_3d,[](){return 0;});
		
		assert_cells_on_grid(rect_3d,cells_3d);

		Citcat::Setup::apply_periodic_boundaries<3>(cells_3d);
		check_grid_neighbors_count<3>(cells_3d);

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