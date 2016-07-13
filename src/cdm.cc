#include <dune/toolbox/toolbox.hh>

int main(int argc, char** argv)
{
	try{
		std::ranlux24_base gen(123456);
		std::uniform_int_distribution<> dist(0, 1);

		auto grid = Setup::create_grid(500);
		auto my_cells = Setup::create_cells_on_grid(grid,[&](){ return dist(gen);});
		auto sim = Setup::create_sim_cells(grid,my_cells);
		Setup::apply_periodic_boundaries(my_cells);

		for(auto&& i : my_cells)
			Neighborhood::vonNeumann::apply(i);

		sim.add_output(Output::plot_time_state_mean(my_cells,"mean.csv"));
		sim.add_output(Output::plot_time_state_density(my_cells,"density.csv",0,2));
		auto vtkwriter = Output::create_vtk_writer(grid);
		sim.add_output(vtkwriter,20);
		vtkwriter->add_adaptor(Output::vtk_output_cell_state(my_cells));

		const auto prob_growth = 0.0075;
		const auto prob_fire = 1e-6;

		std::uniform_real_distribution<float> dist_2(.0,.1);

		sim.add_rule([&](const auto c){
			if(c->state()==2)
				return 0;
			if(c->state()==0){
				if(dist_2(gen)<prob_growth)
					return 1;
				return 0;
			}
			if(dist_2(gen)<prob_fire)
				return 2;
			for(const auto& i : c->neighbors()){
				if(i->state()==2)
					return 2;
			}
			return 1;
			});

		sim.run(500);

		return 0;
	}
	catch (Dune::Exception &e){
		std::cerr << "Dune reported error: " << e << std::endl;
		throw;
	}
	catch (...){
		std::cerr << "Unknown exception thrown!" << std::endl;
	}
}