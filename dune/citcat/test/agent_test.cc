#include <random>
#include <cassert>
#include <dune/citcat/citcat.hh>

int main(int argc, char** argv)
{
	try{
		constexpr size_t agent_count = 100;
		auto& helper = Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid(50);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});
		// unstructured, non-periodic
		auto m1 = Citcat::Setup::create_manager<false,false>(grid,cells);
		// structured, non-periodic
		auto m2 = Citcat::Setup::create_manager<true,false>(grid,cells);

		cells.clear();

		using Pos = typename Citcat::GridTypeAdaptor<typename decltype(m1.grid())::element_type>::Position;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> dist(0.0,50.0);

		std::vector<std::shared_ptr<Citcat::Agent<int,int,Pos>>> agents;
		for(std::size_t i = 0; i<agent_count; ++i){
			Pos pos({dist(gen),dist(gen)});
			agents.push_back(std::make_shared<Citcat::Agent<int,int,Pos>>(0,0,pos));
		}

		for(auto agent : agents){
			auto cell1 = Citcat::find_cell(agent,m1);
			auto cell2 = Citcat::find_cell(agent,m2);
			if(cell1 != cell2){
				auto pos_a = agent->position();
				auto pos_c1 = cell1->position();
				auto pos_c2 = cell2->position();
				DUNE_THROW(Dune::Exception,"Agent (" + std::to_string(pos_a[0]) + "," + std::to_string(pos_a[1]) + "): Cell 1 (" + std::to_string(pos_c1[0]) + "," + std::to_string(pos_c1[1]) + ")" +
					" Cell 2 (" + std::to_string(pos_c2[0]) + "," + std::to_string(pos_c2[1]) + ")");
			}
		}

/*
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
*/
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