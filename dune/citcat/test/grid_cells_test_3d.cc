#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto rect_3d = Citcat::Setup::create_grid<3>(10);
		auto cells_3d = Citcat::Setup::create_cells_on_grid(rect_3d,[](){return 0;});
		assert(cells_3d.size() == 10 * 10 * 10);

		Citcat::Setup::apply_periodic_boundaries<3>(cells_3d);

		auto vtkwriter = Citcat::Output::create_vtk_writer(rect_3d);
		vtkwriter->add_adaptor(Citcat::Output::vtk_output_cell_state(cells_3d));
		cells_3d[119]->new_state() = 2;
		cells_3d[119]->update();
		for(auto&& nb: cells_3d[119]->grid_neighbors()){
			nb->new_state() = 1;
			nb->update();
		}
		vtkwriter->write(0);

		bool exception = false;
		for(auto&& cell : cells_3d){
			if(cell->grid_neighbors_count() != 6){
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