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

template<typename Position, class Agent, class Manager>
void move_to_and_back (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
	const auto pos_old = agent->position();
	Citcat::move_to(pos,agent,manager);
	Citcat::move_to(pos_old,agent,manager);
}

template<class Manager>
void compare_agent_cell_coupling (const Manager& manager)
{
	for(auto agent : manager.agents()){
		const auto cell = Citcat::find_cell(agent,manager);
		const auto cell_agents = Citcat::find_agents_on_cell(cell,manager);
		if(std::find(cell_agents.begin(),cell_agents.end(),agent)
			== cell_agents.end()){
			std::cout << "Agent: ";
			const auto& pos_a = agent->position();
			std::for_each(pos_a.begin(),pos_a.end(),[](const auto a){std::cout << a << " ";});
			std::cout << std::endl << "Cell: ";
			const auto& pos_c = cell->position();
			std::for_each(pos_c.begin(),pos_c.end(),[](const auto a){std::cout << a << " ";});
			std::cout << std::endl;
			assert(false);
		}
	}
}

template<int dim>
void test_agents_on_grid (const std::size_t agent_count, const std::size_t grid_size)
{
	auto grid = Citcat::Setup::create_grid<dim>(grid_size);
	auto cells = Citcat::Setup::create_cells_on_grid(grid);
	auto agents = Citcat::Setup::create_agents_on_grid(grid,agent_count);

	using Pos = typename Citcat::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<double> dist(0.0,grid_size);
	auto gen1 = [&gen,&dist](){ return dist(gen); };

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
		Pos pos;
		std::generate(pos.begin(),pos.end(),gen1);
		move_to_and_back(pos,agent,m1);
		move_to_and_back(pos,agent,m2);
		move_to_and_back(pos,agent,m3);
	}
	compare_cells_of_agents(m1,m2,m3);

		// check out-of-bounds handling
	for(auto agent: m3.agents()){
		std::uniform_real_distribution<double> dist2(-2.3*grid_size,2.3*grid_size);
		auto gen2 = [&gen,&dist2](){ return dist2(gen); };
		Pos pos;
		std::generate(pos.begin(),pos.end(),gen2);
		Citcat::move_to(pos,agent,m3);
	}
	compare_cells_of_agents(m1,m2,m3);

		// check correct translation out of grid
	Pos extensions;
	std::fill(extensions.begin(),extensions.end(),grid_size);
	for(auto agent: m1.agents()){
		const auto pos = agent->position();
		Citcat::move_to(pos+extensions,agent,m3);
		const auto diff = pos - agent->position();
		assert(diff.two_norm() < 1e-6);
	}

	// check if coupling functions are compliant
	compare_agent_cell_coupling(m1);
	compare_agent_cell_coupling(m2);
	compare_agent_cell_coupling(m3);

	// check removal and addition of agents
	const auto agent = m1.agents().front();
	Citcat::remove(agent,m1);
	assert(std::find(m2.agents().begin(),m2.agents().end(),agent)!=m2.agents().end());
	assert(std::find(m1.agents().begin(),m1.agents().end(),agent)==m1.agents().end());
	assert(Citcat::add(agent,m1));
	assert(m1.agents().back() == agent);
	assert(!Citcat::add(agent,m2));
}