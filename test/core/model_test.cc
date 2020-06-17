#define BOOST_TEST_MODULE model test

#include <thread>
#include <chrono>
#include <numeric>

#include <boost/test/included/unit_test.hpp>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "../data_io/testtools.hh"
#include "model_test.hh"


// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using namespace Utopia;
namespace tt = boost::test_tools;

struct Setup {
    /// The PseudoParent
    Utopia::PseudoParent<> pp;

    /// A logger instance to use within the tests
    const std::shared_ptr<spdlog::logger> log;

    /// Constructor, invoked before entering each test
    Setup()
    :
        pp("model_test.yml"),
        log(pp.get_logger())
    {
        log->info("PseudoParent and logger set up.");
    }

        /// Teardown of the fixture, invoked after each test
        /** Cleanup: Close and remove the created HDF5 file
         *  and the logger associated with the test model, ``root.test``.
         */
    ~Setup() {
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());
        log->info("Temporary files closed and removed.");

        // Remove the logger created by the model
        spdlog::drop("root.test");
    }
};

/// size of the state vector
const auto SIZE = 5u;

/// Type of the state
using state_vec = std::vector<double>;

/// A shared initial state vector
const auto initial_state = state_vec(SIZE, 0.0);


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Tests that all use the Setup fixture
BOOST_FIXTURE_TEST_SUITE (test_model_base_class, Setup)


/// Check model iteration and some basic properties
BOOST_AUTO_TEST_CASE (test_model_iterate) {
    // the test model
    TestModel model("test", pp, initial_state);

    // assert initial state
    BOOST_TEST(model.get_time() == 0);
    BOOST_TEST(model.state() == initial_state, tt::per_element());

    // assert state after first iteration
    model.iterate();
    BOOST_TEST(model.get_time() == 1);

    BOOST_TEST(model.state() == state_vec(SIZE, 1.0),
               tt::per_element());

    // set boundary conditions and check again
    // NOTE Henceforth, iteration leads to an increment of 2 instead of 1
    model.set_bc(state_vec(SIZE, 2.0));

    model.iterate();
    BOOST_TEST(model.get_time() == 2);

    BOOST_TEST(model.state() == state_vec(SIZE, 3.0),
               tt::per_element());

    // set state manually and assert it worked
    model.set_state(state_vec(SIZE, 1.0));
    BOOST_TEST(model.state() == state_vec(SIZE, 1.0),
               tt::per_element());

    // ... and perform a last iteration
    model.iterate();
    BOOST_TEST(model.get_time() == 3);

    BOOST_TEST(model.state() == state_vec(SIZE, 3.0),
               tt::per_element());
}


/// Check model iteration with a custom iterate method
/** \warning Overwriting the iterate method is NOT recommended. It should still
  *          be possible, that's why it's tested; but it should only be done if
  *          one absolutely knows what one is doing ...
  */
BOOST_AUTO_TEST_CASE (test_model_custom_iterate) {
    TestModelWithIterate model_it("test", pp, initial_state);

    BOOST_TEST(model_it.get_time() == 0);
    BOOST_TEST(model_it.state() == initial_state,
               tt::per_element());

    // Check override of iterate function in model_it, which iterates TWICE
    model_it.iterate();
    BOOST_TEST(model_it.get_time() == 1);
    BOOST_TEST(model_it.state() == state_vec(SIZE, 2.0),
               tt::per_element());

    model_it.iterate();
    BOOST_TEST(model_it.get_time() == 2);
    BOOST_TEST(model_it.state() == state_vec(SIZE, 4.0),
               tt::per_element());

    // Set boundary conditions, leading to an increment of (2 * 2.5) == 5
    model_it.set_bc(state_vec(SIZE, 2.5));

    model_it.iterate();
    BOOST_TEST(model_it.get_time() == 3);
    BOOST_TEST(model_it.state() == state_vec(SIZE, 9.0),
               tt::per_element());
}


/// Test the model's run method carries out the expected number of steps
BOOST_AUTO_TEST_CASE (test_model_run) {
    // Create the model
    auto model = TestModel("test", pp, initial_state);

    BOOST_TEST(model.get_time() == 0);

    // Get the number of steps and check it matches the internally accessible
    const auto cfg = YAML::LoadFile("model_test.yml");
    auto num_steps = get_as<unsigned int>("num_steps", cfg);
    BOOST_TEST(num_steps == model.get_time_max());

    // Test the datasets capacities are correct, matching the number of steps
    auto cap_state = model.get_dset_state()->get_capacity();
    auto cap_mean = model.get_dset_mean()->get_capacity();

    BOOST_TEST(cap_state.size() == 2);  // 2D
    BOOST_TEST(cap_mean.size() == 1);   // 1D

    // ... and that the size of the time dimension is correct, too.
    BOOST_TEST(cap_state[0] == num_steps + 1);
    BOOST_TEST(cap_mean[0] == num_steps + 1);

    // Before run is invoked, the dataset should be empty; let's check by
    // having a look at the current extent, which should not be set, because no
    // write operation took place yet
    auto ext_state = model.get_dset_state()->get_current_extent();
    auto ext_mean = model.get_dset_mean()->get_current_extent();

    BOOST_TEST(ext_state == std::vector<int>(), tt::per_element());
    BOOST_TEST(ext_mean == std::vector<int>(), tt::per_element());

    // Run the model
    model.run();
    BOOST_TEST(model.get_time() == num_steps);

    // After running, data should have been written, and the datasets' extent
    // should be set (with two or one entries) and match the expected shape.
    // This indirectly checks that writing took place. The correctness of the
    // written data is asserted in the corresponding Data I/O tests.
    ext_state = model.get_dset_state()->get_current_extent();
    ext_mean = model.get_dset_mean()->get_current_extent();

    BOOST_TEST(ext_state == std::vector<std::size_t>({num_steps + 1, SIZE}),
               tt::per_element());
    BOOST_TEST(ext_mean == std::vector<std::size_t>(1, num_steps + 1),
               tt::per_element());
    // NOTE For ext_mean, the expected value is {num_steps + 1}, a scalar, for
    //      which the compiler generates a warning for scalar brace
    //      construction; thus the (num_values, value) constructor is used.
}



/// Check frontend monitor during model iteration
BOOST_AUTO_TEST_CASE (test_model_monitor_emit) {
    // the test model
    TestModel model("test", pp, initial_state);

    // no monitor emit should have happened so far
    BOOST_TEST(model.get_time() == 0);
    BOOST_TEST(model.get_monitor_manager()->get_emit_counter() == 0);

    // make sure the monitor_emit_interval is set to the expected value
    BOOST_TEST(get_as<double>("monitor_emit_interval", pp.get_cfg()) == 1.5);

    // monitoring should happen after the first iteration (because it always
    // happens after the first iteration)
    model.iterate();
    BOOST_TEST(model.get_time() == 1);
    BOOST_TEST(model.get_monitor_manager()->get_emit_counter() == 1);

    // Iterate once more
    model.iterate();
    BOOST_TEST(model.get_time() == 2);

    // The second emit should _not_ have happened yet, because the previous
    // steps all occurred within the emit interval.
    BOOST_TEST(model.get_monitor_manager()->get_emit_counter() == 1);

    // Wait a while, such that the emit interval is surpassed
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1.5s);  // same value as above!

    // ... and perform a last iteration
    model.iterate();
    BOOST_TEST(model.get_time() == 3);
    BOOST_TEST(model.get_monitor_manager()->get_emit_counter() == 2);
}


/// Test whether passing of a custom configuration is possible
BOOST_AUTO_TEST_CASE (test_model_custom_config) {
    auto custom_cfg = pp.get_cfg()["custom_cfg"];
    TestModel model("some instance name without counterpart in the config",
                    pp, initial_state, custom_cfg);

    const auto model_cfg = model.get_cfg();
    BOOST_TEST(model_cfg.size() == 2);
    BOOST_TEST(get_as<std::string>("foo", model_cfg) == "bar");
    BOOST_TEST(get_as<std::string>("note", model_cfg)
               == "this is the custom configuration node");
}

BOOST_AUTO_TEST_SUITE_END() // end of test_model_base_class test suite
