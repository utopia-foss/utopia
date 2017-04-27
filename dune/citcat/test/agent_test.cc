#include <random>
#include <cassert>
#include <dune/citcat/citcat.hh>


template<class M1, class M2, class M3>
void compare_cells_of_agents (const M1& m1, const M2& m2, const M3& m3)
{
	for(auto agent : m1.agents()){
		auto cell1 = Citcat::find_cell(agent,m1);
		auto cell2 = Citcat::find_cell(agent,m2);
		auto cell3 = Citcat::find_cell(agent,m3);
		assert(cell1 == cell2 && cell1 == cell3);
	}
}

int main(int argc, char** argv)
{
	try{
		constexpr size_t agent_count = 1000;
		auto& helper = Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid(50);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});

		using Pos = typename Citcat::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> dist(0.0,50.0);

		Citcat::AgentContainer<Citcat::Agent<int,int,Pos>> agents;
		for(std::size_t i = 0; i<agent_count; ++i){
			Pos pos({dist(gen),dist(gen)});
			agents.push_back(std::make_shared<Citcat::Agent<int,int,Pos>>(0,0,pos));
		}

		// unstructured, non-periodic
		auto m1 = Citcat::Setup::create_manager<false,false>(grid,cells,agents);
		// structured, non-periodic
		auto m2 = Citcat::Setup::create_manager<true,false>(grid,cells,agents);
		// structured, periodic
		auto m3 = Citcat::Setup::create_manager<true,true>(grid,cells,agents);

		cells.clear();
		agents.clear();

		// check if cells are found correctly
		compare_cells_of_agents(m1,m2,m3);

		// check agent movement
		for(auto agent: m2.agents()){
			Pos pos({dist(gen),dist(gen)});
			Citcat::move_to(pos,agent,m1);
			Citcat::move_to(pos,agent,m2);
			Citcat::move_to(pos,agent,m3);
		}
		compare_cells_of_agents(m1,m2,m3);

		// check out-of-bounds handling
		for(auto agent: m3.agents()){
			std::uniform_real_distribution<double> dist2(-120.0,120.0);
			Pos pos({dist2(gen),dist2(gen)});
			Citcat::move_to(pos,agent,m3);
		}
		compare_cells_of_agents(m1,m2,m3);

		// check correct translation out of grid
		Pos extensions({50.0,50.0});
		for(auto agent: m1.agents()){
			const auto pos = agent->position();
			Citcat::move_to(pos+extensions,agent,m3);
			const auto diff = pos - agent->position();
			assert(diff.two_norm() < 1e-6);
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