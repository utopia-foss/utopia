#include <dune/citcat/citcat.hh>
#ifdef HAVE_PSGRAF
#include <PSGraf312.a>
#endif

int main(int argc, char** argv) {
	try{ 
		Dune::MPIHelper::instance(argc, argv);

		using State = int;
		using Position = Dune::FieldVector<double,2>;

		std::ranlux24_base gen(123456);
		std::uniform_int_distribution<> dist(0, 3);

		auto grid = Citcat::Setup::create_grid(8);
		auto cells = Citcat::Setup::create_cells_on_grid< int >(grid,[&](){ return dist(gen); });
		auto sim = Citcat::Setup::create_sim_cells(grid,cells);

		sim.add_output(Citcat::Output::plot_time_state_mean(cells));
		sim.add_output(Citcat::Output::eps_plot_cell_state(cells));

		return 0;
	}
	catch (Dune::Exception &e){
		std::cerr << "Dune reported error: " << e << std::endl;
	}
	catch (...){
		std::cerr << "Unknown exception thrown!" << std::endl;
		throw;
	}
}