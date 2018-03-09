#include <dune/utopia/base.hh>
#include <dune/utopia/setup.hh>

template<class Agent>
void test_cloning (Agent agent)
{
    auto clone = Utopia::clone(agent);
    assert(clone != agent);
    assert(clone->state() == agent->state());
    assert(clone->position() == agent->position());
}

template<class Container, class M1, class M2, class M3>
void compare_cells_of_agents (const Container& cont, const M1& m1, const M2& m2, const M3& m3)
{
    for(auto agent : cont){
        auto cell1 = Utopia::find_cell(agent,m1);
        auto cell2 = Utopia::find_cell(agent,m2);
        auto cell3 = Utopia::find_cell(agent,m3);
        assert(cell1 == cell2 && cell1 == cell3);
    }
}

template<typename Position, class Agent, class Manager>
void move_to_and_back (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
    const auto pos_old = agent->position();
    Utopia::move_to(pos,agent,manager);
    Utopia::move_to(pos_old,agent,manager);
}

template<class MA, class MC>
void compare_agent_cell_coupling (const MA& ma, const MC& mc)
{
    for(auto agent : ma.agents()){
        const auto cell = Utopia::find_cell(agent, mc);
        const auto cell_agents = Utopia::find_agents_on_cell(cell, ma);
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

/// Remove and add agents using container add
template<class M1, class M2>
void test_remove_and_add_container (M1& m1, M2& m2)
{
    // place agent and clone in container
    auto agent = m1.agents().front();
    auto clone = Utopia::clone(agent);
    clone->state() = 42;
    using Agent = std::remove_reference_t<decltype(*clone)>;
    Utopia::AgentContainer<Agent> cont({agent, clone});

    // add container to M1
    assert(Utopia::add(cont, m1));
    assert(std::count(m1.agents().begin(), m1.agents().end(), agent) == 2);
    assert(std::count(m1.agents().begin(), m1.agents().end(), clone) == 1);

    // check that insertion of agent fails on debugging
    const auto ret = Utopia::add<true>(cont, m2);
    assert(!ret[0]);
    // clone must work
    assert(ret[1]);
    assert(m2.agents().back()->state() == 42);
}

/// Remove and add agent using single add
template<class M1, class M2>
void test_remove_and_add_single (M1& m1, M2& m2)
{
    // remove agent from m1
    const auto agent = m1.agents().front();
    Utopia::remove(agent,m1);
    assert(std::find(m2.agents().begin(), m2.agents().end(), agent)
        != m2.agents().end());
    assert(std::find(m1.agents().begin(), m1.agents().end(), agent)
        == m1.agents().end());
    // add agent to back of m1
    assert(Utopia::add(agent,m1));
    assert(m1.agents().back() == agent);
    // check that agent is not inserted, because its found
    assert(!Utopia::add<true>(agent,m2));
    const auto size = m2.agents().size();
    // check that agent is inserted (no debugging)
    assert(Utopia::add<false>(agent,m2));
    assert(m2.agents().size() == size+1);
}

template<typename Manager>
void check_rule_based_removal(Manager& manager)
{
    auto& agents = manager.agents();
    std::size_t n_agents_old = agents.size();

    // set flag to true for some agents (all that have an odd id)
    for (auto a : agents) {
        if (a->id() % 2 == 1) a->is_tagged = true;
    }

    // erase agents
    manager.erase_if([&] (auto a) { return a->is_tagged; });

    // check if there are no agents flagged true
    for (auto a : agents) assert(!a->is_tagged);

    // check if number of agents left is consisted
    assert((agents.size() == std::size_t(n_agents_old/2))
        or (agents.size() == std::size_t(n_agents_old/2)+1));
}

template<int dim>
void test_agents_on_grid (const std::size_t agent_count, const std::size_t grid_size)
{
    auto grid = Utopia::Setup::create_grid<dim>(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid);
    auto agents = Utopia::Setup::create_agents_on_grid<int, Utopia::DefaultTag, std::size_t>(grid,agent_count);

    using Pos = typename Utopia::GridTypeAdaptor<typename decltype(grid._grid)::element_type>::Position;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0,grid_size);
    auto gen1 = [&gen,&dist](){ return dist(gen); };

    // unstructured, non-periodic
    auto ma1 = Utopia::Setup::create_manager_agents<false, false>(grid, agents);
    // structured, non-periodic
    auto ma2 = Utopia::Setup::create_manager_agents<true, false>(grid, agents);
    // structured, periodic
    auto ma3 = Utopia::Setup::create_manager_agents<true, true>(grid, agents);

    // unstructured, non-periodic
    auto mc1 = Utopia::Setup::create_manager_cells<false, false>(grid, cells);
    // structured, non-periodic
    auto mc2 = Utopia::Setup::create_manager_cells<true, false>(grid, cells);
    // structured, periodic
    auto mc3 = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    cells.clear();
    agents.clear();

    // check cloning
    test_cloning(*ma1.agents().begin());

    // check if cells are found correctly
    compare_cells_of_agents(ma1.agents(), mc1, mc2, mc3);

    // check agent movement
    for(auto agent: ma2.agents()){
        Pos pos;
        std::generate(pos.begin(),pos.end(),gen1);
        move_to_and_back(pos, agent, ma1);
        move_to_and_back(pos, agent, ma2);
        move_to_and_back(pos, agent, ma3);
    }
    compare_cells_of_agents(ma1.agents(), mc1, mc2, mc3);

    // check out-of-bounds handling
    for(auto agent: ma3.agents()){
        std::uniform_real_distribution<double> dist2(-2.3*grid_size,2.3*grid_size);
        auto gen2 = [&gen,&dist2](){ return dist2(gen); };
        Pos pos;
        std::generate(pos.begin(),pos.end(),gen2);
        Utopia::move_to(pos,agent,ma3);
    }
    compare_cells_of_agents(ma1.agents(), mc1, mc2, mc3);

    // check correct translation out of grid
    Pos extensions;
    std::fill(extensions.begin(),extensions.end(),grid_size);
    for(auto agent: ma1.agents()){
        const auto pos = agent->position();
        Utopia::move_to(pos+extensions, agent, ma3);
        const auto diff = pos - agent->position();
        assert(diff.two_norm() < 1e-6);
    }

    // check if coupling functions are compliant
    compare_agent_cell_coupling(ma1, mc1);
    compare_agent_cell_coupling(ma2, mc2);
    compare_agent_cell_coupling(ma3, mc3);

    // check add functions
    test_remove_and_add_single(ma1, ma2);
    test_remove_and_add_container(ma1, ma2);

    // check rule-based removal of agents
    check_rule_based_removal(ma3);
}