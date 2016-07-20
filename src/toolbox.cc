#include <dune/citcat/citcat.hh>

int i = 0;

template<typename Grid, typename Cells>
void default_sim (const Grid& grid, const Cells& cells)
{
	//try{

		std::cout << "--- SIMULATION NO " << i << " ---" << std::endl;

		auto sim = Setup::create_sim_cells(grid,cells);

		const std::string filename = "sim" + std::to_string(i);
		auto vtkwriter = Output::create_vtk_writer(grid,filename);
		sim.add_output(vtkwriter);
		vtkwriter->add_adaptor(Output::vtk_output_cell_state(cells));
		//vtkwriter->add_adaptor(Output::vtk_output_cell_state_clusters(cells));

		// print out some data
		int boundary_count = 0;
		for(const auto& i : cells)
		{
			/*
			std::cout << std::boolalpha;
			std::cout << i->index() << " , " << i->state() << " , " << i->grid_neighbors_count() << " , " << i->boundary() << " , " << i->neighbors_count() << " , Position: " << i->position()[0] << "," << i->position()[1] << std::endl;
			*/
			if(i->boundary())
				boundary_count++;
		}
		std::cout << "Domain contains " << cells.size() << " cells." << std::endl;
		std::cout << "Domain contains " << boundary_count << " boundary cells." << std::endl; 

		sim.add_rule([&](const auto c){
			//c->new_state()=2;
			if(c->index()==0 || c->index()==16){
				return 3;
			}
			for(const auto& i : c->grid_neighbors()){
				//i->new_state()=2;
				if(i->state()==3)
					return 1;
			}
			for(const auto& i : c->neighbors()){
				//i->new_state()=2;
				if(i->state()==3)
					return 2;
			}
			return 0;});

		sim.iterate(3);
		sim.add_rule([&](const auto c){
			if(c->boundary())
				return 4;
			return 0;
		});
		sim.iterate();
		i++;
	//}
	/*
	catch (...){
		std::cerr << "ERROR!" << std::endl;
		throw;
	}
	*/
}

int main(int argc, char** argv)
{
	try{

		using StateType = int;
		using TraitsType = std::array<bool,2>;

		std::function<StateType(void)> state_default = []() { 
			StateType new_state = 1;
			return new_state; };
		
		auto grid = Setup::create_grid(50);
		auto my_cells = Setup::create_cells_on_grid<StateType,TraitsType>(grid,state_default,[](){
			TraitsType i({true,false});
			return i;});
		Setup::apply_periodic_boundaries(my_cells);
		for(auto&& i : my_cells)
			Neighborhood::Moore::apply(i);

		auto grid_gmsh = Setup::read_gmsh("../../src/square.msh");
		auto my_cells_gmsh = Setup::create_cells_on_grid(grid_gmsh,state_default);

		default_sim(grid,my_cells);
		default_sim(grid_gmsh,my_cells_gmsh);

		return 0;
	}
	catch (Dune::Exception &e){
		std::cerr << "Dune reported error: " << e << std::endl;
		throw;
	}
	catch (...){
		std::cerr << "Unknown exception thrown!" << std::endl;
		throw;
	}
}
