#define BOOST_TEST_MODULE parallel stl test
#include <boost/test/data/test_case.hpp>
#include <boost/test/included/unit_test.hpp> // for unit tests

#include <algorithm>
#include <type_traits>
#include <vector>

#include <utopia/core/logging.hh>
#include <utopia/core/parallel.hh>

// +++ Fixtures +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

struct logger_setup
{
    logger_setup() { Utopia::setup_loggers(); }
};

struct parallel_setup : public logger_setup
{
    parallel_setup()
    {
        Utopia::ParallelExecution::set(
            Utopia::ParallelExecution::Setting::enabled);
    }
    ~parallel_setup()
    {
        Utopia::ParallelExecution::set(
            Utopia::ParallelExecution::Setting::disabled);
    }
};

struct vectors : public parallel_setup
{
    std::vector<double> from = std::vector<double>(1E6, 0.0);
    std::vector<double> to = std::vector<double>(1E6, 1.0);
};

// +++ Test Cases +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

/// Test methods and defaults of 'ParallelExecution'
BOOST_FIXTURE_TEST_CASE(parallel_execution_struct, logger_setup)
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
BOOST_FIXTURE_TEST_CASE(parallel_select, parallel_setup)
{
    namespace stdexc = std::execution;

    auto is_seq = [](auto stl_policy) -> bool {
        return std::is_same_v<decltype(stl_policy), stdexc::sequenced_policy>;
    };
    auto is_unseq = [](auto stl_policy) -> bool {
        return std::is_same_v<decltype(stl_policy), stdexc::unsequenced_policy>;
    };
    auto is_par = [](auto stl_policy) -> bool {
        return std::is_same_v<decltype(stl_policy), stdexc::parallel_policy>;
    };
    auto is_par_unseq = [](auto stl_policy) -> bool {
        return std::is_same_v<decltype(stl_policy),
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

// --- Test STL Algorithm Overloads for all Execution Policies --- //

/// Test copy
BOOST_DATA_TEST_CASE_F(
    vectors,
    copy,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::copy(policy, begin(from), end(from), begin(to));
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test for_each
BOOST_DATA_TEST_CASE_F(
    vectors,
    for_each,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::for_each(policy, begin(from), end(from), [](auto& val) { val = 1.0; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test unary transform
BOOST_DATA_TEST_CASE_F(
    vectors,
    transform_1,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::transform(
      policy, begin(from), end(from), begin(from), [](auto&&) { return 1.0; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test binary transform
BOOST_DATA_TEST_CASE_F(
    vectors,
    transform_2,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::transform(policy,
                   begin(from),
                   end(from),
                   begin(to),
                   begin(from),
                   [](auto&& lhs, auto&& rhs) { return lhs + rhs; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}
