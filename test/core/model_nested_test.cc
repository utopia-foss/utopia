#define BOOST_TEST_MODULE nested model test

#include <boost/test/unit_test.hpp>

#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/cfg_utils.hh>
#include <utopia/core/logging.hh>

#include "testtools.hh"
#include <utopia/core/testtools/fixtures.hh>

#include "model_nested_test.hh"


// +++ Fixtures +++

/// A specialized infrastructure fixture, loading a configuration file
/** \note If no configuration file is required or available, you can
  *       simply omit the file path. The configuration is then empty.
  */
struct TestNestedModels : public Utopia::TestTools::BaseInfrastructure<> {
    Utopia::PseudoParent<> pp;
    
    Utopia::RootModel root;

    TestNestedModels ()
    :
        BaseInfrastructure<>("model_nested_test.yml"),
        pp("model_nested_test.yml"),
        root("root", pp)
    { }

    
    ~TestNestedModels () {
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());

        log->debug("Temporary files removed.");
    }
};

BOOST_FIXTURE_TEST_SUITE (test_nested_models, TestNestedModels)

BOOST_AUTO_TEST_CASE (test_iteration_order)
{
    /** Created model hierarchy:
     *
     *   0               Root (run for 10 steps)
     *                  /   \
     *                 /      ----------------- \
     *   1          One (iterated, until stop)   \
     *               |                         Another (iterated from start)
     *               |                        /               \
     *   2       DoNothing (iterated)      One (iterated)   DoNothing
     *                                      |               (run in prolog)
     *                                      |
     *   3                               DoNothing (iterated)
     */

    // Run model; should also iterate submodels
    log->debug("Performing run at topmost level {} ...", root.get_full_name());
    root.run();


    log->debug("Asserting prologs and epilogs executed ...");

    BOOST_TEST(root._prolog_run);
    BOOST_TEST(root._epilog_run);

    BOOST_TEST(root.sub_one._prolog_run);
    BOOST_TEST(root.sub_one._epilog_run);

    BOOST_TEST(root.sub_one.lazy._prolog_run);
    BOOST_TEST(root.sub_one.lazy._epilog_run);


    BOOST_TEST(root.sub_another._prolog_run);
    BOOST_TEST(root.sub_another._epilog_run);

    BOOST_TEST(root.sub_another.another_lazy._prolog_run);
    BOOST_TEST(root.sub_another.another_lazy._epilog_run);

    BOOST_TEST(root.sub_another.another_one._prolog_run);
    BOOST_TEST(root.sub_another.another_one._epilog_run);

    BOOST_TEST(root.sub_another.another_one.lazy._prolog_run);
    BOOST_TEST(root.sub_another.another_one.lazy._epilog_run);


    // Check that all models were iterated
    log->debug("Asserting correct iteration ...");

    BOOST_TEST(root.get_time() == 10); // time_max = 10

    BOOST_TEST(root.sub_one.get_time() == 3); // time stop = 3
    BOOST_TEST(root.sub_one.lazy.get_time() == 3);

    BOOST_TEST(root.sub_another.get_time() == 6); // time start = 5
    BOOST_TEST(root.sub_another.another_one.get_time() == 6);
    BOOST_TEST(root.sub_another.another_one.lazy.get_time() == 6);

    // the sub-model run during prolog with num_steps = 20
    BOOST_TEST(root.sub_another.another_lazy.get_time() == 20);


    // check log level propagation
    log->debug("Asserting correct log levels ...");
    BOOST_TEST(root.get_logger()->level() == spdlog::level::debug);
    BOOST_TEST(root.sub_another.get_logger()->level() == spdlog::level::debug);
    BOOST_TEST(root.sub_one.get_logger()->level() == spdlog::level::trace);
    BOOST_TEST(root.sub_one.lazy.get_logger()->level() == spdlog::level::trace);

    
    // check different random numbers are drawn from each submodel
    log->debug("Asserting correct random number generation ...");
    BOOST_TEST((*root.get_rng())() != (*root.sub_one.get_rng())());
    BOOST_TEST((*root.sub_one.get_rng())() != (*root.sub_another.get_rng())());
    BOOST_TEST((*root.sub_another.get_rng())() != (*root.sub_one.lazy.get_rng())());
    BOOST_TEST((*root.sub_one.lazy.get_rng())() != (*root.sub_another.another_one.lazy.get_rng())());

    // check RNG with same seed gives same value
    Utopia::DefaultRNG rng(Utopia::get_as<int>("seed", pp.get_cfg()));
    rng.discard(8);
    BOOST_TEST(rng() == (*root.get_rng())());
    

    // test that sub-models with undefined `num_steps` cannot be iterated
    auto& idle = root.sub_idle;
    BOOST_TEST(check_error_message<std::runtime_error>(
        "run sub-model without specifying `num_steps`",
        [&](){
            idle.run();
        },
        "Cannot perform run on (sub-)model",
        "   ", true)
    );

    log->info("Tests successful. :)");
}

BOOST_AUTO_TEST_SUITE_END()
