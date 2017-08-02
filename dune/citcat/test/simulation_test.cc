#include <dune/citcat/citcat.hh>
#include <cassert>

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		constexpr unsigned int cell_count = 10;
		constexpr unsigned int agent_count = 10;

		auto grid = Citcat::Setup::create_grid(cell_count);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});
		auto agents = Citcat::Setup::create_agents_on_grid(grid,agent_count,0);
		// structured, non-periodic manager
		auto manager = Citcat::Setup::create_manager<true,false>(grid,cells,agents);

		// create VTK writer
		auto vtkwriter = Citcat::Output::create_vtk_writer(grid._grid,"sim-test");
		vtkwriter->add_adaptor(Citcat::Output::vtk_output_cell_state(manager.cells()));

		// create simulation
		auto sim = Citcat::Setup::create_sim(manager);
		sim.add_output(vtkwriter);

		sim.add_rule([&](const auto cell){
			return 1;
		});
		sim.add_bc([&](const auto cell){
			return 2;
		});

		sim.run(1);

		for(const auto cell : manager.cells()){
			std::cout << "Cell " << std::to_string(cell->index()) << " State " << std::to_string(cell->state()) << std::endl;
		}

		// check states
		for(const auto cell : manager.cells()){
			if (cell->boundary()) {
				if (cell->state() != 2)
					DUNE_THROW(Dune::Exception,"Boundary Cell State not 2");
			}
			else if (!cell->boundary()) {
				if (cell->state() != 1)
				DUNE_THROW(Dune::Exception,"Cell " + std::to_string(cell->index()) + " is not State 1");
			}
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