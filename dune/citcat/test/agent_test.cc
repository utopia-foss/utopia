#include <random>
#include <cassert>
#include <dune/citcat/citcat.hh>

int main(int argc, char** argv)
{
	try{
		auto& helper = Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid(50);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

		auto cell_a = cells[1];

		Citcat::Agent<int,int,decltype(cells)::value_type::element_type> 
			agent(0,0,{0.1,0.1},cell_a);

		assert(cell_a->position() == agent.parent()->position());
		assert(cell_a == agent.parent());

		agent.find_parent(grid,cells);
		assert(cells[0] == agent.parent());

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