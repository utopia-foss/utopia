#define BOOST_TEST_MODULE apply rule test
#include <boost/test/unit_test.hpp>

#include <utopia/core/apply.hh>
#include <utopia/data_io/cfg_utils.hh>

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
                                          Utopia::Update::sync,
                                          true>;
using CellTraitsAsync = Utopia::CellTraits<int,
                                           Utopia::Update::async,
                                           true>;
using CellTraitsManual = Utopia::CellTraits<int,
                                            Utopia::Update::manual,
                                            true>;

using Config = Utopia::DataIO::Config;

struct ModelFixture
{
    Config cfg;
    MockModel<CellTraitsSync> mm_sync;
    MockModel<CellTraitsAsync> mm_async;
    MockModel<CellTraitsManual> mm_manual;

    ModelFixture () :
        cfg(YAML::LoadFile("cell_manager_test.yml")),
        mm_sync(build_model_sync(cfg)),
        mm_async(build_model_async(cfg)),
        mm_manual(build_model_manual(cfg))
    { }

private:
    MockModel<CellTraitsSync> build_model_sync (const Config cfg) const
    {
        return MockModel<CellTraitsSync>("mm_sync",
                                         cfg["nb_vonNeumann"]);
    }

    MockModel<CellTraitsAsync> build_model_async (const Config cfg) const
    {
        return MockModel<CellTraitsAsync>("mm_async",
                                          cfg["nb_vonNeumann"]);
    }

    MockModel<CellTraitsManual> build_model_manual (const Config cfg) const
    {
        return MockModel<CellTraitsManual>("mm_async",
                                          cfg["nb_vonNeumann"]);
    }
};

/// Return the IDs of the cells contained in a CellManager
template<class CellManager>
std::vector<int> collect_ids (const CellManager& cm)
{
    std::vector<int> ids(cm.cells().size());
    for (const auto& cell : cm.cells()) {
        ids.push_back(cell->id());
    }

    return ids;
}

BOOST_FIXTURE_TEST_CASE(sync_rule, ModelFixture)
{
    auto& cm = mm_sync._cm;
    auto rule = get_rule_acc_neighbors_with_mngr(cm);
    Utopia::apply_rule(rule, cm.cells());

    // check that rule was applied correctly
    const auto wrong = std::count_if(cm.cells().begin(),
                                     cm.cells().end(),
                                     [](const auto cell) {
                                        return cell->state() != 1;
                                     });
    BOOST_TEST(wrong == 0);
}

BOOST_FIXTURE_TEST_CASE(async_rule, ModelFixture)
{
    auto& cm = mm_async._cm;
    auto rule = get_rule_acc_neighbors_with_mngr(cm);
    auto& rng = *mm_async._rng;

    // store IDs
    const auto ids = collect_ids(cm);

    // apply the rule
    Utopia::apply_rule(rule, cm.cells(), rng);

    // check that rule had different outcome than for sync
    const auto not_one = std::count_if(cm.cells().begin(),
                                       cm.cells().end(),
                                       [](const auto cell) {
                                            return cell->state() != 1;
                                       });
    BOOST_TEST(not_one > 0);

    // check that order of container did not change
    const auto ids_now = collect_ids(cm);
    BOOST_TEST(ids == ids_now, boost::test_tools::per_element());
}

BOOST_FIXTURE_TEST_CASE(async_rule_shuffle, ModelFixture)
{
    auto& cm = mm_async._cm;
    auto& rng = *mm_async._rng;

    //define a rule
    auto ids = collect_ids(cm);
    ids.clear();
    auto rule_register_ids = [&ids](const auto cell) {
                                ids.push_back(cell->id());
                                return cell->state();
                             };

    // shuffle
    Utopia::apply_rule<true>(rule_register_ids, cm.cells(), rng);
    const auto ids_now = ids;
    ids.clear();

    // don't shuffle
    Utopia::apply_rule<false>(rule_register_ids, cm.cells());

    // check that execution order of the container changed
    // (there is a chance that some elements remain in place...)
    BOOST_TEST(ids != ids_now, boost::test_tools::per_element());
}

BOOST_FIXTURE_TEST_CASE(lambda_rule, ModelFixture)
{
    auto& cm_sync = mm_sync._cm;
    Utopia::apply_rule([](const auto&) {return 42;},
                        cm_sync.cells());
    // check that rule was applied correctly
    auto wrong = std::count_if(cm_sync.cells().begin(),
                               cm_sync.cells().end(),
                               [](const auto cell) {
                                    return cell->state() != 42;
                              });
    BOOST_TEST(wrong == 0);

    auto& cm_async = mm_async._cm;
    auto& rng = *mm_async._rng;
    Utopia::apply_rule([](const auto&) { return 42; },
                        cm_async.cells(), rng);
    // check that rule was applied correctly
    wrong = std::count_if(cm_async.cells().begin(),
                          cm_async.cells().end(),
                          [](const auto cell) {
                            return cell->state() != 42;
                         });
    BOOST_TEST(wrong == 0);
}

// TODO: Split this into multiple tests and make it more convenient with
//       using template test cases.
BOOST_FIXTURE_TEST_CASE(manual, ModelFixture)
{
    using Utopia::Update;
    using Utopia::Shuffle;

    auto& cm = mm_manual._cm;
    auto rule = [&cm](const auto& cell) {
                    auto nb = cm.neighbors_of(cell);
                    return std::accumulate(nb.begin(), nb.end(), 1,
                        [](const auto val, const auto neighbor){
                            return val + neighbor->state;
                    });
                };

    // apply sync
    Utopia::apply_rule<Update::sync>(rule, cm.cells());

    // check that rule was applied correctly
    const auto wrong = std::count_if(cm.cells().begin(),
                                     cm.cells().end(),
                                     [](const auto cell) {
                                        return cell->state != 1;
                                     });
    BOOST_TEST(wrong == 0);

    // apply async unshuffled
    auto& rng = *mm_manual._rng;
    auto ids = collect_ids(cm);
    Utopia::apply_rule<Update::async, Shuffle::off>(rule,
                                                        cm.cells(),
                                                        rng);

    // check that rule was applied correctly
    const auto not_one = std::count_if(cm.cells().begin(),
                                     cm.cells().end(),
                                     [](const auto cell) {
                                        return cell->state != 1;
                                     });
    BOOST_TEST(not_one > 0);

    auto ids_now = collect_ids(cm);
    BOOST_TEST(ids == ids_now, boost::test_tools::per_element());

    // apply async shuffled
    ids.clear();
    ids_now.clear();
    auto rule_register_ids = [&ids_now](const auto cell) {
                                ids_now.push_back(cell->id());
                                return cell->state;
                             };

    Utopia::apply_rule<Update::async, Shuffle::off>(rule_register_ids,
                                                        cm.cells(),
                                                        rng);
    ids = ids_now;
    ids_now.clear();

    Utopia::apply_rule<Update::async>(rule_register_ids, cm.cells(), rng);

    // check that execution order of the container changed
    // (there is a chance that some elements remain in place...)
    BOOST_TEST(ids != ids_now, boost::test_tools::per_element());
}
