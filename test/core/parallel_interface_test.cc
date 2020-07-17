#define BOOST_TEST_MODULE parallel interface test
#include <boost/test/included/unit_test.hpp> // for unit tests

#include <type_traits>

#include <utopia/core/parallel.hh>

#include "parallel_fixtures.hh"

// +++ Test Cases +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

/// Test methods and defaults of 'ParallelExecution'
BOOST_FIXTURE_TEST_CASE(parallel_execution_struct,
                        logger_setup)
{
    // Disabled by default
    BOOST_TEST(not Utopia::ParallelExecution::is_enabled());
    BOOST_TEST(not Utopia::ParallelExecution::is_applied());

    // Use default disable ("model_setup_test.yml" is missing the key)
    const Utopia::DataIO::Config cfg = YAML::LoadFile("parallel_stl_test.yml");

    // Default should disable the setting
    const Utopia::DataIO::Config cfg_default = cfg["default"];
    Utopia::ParallelExecution::init(cfg_default);
    BOOST_TEST(not Utopia::ParallelExecution::is_enabled());
    BOOST_TEST(not Utopia::ParallelExecution::is_applied());

    // Enable explicitly
    const Utopia::DataIO::Config cfg_works = cfg["works"];
    Utopia::ParallelExecution::init(cfg_works);
    BOOST_TEST(Utopia::ParallelExecution::is_enabled());
    BOOST_TEST(Utopia::ParallelExecution::is_applied());

    // Check error if node exists but required keys are missing
    const Utopia::DataIO::Config cfg_throws = cfg["throws"];
    BOOST_CHECK_THROW(
        Utopia::ParallelExecution::init(cfg_throws),
        Utopia::KeyError);
}

/// Test correct selection of STL execution policies in 'exec_parallel'
BOOST_FIXTURE_TEST_CASE(parallel_select,
                        parallel_setup)
{
    namespace stdexc = std::execution;

    auto is_seq = [](auto tpl) -> bool {
        return std::is_same_v<
            std::decay_t<std::tuple_element_t<0, decltype(tpl)>>,
            stdexc::sequenced_policy>;
    };
    auto is_unseq = [](auto tpl) -> bool {
        return std::is_same_v<
            std::decay_t<std::tuple_element_t<0, decltype(tpl)>>,
            stdexc::unsequenced_policy>;
    };
    auto is_par = [](auto tpl) -> bool {
        return std::is_same_v<
            std::decay_t<std::tuple_element_t<0, decltype(tpl)>>,
            stdexc::parallel_policy>;
    };
    auto is_par_unseq = [](auto tpl) -> bool {
        return std::is_same_v<
            std::decay_t<std::tuple_element_t<0, decltype(tpl)>>,
            stdexc::parallel_unsequenced_policy>;
    };

    using namespace Utopia;

    // Disable parallel features, all must be sequential
    Utopia::ParallelExecution::set(
        Utopia::ParallelExecution::Setting::disabled);
    BOOST_TEST(exec_parallel(ExecPolicy::seq, is_seq));
    BOOST_TEST(exec_parallel(ExecPolicy::unseq, is_seq));
    BOOST_TEST(exec_parallel(ExecPolicy::par, is_seq));
    BOOST_TEST(exec_parallel(ExecPolicy::par_unseq, is_seq));

    // Enable parallel features
    Utopia::ParallelExecution::set(Utopia::ParallelExecution::Setting::enabled);
    BOOST_TEST(exec_parallel(ExecPolicy::seq, is_seq));
    BOOST_TEST(exec_parallel(ExecPolicy::unseq, is_unseq));
    BOOST_TEST(exec_parallel(ExecPolicy::par, is_par));
    BOOST_TEST(exec_parallel(ExecPolicy::par_unseq, is_par_unseq));
}
