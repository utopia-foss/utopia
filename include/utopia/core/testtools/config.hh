#ifndef UTOPIA_CORE_TESTTOOLS_CONFIG_HH
#define UTOPIA_CORE_TESTTOOLS_CONFIG_HH

#include <string>
#include <functional>
#include <string_view>
#include <exception>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/core/demangle.hpp>

#include <yaml-cpp/yaml.h>

#include "exceptions.hh"
#include "utils.hh"


namespace Utopia::TestTools {

/**
 *  \addtogroup TestTools
 *  \{
 */

/// Repeatedly invokes a unary function that expects a Config node
/** The parameters with which the function is invoked are specified in a YAML
  * mapping, ``test_cases``. Each case also allows to specify whether the
  * callable will throw an exception; this makes use of the
  * Utopia::TestTools::check_exception method and supports most of the basic
  * exception types.
  *
  * Each of the test cases has the following form:
  *
  * \code{.yml}
  *     my_test_case:
  *       params: {}       # passed to callable
  *       throws: true     # (optional) If given, this should be the name of
  *                        # the expected exception thrown from the callable
  *       match: "foobar"  # (optional) If given, this string is expected to
  *                        # be found within the thrown exception's error
  *                        # message. It can be a substring.
  * \endcode
  *
  * Example YAML configuration for multiple test cases:
  *
  * \code{.yml}
  *     ---
  *     test_cases:
  *       case1:
  *         # The parameters that are passed to the callable
  *         params: {foo: bar, some_number: 42}
  *
  *       case1_but_failing:
  *         params: {foo: bar, some_number: -1}
  *         # With these parameters, the callable is expected to throw:
  *         throws: std::invalid_argument
  *
  *       case1_but_failing_with_match:
  *         params: {foo: bar, some_number: -1}
  *         throws: std::invalid_argument
  *         # Can optionally also define a string that is meant to be contained
  *         # in the full error message string.
  *         match: "Expected a positive number but got: -1"
  *
  *       # More test cases ...
  *       case2:
  *         params: {foo: spam, some_number: 23}
  *
  *       case2_KeyError:
  *         params: {some_number: 23}
  *         throws: Utopia::KeyError
  *
  *     # Other, unrelated config parameters here
  *     # ...
  * \endcode
  *
  * For the ``test_cases`` maping exemplified above, this can be invoked via:
  *
  * \code{.cc}
  *     test_config_callable(
  *         [](auto cfg){
  *             auto foo = get_as<std::string>("foo", cfg);
  *             auto some_number = get_as<int>("some_number", cfg);
  *
  *             some_function_that_throws_on_negative_int(some_number);
  *
  *             // Can do more tests here. Ideally, use BOOST_TEST( ... )
  *             // ...
  *         }, cfg["test_cases"], "My test cases", {__LINE__, __FILE__}
  *     );
  * \endcode
  *
  * \param  func        The callable to repeatedly invoke. This must be a
  *                     unary function that accepts a Config node.
  * \param  test_cases  A YAML::Node mapping that contains as keys the test
  *                     cases and as value another mapping. The ``params`` key
  *                     within each test case is passed to the callable.
  *                     The ``throws`` and ``match`` keys are optional and
  *                     control the exception handling.
  * \param  context_name A name that is added to the BOOST_TEST_CONTEXT which
  *                     is created for each test case. The context helps to
  *                     debug failing test cases, as it provides all parameters
  *                     that were passed to the callable.
  * \param  loc         Optional location information that can help tracing
  *                     back where this is invoked from.
  */
template<typename Callable=std::function<void(const DataIO::Config&)>>
void test_config_callable(Callable&& func,
                          const DataIO::Config& test_cases,
                          const std::string_view context_name = "",
                          const LocationInfo loc = {})
{
    // Check if callable can be invoked
    static_assert(std::is_invocable_v<Callable, DataIO::Config&>,
                  "Callable requires DataIO::Config as its only argument!");

    // Iterate over multiple configuration cases
    for (const auto& kv_pair : test_cases) {
        // Extract name of the case that is to be tested and enter context
        const auto case_name = kv_pair.first.template as<std::string>();
        const auto case_cfg = kv_pair.second;

        BOOST_TEST_CONTEXT(loc << context_name
                           << " -- Testing case '" << case_name
                           << "' ... with the following parameters:\n\n"
                           << case_cfg << "\n")
        {
        // Check if it this call is expected to throw; if so, check that it
        // throws the expected error message
        if (case_cfg["throws"]) {
            // Bind the config to a new callable (which accepts no arguments)
            const auto to_test = [func, case_cfg](){
                func(case_cfg["params"]);
            };

            const auto exc_type = get_as<std::string>("throws", case_cfg);
            const auto match = get_as<std::string>("match", case_cfg, "");

            // Check the exception type
            // ... have to define this manually for all types ...
            if (exc_type == "std::exception") {
                check_exception<std::exception>(to_test, match, loc);
            }
            else if (exc_type == "std::logic_error") {
                check_exception<std::logic_error>(to_test, match, loc);
            }
            else if (exc_type == "std::invalid_argument") {
                check_exception<std::invalid_argument>(to_test, match, loc);
            }
            else if (exc_type == "std::domain_error") {
                check_exception<std::domain_error>(to_test, match, loc);
            }
            else if (exc_type == "std::length_error") {
                check_exception<std::length_error>(to_test, match, loc);
            }
            else if (exc_type == "std::out_of_range") {
                check_exception<std::out_of_range>(to_test, match, loc);
            }
            else if (exc_type == "std::runtime_error") {
                check_exception<std::runtime_error>(to_test, match, loc);
            }
            else if (exc_type == "std::range_error") {
                check_exception<std::range_error>(to_test, match, loc);
            }
            else if (exc_type == "std::overflow_error") {
                check_exception<std::overflow_error>(to_test, match, loc);
            }
            else if (exc_type == "std::underflow_error") {
                check_exception<std::underflow_error>(to_test, match, loc);
            }
            else if (exc_type == "Utopia::KeyError") {
                check_exception<Utopia::KeyError>(to_test, match, loc);
            }
            else if (exc_type == "Utopia::Exception") {
                check_exception<Utopia::Exception>(to_test, match, loc);
            }
            else if (exc_type == "YAML::Exception") {
                check_exception<YAML::Exception>(to_test, match, loc);
            }
            else {
                BOOST_ERROR(
                    "Invalid exception type '" <<  exc_type << "' given in "
                    "`throws` argument! Supported exception types are: "
                    "std::exception, std::logic_error, std::invalid_argument, "
                    "std::domain_error, std::length_error, std::out_of_range, "
                    "std::runtime_error, std::range_error, "
                    "std::overflow_error, std::underflow_error, "
                    "Utopia::KeyError, Utopia::Exception, "
                    "and YAML::Exception."
                );
            }

            // No need to test anything else; continue in outer loop
            continue;
        }

        // NOT expected to throw. Invoke the test callable with its params...
        try {
            func(case_cfg["params"]);
        }
        catch (std::exception& e) {
            BOOST_ERROR(loc << "Unexpectedly threw an error ("
                        << boost::core::demangle(typeid(e).name())
                        << ") with message: " << e.what());
        }
        catch (...) {
            BOOST_ERROR(loc << "Unexpectedly threw a non-std error!");
        }

        } // End of test context
    }
}



// end group TestTools
/**
 *  \}
 */

} // namespace Utopia::TestTools

#endif // UTOPIA_CORE_TESTTOOLS_CONFIG_HH
