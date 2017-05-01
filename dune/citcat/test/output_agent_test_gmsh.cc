#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		// create two grids
		auto gmsh_2d = Citcat::Setup::read_gmsh("square.msh",2);

		// cells on gmsh
		auto cells = Citcat::Setup::create_cells_on_grid(gmsh_2d,[](){return 0;});
		auto agents = Citcat::Setup::create_agents_on_grid(gmsh_2d,100,0);

		auto m1 = Citcat::Setup::create_manager<false,false>(gmsh_2d,cells,agents);

		auto vtkwriter = Citcat::Output::create_vtk_writer(gmsh_2d._grid,"simplex");
		vtkwriter->add_adaptor(Citcat::Output::vtk_output_agent_count_per_cell(m1));
		vtkwriter->write(0);
		
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