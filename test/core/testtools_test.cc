#define BOOST_TEST_MODULE test testtools

#include <random>
#include <vector>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <string>
#include <string_view>
#include <exception>

#include <yaml-cpp/yaml.h>

#include <utopia/core/exceptions.hh>
#include <utopia/core/testtools.hh>

// Enter the Utopia::TestTools namespace
using namespace Utopia::TestTools;


// ++ Definitions +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Some exception types
using exc_types = std::tuple<std::logic_error,
                             std::invalid_argument,
                             std::domain_error,
                             std::length_error,
                             std::out_of_range,
                             std::runtime_error,
                             std::range_error,
                             std::overflow_error,
                             std::underflow_error,
                             Utopia::Exception
                             >;


// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// A specialized infrastructure fixture, loading a configuration file
struct Infrastructure : BaseInfrastructure<> {
    Infrastructure () : BaseInfrastructure<>("testtools_test.yml") {};
};



// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Test testtools/fixtures.hh ------------------------------------------------
BOOST_AUTO_TEST_SUITE (test_fixtures)

/// Test the BaseInfrastructure class
BOOST_FIXTURE_TEST_CASE (test_BaseInfrastructure, BaseInfrastructure<>)
{
    // Have all members available directly here: log, rng, cfg, ...
    log->info("Hello hello");
    BOOST_TEST(std::uniform_real_distribution<>(0., 0.)(*rng) == 0.);
    BOOST_TEST(cfg.size() == 0);

    // Can also invoke it with a path and make it load a config file
    auto infrastructure = BaseInfrastructure<>("testtools_test.yml");
    BOOST_TEST(infrastructure.cfg.size() > 0);
}

/// Test that derivation from the BaseInfrastructure class works as expected
BOOST_FIXTURE_TEST_CASE (test_BaseInfrastructure_derivation, Infrastructure)
{
    // BaseInfrastructure members still available
    log->info("Hello hello");

    BOOST_TEST(std::uniform_real_distribution<>(0., 0.)(*rng) == 0.);

    // Configuration loaded and accessible
    BOOST_TEST(cfg.size() > 0);

    auto some_cfg = get_as<Config>("infrastructure_test", cfg);
    BOOST_TEST(get_as<std::string>("some_string", some_cfg) == "foobar");
    BOOST_TEST(get_as<std::vector<int>>("some_list", some_cfg)
               == std::vector<int>({1,2,3}), tt::per_element());
    BOOST_TEST(get_as<int>("some_number", some_cfg) == 42);
}

BOOST_AUTO_TEST_SUITE_END ()


/// Test testtools/utils.hh ---------------------------------------------------
BOOST_AUTO_TEST_SUITE (test_utils)


/// Test the contains method
BOOST_AUTO_TEST_CASE (test_contains)
{
    const std::string s1 = "i am a foo bar string";
    const std::string_view s2 = "i am a BAR FOO string";
    const char s3[8] = "foo bar";

    BOOST_TEST(contains(s1, ""));
    BOOST_TEST(contains(s1, "foo bar"));
    BOOST_TEST(contains(s1, s3));
    BOOST_TEST(contains(s1, "i am a foo bar string"));

    BOOST_TEST(not contains(s1, "BAR FOO"));
    BOOST_TEST(not contains(s1, "some other string"));
    BOOST_TEST(not contains(s1, s2));
    BOOST_TEST(not contains(s2, s3));
}

/// Test the LocationInfo struct
BOOST_AUTO_TEST_CASE (test_LocationInfo)
{
    // Construct empty: should be empty
    const LocationInfo no_loc;
    BOOST_TEST(no_loc.line == 0);
    BOOST_TEST(no_loc.file_path.string() == "");
    BOOST_TEST(no_loc.string() == "");

    // Construct with line and file information
    auto loc = LocationInfo(42, __FILE__);
    BOOST_TEST(loc.line == 42);
    BOOST_TEST(loc.file_path.string() == __FILE__);

    // Copy and manipulate
    auto loc_2 = loc;
    loc_2.line = 24;
    loc_2.file_path = "none";
    BOOST_TEST(loc_2.line == 24);
    BOOST_TEST(loc_2.file_path.string() == "none");

    // Can format it to a string using fmt library
    auto loc_str = loc.string();
    BOOST_TEST(loc_str.size() > 0);
    loc.fstr = "{file_name:}, line {line:d}";
    BOOST_TEST(loc.string() == "testtools_test.cc, line 42");

    // Can ostream both
    std::cout << loc << std::endl;
    std::cout << no_loc << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()


/// Test testtools/exceptions.hh ----------------------------------------------

BOOST_AUTO_TEST_SUITE (test_exceptions)

/// Test the "matching" branch of the check_exception function
BOOST_AUTO_TEST_CASE_TEMPLATE (test_check_exception, ExcT, exc_types)
{
    std::string msg = "foo bar 12345 baz";
    check_exception<ExcT>(
        [&](){
            throw ExcT(msg);
        },
        "ar 123", {__LINE__, __FILE__}
    );

    // match string and line information is optional
    check_exception<ExcT>([&](){ throw ExcT(msg); }, "", {__LINE__, __FILE__});
    check_exception<ExcT>([&](){ throw ExcT(msg); });
}

/// Test the cases where the check_exception function invokes BOOST_ERROR
BOOST_AUTO_TEST_CASE (test_check_exception_errors,
                      *utf::expected_failures(5))
{
    using ExcT = std::invalid_argument;
    using BadExcT = std::runtime_error;

    // Failure 1: not matching the error message
    std::string msg = "this is the error message";
    check_exception<ExcT>([&](){ throw ExcT(msg); }, "i do not match");

    // Failure 2: not matching the type
    check_exception<ExcT>([&](){ throw BadExcT(msg); }, "the error message");

    // Failure 3: non-std::exception
    check_exception<ExcT>([&](){ throw 1; }, "the error message");

    // Failure 4: should have thrown but did not
    check_exception<ExcT>([](){ return; });

    // Failure 5: umbrella exception message check fails
    check_exception<std::exception>([&](){ throw ExcT(msg); }, "do not match");
}

BOOST_AUTO_TEST_SUITE_END()


/// Test testtools/config.hh --------------------------------------------------

BOOST_FIXTURE_TEST_SUITE (test_config, Infrastructure)

/// Test the succeeding cases for the test_config_callable function
BOOST_AUTO_TEST_CASE (test_test_config_callable)
{
    // Define the test callable
    const auto callable =
        [](auto cfg){
            auto foo = get_as<std::string>("foo", cfg);
            auto num = get_as<int>("num", cfg);

            if (num < 0) {
                throw std::invalid_argument("Expected non-negative number but "
                                            "got: " + std::to_string(num));
            }

            if (foo == "some very bad value") {
                throw std::runtime_error("Something really bad happened!");
            }
       };

    // And invoke it, with and without optional arguments
    test_config_callable(
        callable,
        cfg["config_based_tests"]["succeeding"],
        "Succeeding test cases",
        {__LINE__, __FILE__}
    );
    test_config_callable(
        callable,
        cfg["config_based_tests"]["succeeding"]
    );
}

/// Test some failing cases; there are three cases defined in the config
BOOST_AUTO_TEST_CASE (test_test_config_callable_failing,
                      *utf::expected_failures(6))
{
    test_config_callable(
        [](auto){
            BOOST_TEST(false);
            BOOST_TEST(2 * false);
        },
        cfg["config_based_tests"]["no_params"],
        "three test cases that each fail twice"
    );
}

/// Test some succeeding cases for certain error messages
BOOST_AUTO_TEST_CASE (test_test_config_callable_exceptions,
                      *utf::expected_failures(3))
{
    test_config_callable(
        [](auto cfg){
            const auto exc_typename = get_as<std::string>("exc_typename", cfg);

            if (exc_typename == "none") {
                return;  // Expected failure 1
            }
            else if (exc_typename == "std::exception") {
                throw std::exception();
            }
            else if (exc_typename == "std::logic_error") {
                throw std::logic_error("foo");
            }
            else if (exc_typename == "std::invalid_argument") {
                throw std::invalid_argument("foo");
            }
            else if (exc_typename == "std::domain_error") {
                throw std::domain_error("foo");
            }
            else if (exc_typename == "std::length_error") {
                throw std::length_error("foo");
            }
            else if (exc_typename == "std::out_of_range") {
                throw std::out_of_range("foo");
            }
            else if (exc_typename == "std::runtime_error") {
                throw std::runtime_error("foo");
            }
            else if (exc_typename == "std::range_error") {
                throw std::range_error("foo");
            }
            else if (exc_typename == "std::overflow_error") {
                throw std::overflow_error("foo");
            }
            else if (exc_typename == "std::underflow_error") {
                throw std::underflow_error("foo");
            }
            else if (exc_typename == "Utopia::KeyError") {
                get_as<int>("foo", {});
            }
            else if (exc_typename == "Utopia::Exception") {
                throw Utopia::Exception("foo");
            }
            else if (exc_typename == "YAML::Exception") {
                cfg["exc_typename"].template as<int>();
            }
            else {
                throw 1;  // Expected failure 2
            }
        },
        cfg["config_based_tests"]["expected_exceptions"],
        // config contains expected failure 3
        "Expected exceptions"
    );
}

BOOST_AUTO_TEST_CASE (test_test_config_callable_failing_due_to_bad_exception,
                      *utf::expected_failures(6))
{
    test_config_callable(
        [](auto){
            throw std::invalid_argument("some irrelevant error message");
        },
        cfg["config_based_tests"]["no_params"],
        "three test cases with unexpected std::exception"
    );

    test_config_callable(
        [](auto){
            throw 1;
        },
        cfg["config_based_tests"]["no_params"],
        "three test cases with unexpected non-std::exception"
    );
}

BOOST_AUTO_TEST_SUITE_END()
