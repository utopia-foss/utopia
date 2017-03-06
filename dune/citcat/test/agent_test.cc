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

		// set 'wrong' parent
		Citcat::Agent<int,int,decltype(cells)::value_type::element_type> 
			agent(0,0,{0.1,0.1},cell_a);

		// check member access
		assert(cell_a->position() == agent.parent()->position());
		assert(cell_a == agent.parent());

		// find correct parent
		agent.find_parent(grid,cells);
		assert(cells[0] == agent.parent());

		// move agent
		agent.move_along({1.0,1.0});
		decltype(agent)::Position pos({1.1,1.1});
		assert(agent.position() == pos);

		agent.find_parent(grid,cells);
		pos = decltype(agent)::Position({1.0,1.0});
		assert(agent.parent()->position() == pos);

		// move somewhere
		agent.move_to({10.1,10.1});
		pos = decltype(agent)::Position({10.1,10.1});
		assert(agent.position() == pos);

		agent.find_parent(grid,cells);
		pos = decltype(agent)::Position({10.0,10.0});
		assert(agent.parent()->position() == pos);

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