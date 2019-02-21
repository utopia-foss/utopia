#include <cassert>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/apply.hh>

template<class Manager>
decltype(auto) get_rule_acc_neighbors_with_mngr (Manager& manager){
    auto rule = [&manager](const auto cell) {
        auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(cell, manager);
        return std::accumulate(nb.begin(), nb.end(), 1,
            [](const auto val, const auto neighbor){
                return val + neighbor->state();
        });
    };
    return rule;
}

int main(int, char* []) {
    try {
        std::cout << "Testing apply_rule..." << std::endl << std::endl;

        constexpr bool sync = true;
        constexpr bool async = false;

        // create a random number generator
        Utopia::DefaultRNG rng(42);

        // build the ingredients for a manager
        auto grid = Utopia::Setup::create_grid(5);
        auto cells = Utopia::Setup::create_cells_on_grid<sync>(grid);

        // build a manager - structured, periodic
        auto m_sync = Utopia::Setup::create_manager_cells<true, true>(
            grid, cells);

        // define a rule which yields different results for sync and async
        auto rule_acc_neighbors_sync = get_rule_acc_neighbors_with_mngr(m_sync);

        // apply the rule in a synchronous fashion
        Utopia::apply_rule(rule_acc_neighbors_sync, m_sync.cells());
        assert(std::all_of(m_sync.cells().begin(), m_sync.cells().end(),
            [](const auto cell){ return cell->state() == 1; }
        ));

        // repeat test for asynchronous application
        auto cells2 = Utopia::Setup::create_cells_on_grid<async>(grid);

        // build a manager - structured, periodic
        auto m_async = Utopia::Setup::create_manager_cells<true, true>(
            grid, cells2);

        // store order of cells
        std::vector<unsigned int> ids;
        std::transform(m_async.cells().begin(), m_async.cells().end(),
            std::back_inserter(ids),
            [](const auto cell){ return cell->id(); }
        );

        // apply the rule in a asynchronous fashion
        auto rule_acc_neighbors_async 
            = get_rule_acc_neighbors_with_mngr(m_async);
        Utopia::apply_rule(rule_acc_neighbors_async, m_async.cells(), rng);
        assert(std::any_of(m_async.cells().begin(), m_async.cells().end(),
            [](const auto cell){ return cell->state() != 1; }
        ));

        //check that the order of the container was not altered
        std::vector<bool> inplace;
        std::transform(m_async.cells().begin(), m_async.cells().end(),
            ids.begin(), std::back_inserter(inplace),
            [](const auto cell, const auto id){ return cell->id() == id; }
        );
        assert(std::all_of(inplace.begin(), inplace.end(),
            [](const bool val){ return val; }
        ));

        //define a rule
        ids.clear();
        auto rule_register_ids
            = [&ids](const auto cell) {
            ids.push_back(cell->id());
            return cell->state();
        };

        // shuffle
        Utopia::apply_rule<true>(rule_register_ids, m_async.cells(), rng);
        const auto ids_copy = ids;
        // don't shuffle
        Utopia::apply_rule<false>(rule_register_ids, m_async.cells());

        //check that execution order of the container changed
        inplace.clear();
        std::transform(ids.begin(), ids.end(),
            ids_copy.begin(), std::back_inserter(inplace),
            [](const auto id1, const auto id2){ return id1 == id2; }
        );
        assert(std::any_of(inplace.begin(), inplace.end(),
            [](const bool val){ return !val; }
        ));

        // test that apply_rule works for agents as well
        auto agents = Utopia::Setup::create_agents_on_grid<
            int, Utopia::DefaultTag, std::size_t>(grid, 30);
        auto m_agents = Utopia::Setup::create_manager_agents<true, true>(
            grid, agents);

        // apply rule only to some agents
        decltype(agents) applicants;
        std::sample(agents.begin(), agents.end(),
            std::back_inserter(applicants), 10, rng);
        
        auto rule_state_increment
            = []([[maybe_unused]] const auto agent){
            return 42;
        };
        Utopia::apply_rule(rule_state_increment, applicants, rng);

        assert(std::count_if(m_agents.agents().begin(),
                m_agents.agents().end(),
                [](const auto agent){ return agent->state() == 42; })
            == 10
        );

        // Check that application of rule also works with temporary lambdas
        Utopia::apply_rule([](const auto& a) {return 42;},
                           m_sync.cells());
        Utopia::apply_rule([](const auto& a) { return 42; },
                           m_async.cells(), rng);
        Utopia::apply_rule<true>([](const auto& a) { return 42; },
                                 applicants, rng);
        Utopia::apply_rule<false>([](const auto& a) {return 42;}, applicants);

        std::cout << "Total success." << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occured: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
