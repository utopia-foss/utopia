#include <dune/citcat/citcat.hh>

template<typename Cell>
int r1 (Cell cell)
{
	if(cell->state()==0)
		return 1;
	return 0;
}

int main(int argc, char** argv)
{
	try{
		// Maybe initialize MPI
		auto& helper = Dune::MPIHelper::instance(argc, argv);

		using StateType = int;
		using TraitsType = std::array<bool,2>;

		std::function<StateType(void)> state_default = []() { 
			StateType new_state = 1;
			return new_state; };
	 	//std::function<TraitsType(void)> traits_default = []() { return TraitsType({true,false}); };

		auto grid = Setup::create_grid(6);
		auto my_cells = Setup::create_cells_on_grid(grid,state_default);
		Setup::apply_periodic_boundaries(my_cells);
		auto sim = Setup::create_sim_cells(grid,my_cells);

		for(auto&& i : my_cells)
			Neighborhood::Moore::apply(i);

		auto vtkwriter = Output::create_vtk_writer(grid);
		sim.add_output(vtkwriter);
		vtkwriter->add_adaptor(Output::vtk_output_cell_state(my_cells));

		for(const auto& i : my_cells)
		{
			std::cout << std::boolalpha;
			std::cout << i->index() << " , " << i->state() << " , " << i->grid_neighbors_count() << " , " << i->boundary() << " , " << i->neighbors_count() << " , Position: " << i->position()[0] << "," << i->position()[1] << std::endl;
		}

		//typename Sim::StateRule rule1 = [&sim](std::shared_ptr<typename Sim::Cell> c) { return r1(c); };
		sim.add_rule([&](auto c){
			//c->new_state()=2;
			if(c->index()==0 || c->index()==16){
				return 3;
			}
			c->neighbors_count();
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
		sim.run(4);

		//Simulation<Setup,decltype(my_cells)> sim(setup,my_cells);

		//auto x = std::make_shared< GridCell<GV,State,int>>(gv,State::C,3);
		//auto count = x->grid().neighbors_with(State::B);

/*
		Cell<State,int> e(State::A,1);
		auto e1 = std::make_shared< Cell<int> >(State::A,1);
		auto e2 = std::make_shared< Cell<int> >(State::B,2);
		e1->set_neighbor({e2});
		std::cout << e1->neighbors_count() << std::endl;
		std::cout << e1->neighbors_with(State::A) << std::endl;
		std::cout << e1->neighbors_with(State::B) << std::endl;
		e2->set(State::A);
		std::cout << e1->neighbors_count() << std::endl;
		std::cout << e1->neighbors_with(State::A) << std::endl;
		std::cout << e1->neighbors_with(State::B) << std::endl;
*/
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
