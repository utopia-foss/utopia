#include <random>
#include <cassert>
#include <dune/citcat/citcat.hh>


template<class M1, class M2, class M3, class Agent>
void compare_cell_of_agent (const std::shared_ptr<Agent> agent,
	const M1& m1, const M2& m2, const M3& m3)
{
	auto cell1 = Citcat::find_cell(agent,m1);
	auto cell2 = Citcat::find_cell(agent,m2);
	auto cell3 = Citcat::find_cell(agent,m3);
	if(cell1 != cell2){
		auto pos_a = agent->position();
		auto pos_c1 = cell1->position();
		auto pos_c2 = cell2->position();
		DUNE_THROW(Dune::Exception,"Agent (" + std::to_string(pos_a[0]) + "," + std::to_string(pos_a[1]) + "): Cell 1 (" + std::to_string(pos_c1[0]) + "," + std::to_string(pos_c1[1]) + ")" +
			" Cell 2 (" + std::to_string(pos_c2[0]) + "," + std::to_string(pos_c2[1]) + ")");
	}
	assert(cell1 == cell2 && cell1 == cell3);
}

int main(int argc, char** argv)
{
	try{
		constexpr size_t agent_count = 1000;
		auto& helper = Dune::MPIHelper::instance(argc,argv);

		auto grid = Citcat::Setup::create_grid(50);
		auto cells = Citcat::Setup::create_cells_on_grid(grid,[](){return 0;});
		// unstructured, non-periodic
		auto m1 = Citcat::Setup::create_manager<false,false>(grid,cells);
		// structured, non-periodic
		auto m2 = Citcat::Setup::create_manager<true,false>(grid,cells);
		// structured, periodic
		auto m3 = Citcat::Setup::create_manager<true,true>(grid,cells);

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

		// check if cells are found correctly
		for(auto agent : agents)
			compare_cell_of_agent(agent,m1,m2,m3);

		// check agent movement
		for(auto agent: agents){
			Pos pos({dist(gen),dist(gen)});
			Citcat::move_to(pos,agent,m1);
			Citcat::move_to(pos,agent,m2);
			Citcat::move_to(pos,agent,m3);
			compare_cell_of_agent(agent,m1,m2,m3);
		}

		// check out-of-bounds handling
		for(auto agent: agents){
			std::uniform_real_distribution<double> dist2(-120.0,120.0);
			Pos pos({dist2(gen),dist2(gen)});
			Citcat::move_to(pos,agent,m3);
			compare_cell_of_agent(agent,m1,m2,m3);
		}

		// check correct translation out of grid
		Pos extensions({50.0,50.0});
		for(auto agent: agents){
			const auto pos = agent->position();
			Citcat::move_to(pos+extensions,agent,m3);
			compare_cell_of_agent(agent,m1,m2,m3);
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