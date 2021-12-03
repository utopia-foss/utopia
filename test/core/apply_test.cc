#define BOOST_TEST_MODULE apply rule test

#include <vector>
#include <string>
#include <numeric>

#include <boost/test/unit_test.hpp>

#include <utopia/core/apply.hh>
#include <utopia/core/logging.hh>
#include <utopia/data_io/cfg_utils.hh>

#include "cell_manager_test.hh"

// Use the CellManager namespace as it provides the MockModel
using namespace Utopia::Test::CellManager;


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

/// A stong-typed integer that throws an error if copied
struct IntErrorOnCopy
{
    int value = 0;

    IntErrorOnCopy () = default;

    IntErrorOnCopy (const IntErrorOnCopy&)
    {
        BOOST_FAIL("Tried to copy element that should not be copied!");
    }

    IntErrorOnCopy& operator=(const IntErrorOnCopy&)
    {
        BOOST_FAIL("Tried to copy element that should not be copied!");
        return *this;
    }
};

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

// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
struct ModelFixture
{
    Config cfg;
    MockModel<CellTraitsSync> mm_sync;
    MockModel<CellTraitsAsync> mm_async;
    MockModel<CellTraitsManual> mm_manual;
    std::vector<unsigned int> iota;

    ModelFixture () :
        cfg(YAML::LoadFile("cell_manager_test.yml")),
        mm_sync(build_model_sync(cfg)),
        mm_async(build_model_async(cfg)),
        mm_manual(build_model_manual(cfg)),
        iota(mm_manual._cm.cells().size(), 0)
    {
        // Fill vector
        std::iota(begin(iota), end(iota), 0);

        // NOTE: Only applies parallelism if enabled through build system
        Utopia::setup_loggers();
        Utopia::ParallelExecution::set(
            Utopia::ParallelExecution::Setting::enabled);
    }

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

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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

BOOST_FIXTURE_TEST_SUITE(automatic, ModelFixture)

BOOST_AUTO_TEST_CASE(sync_rule)
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

BOOST_AUTO_TEST_CASE(async_rule)
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

BOOST_AUTO_TEST_CASE(async_rule_shuffle)
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

BOOST_AUTO_TEST_CASE(lambda_rule)
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

    // And again for async
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(manual, ModelFixture)

using Utopia::Update;
using Utopia::Shuffle;

/// Get a rule accumulating states of an entity and its neighbors
template<class CellManager>
decltype(auto) get_rule_acc_neighbors (CellManager& manager){
    auto rule = [&manager](const auto cell) {
        auto nb = manager.neighbors_of(cell);
        return std::accumulate(nb.begin(), nb.end(), 1,
            [](const auto val, const auto neighbor){
                return val + neighbor->state;
        });
    };
    return rule;
}

BOOST_AUTO_TEST_CASE(sync)
{
    auto& cm = mm_manual._cm;
    auto acc_neighbors = get_rule_acc_neighbors(cm);

    // apply sync
    Utopia::apply_rule<Update::sync>(acc_neighbors, cm.cells());

    // check that rule was applied correctly
    const auto wrong = std::count_if(cm.cells().begin(),
                                     cm.cells().end(),
                                     [](const auto cell) {
                                        return cell->state != 1;
                                     });
    BOOST_TEST(wrong == 0);
}

BOOST_AUTO_TEST_CASE(async_unshuffled)
{
    auto& cm = mm_manual._cm;

    auto acc_neighbors = get_rule_acc_neighbors(cm);
    auto ids = collect_ids(cm);
    Utopia::apply_rule<Update::async, Shuffle::off>(acc_neighbors,
                                                    cm.cells());

    // check that rule was applied correctly
    const auto not_one = std::count_if(cm.cells().begin(),
                                       cm.cells().end(),
                                       [](const auto cell) {
                                        return cell->state != 1;
                                       });
    BOOST_TEST(not_one > 0);

    // Check that actual elements remained in order
    auto ids_now = collect_ids(cm);
    BOOST_TEST(ids == ids_now, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(async_shuffled)
{
    auto& cm = mm_manual._cm;
    auto& rng = *mm_manual._rng;

    auto ids = collect_ids(cm);
    auto ids_now = ids;
    ids_now.clear();

    auto rule_register_ids = [&ids_now](const auto cell) {
                                ids_now.push_back(cell->id());
                                return cell->state;
                             };

    // shuffle off
    Utopia::apply_rule<Update::async, Shuffle::off>(rule_register_ids,
                                                    cm.cells());
    ids = ids_now;
    ids_now.clear();

    // shuffle on implicitly
    Utopia::apply_rule<Update::async>(rule_register_ids, cm.cells(), rng);

    // check that execution order of the container changed
    // (there is a chance that some elements remain in place...)
    BOOST_TEST(ids != ids_now, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()

// Check for rules with multiple arguments
// NOTE: We already check if sync/shuffle works correctly so we just have to
//       ensure that the correct arguments are applied with each other.
BOOST_FIXTURE_TEST_SUITE(multiple, ModelFixture)

using Utopia::Update;
using Utopia::Shuffle;

// Dummy types for template test cases because "Shuffle::on" is no type!
struct Sync {};
struct AsyncShuffleOn {};
struct AsyncShuffleOff {};

using Cases = std::tuple<Sync, AsyncShuffleOn, AsyncShuffleOff>;

BOOST_AUTO_TEST_CASE_TEMPLATE(apply,
                              Setting,
                              Cases)
{
    auto& cm = mm_manual._cm;

    // apply the rule
    auto set_iota = [](auto, auto input) { return input; };
    const auto& iota_const = iota;
    if constexpr (std::is_same_v<Setting, Sync>) {
        Utopia::apply_rule<Update::sync>(set_iota, cm.cells(), iota_const);
    }
    else if constexpr (std::is_same_v<Setting, AsyncShuffleOn>) {
        auto& rng = *mm_manual._rng;
        Utopia::apply_rule<Update::async, Shuffle::on>(set_iota,
                                                       cm.cells(),
                                                       rng,
                                                       iota_const);
    }
    else {
        Utopia::apply_rule<Update::async, Shuffle::off>(set_iota,
                                                        cm.cells(),
                                                        iota_const);
    }

    std::vector<bool> correct(cm.cells().size(), false);
    std::transform(cm.cells().begin(),
                   cm.cells().end(),
                   iota.begin(),
                   correct.begin(),
                   [](auto&& cell, auto&& val) { return cell->id() == val; });

    BOOST_TEST(correct == std::vector<bool>(correct.size(), true));
}

/// Check with many (>2) arguments
BOOST_AUTO_TEST_CASE_TEMPLATE(many_args,
                              Setting,
                              Cases)
{
    using namespace std::string_literals;

    auto& cm = mm_manual._cm;

    // A rule with a bunch of arguments
    auto set_iota_4args = [](auto, auto, auto, auto input) { return input; };

    // The relevant argument; is passed to `input` and used for checking
    const auto& iota_const = iota;

    // Apply them, depending on the setting
    if constexpr (std::is_same_v<Setting, Sync>) {
        Utopia::apply_rule<Update::sync>(
            set_iota_4args, cm.cells(),
            iota_const, iota_const, iota_const
        );
    }
    else if constexpr (std::is_same_v<Setting, AsyncShuffleOn>) {
        auto& rng = *mm_manual._rng;
        Utopia::apply_rule<Update::async, Shuffle::on>(
            set_iota_4args, cm.cells(), rng,
            iota_const, iota_const, iota_const
        );
    }
    else {
        Utopia::apply_rule<Update::async, Shuffle::off>(
            set_iota_4args, cm.cells(),
            iota_const, iota_const, iota_const
        );
    }

    std::vector<bool> correct(cm.cells().size(), false);
    std::transform(cm.cells().begin(),
                   cm.cells().end(),
                   iota.begin(),
                   correct.begin(),
                   [](auto&& cell, auto&& val) { return cell->id() == val; });

    BOOST_TEST(correct == std::vector<bool>(correct.size(), true));
}

// Verify that a call to asynchronous, shuffling apply_rule makes no copies
BOOST_AUTO_TEST_CASE(verify_no_copy)
{
    auto& cm = mm_manual._cm;
    std::vector<IntErrorOnCopy> objects(cm.cells().size());

    auto set_value = [](auto, auto& object){ return object.value; };
    auto& rng = *mm_manual._rng;
    Utopia::apply_rule<Update::async, Shuffle::on>(set_value,
                                                   cm.cells(),
                                                   rng,
                                                   objects);
}

BOOST_AUTO_TEST_SUITE_END() // multiple arguments

/// Check that rules can also return void
// NOTE This does not check the shuffled update order, because it is already
//      asserted by the other tests.
BOOST_FIXTURE_TEST_CASE(void_rule, ModelFixture)
{
    using Utopia::Update;
    using Utopia::Shuffle;

    auto& rng = *mm_manual._rng;

    // Define void rules that alter the cell state
    auto rule_sync = [](const auto& cell) {
        cell->state_new() = cell->state() + 42;
    };
    auto rule_async = [](const auto& cell) {
        cell->state() += 42;
    };
    auto rule_manual = [](const auto& cell) {
        cell->state += 42;
    };

    // For the sync state update
    {
    auto& cm = mm_sync._cm;
    Utopia::apply_rule(rule_sync, cm.cells());

    const auto num_incorrect =
        std::count_if(cm.cells().begin(), cm.cells().end(),
            [](const auto& cell) { return cell->state() != 42; });
    BOOST_TEST(num_incorrect == 0);
    }

    // For the async state update -- without shuffle
    {
    auto& cm = mm_async._cm;
    Utopia::apply_rule<false>(rule_async, cm.cells());

    const auto num_incorrect =
        std::count_if(cm.cells().begin(), cm.cells().end(),
            [](const auto& cell) { return cell->state() != 42; });
    BOOST_TEST(num_incorrect == 0);
    }

    // For the async state update -- with shuffle
    {
    auto& cm = mm_async._cm;
    Utopia::apply_rule<true>(rule_async, cm.cells(), rng);

    const auto num_incorrect =
        std::count_if(cm.cells().begin(), cm.cells().end(),
            [](const auto& cell) { return cell->state() != 84; });
    BOOST_TEST(num_incorrect == 0);
    }

    // For the manually managed state updates -- without shuffle
    {
    auto& cm = mm_manual._cm;
    Utopia::apply_rule<Update::async, Shuffle::off>(rule_manual, cm.cells());

    const auto num_incorrect =
        std::count_if(cm.cells().begin(), cm.cells().end(),
            [](const auto& cell) { return cell->state != 42; });
    BOOST_TEST(num_incorrect == 0);
    }

    // For the manually managed state updates -- with shuffle
    {
    auto& cm = mm_manual._cm;
    Utopia::apply_rule<Update::async>(rule_manual, cm.cells(), rng);

    const auto num_incorrect =
        std::count_if(cm.cells().begin(), cm.cells().end(),
            [](const auto& cell) { return cell->state != 84; });
    BOOST_TEST(num_incorrect == 0);
    }
}
