#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto rect_2d = Citcat::Setup::create_grid({5,10},{1.0,1.0});
		auto cells_2d = Citcat::Setup::create_cells_on_grid(rect_2d,[](){return 0;});
		assert(cells_2d.size() == 5 * 10);

		Citcat::Setup::apply_periodic_boundaries(cells_2d);

		auto vtkwriter = Citcat::Output::create_vtk_writer(rect_2d);
		vtkwriter->add_adaptor(Citcat::Output::vtk_output_cell_state(cells_2d));
		cells_2d[1]->new_state() = 2;
		cells_2d[1]->update();
		for(auto&& nb: cells_2d[1]->grid_neighbors()){
			nb->new_state() = 1;
			nb->update();
		}
		vtkwriter->write(0);

		bool exception = false;
		for(auto&& cell : cells_2d){
			if(cell->grid_neighbors_count() != 4){
				std::cerr << "Cell No. " << cell->index()
					<< " has " << cell->grid_neighbors_count()
					<< " neighbors!" << std::endl;
				exception = true;
			}
		}
		if(exception){
			DUNE_THROW(Dune::Exception,"Wrong number of neighbors!");
		}

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