#include <cassert>

#include <utopia/core/apply.hh>

#include "cell_manager_test.hh"

template<class CellManager>
decltype(auto) get_rule_acc_neighbors_with_mngr (CellManager& manager){
    auto rule = [&manager](const auto cell) {
        auto nb = manager.neighbors_of(cell);
        return std::accumulate(nb.begin(), nb.end(), 1,
            [](const auto val, const auto neighbor){
                return val + neighbor->state();
        });
    };
    return rule;
}

using CellTraitsSync = Utopia::CellTraits<int,
                                          Utopia::UpdateMode::sync,
                                          true>;
using CellTraitsAsync = Utopia::CellTraits<int,
                                           Utopia::UpdateMode::async,
                                           true>;

int main() {
    try {
        std::cout << "Getting config file ..." << std::endl;
        auto cfg = YAML::LoadFile("cell_manager_test.yml");
        std::cout << "Success." << std::endl << std::endl;

        std::cout << "Creating mock models" << std::endl;
        MockModel<CellTraitsSync> mm_sync("mm_sync", cfg["nb_vonNeumann"]);
        MockModel<CellTraitsAsync> mm_async("mm_async", cfg["nb_vonNeumann"]);
        std::cout << "Success." << std::endl << std::endl;

        // define a rule which yields different results for sync and async
        auto& cm_sync = mm_sync._cm;
        auto rule_acc_neighbors_sync= get_rule_acc_neighbors_with_mngr(cm_sync);
        auto& cm_async = mm_async._cm;
        auto& rng = *mm_async._rng;

        // apply the rule in a synchronous fashion
        Utopia::apply_rule(rule_acc_neighbors_sync, cm_sync.cells());
        assert(std::all_of(cm_sync.cells().begin(), cm_sync.cells().end(),
            [](const auto cell){ return cell->state() == 1; }
        ));

        // store order of cells
        std::vector<unsigned int> ids;
        std::transform(cm_async.cells().begin(), cm_async.cells().end(),
            std::back_inserter(ids),
            [](const auto cell){ return cell->id(); }
        );

        // apply the rule in a asynchronous fashion
        auto rule_acc_neighbors_async 
            = get_rule_acc_neighbors_with_mngr(cm_async);
        Utopia::apply_rule(rule_acc_neighbors_async, cm_async.cells(), rng);
        assert(std::any_of(cm_async.cells().begin(), cm_async.cells().end(),
            [](const auto cell){ return cell->state() != 1; }
        ));

        //check that the order of the container was not altered
        std::vector<bool> inplace;
        std::transform(cm_async.cells().begin(), cm_async.cells().end(),
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
        Utopia::apply_rule<true>(rule_register_ids, cm_async.cells(), rng);
        const auto ids_copy = ids;
        // don't shuffle
        Utopia::apply_rule<false>(rule_register_ids, cm_async.cells());

        //check that execution order of the container changed
        inplace.clear();
        std::transform(ids.begin(), ids.end(),
            ids_copy.begin(), std::back_inserter(inplace),
            [](const auto id1, const auto id2){ return id1 == id2; }
        );
        assert(std::any_of(inplace.begin(), inplace.end(),
            [](const bool val){ return !val; }
        ));

        // Check that application of rule also works with temporary lambdas
        Utopia::apply_rule([](const auto&) {return 42;},
                           cm_sync.cells());
        Utopia::apply_rule([](const auto&) { return 42; },
                           cm_async.cells(), rng);

        std::cout << "Total success." << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
