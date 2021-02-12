#define BOOST_TEST_MODULE test monitor

#include <thread>
#include <chrono>
#include <numeric>
#include <string>
#include <limits>
#include <sstream>
#include <iomanip>

#include <yaml-cpp/yaml.h>
#include <boost/test/unit_test.hpp>

#include <utopia/data_io/monitor.hh>
#include <utopia/core/logging.hh>
#include <utopia/data_io/cfg_utils.hh>
#include <utopia/core/testtools.hh>

#include "testtools.hh"

using namespace Utopia;
using namespace Utopia::DataIO;
using namespace Utopia::TestTools;


// -- Fixture -----------------------------------------------------------------


/// The specialized infrastructure fixture, allowing to replace the cout buffer
struct Infrastructure : public BaseInfrastructure<> {
    /// The regular standard output buffer
    std::streambuf* regular_coutbuf;

    Infrastructure () : BaseInfrastructure<>() {
        // Always store the originally used output buffer
        regular_coutbuf = std::cout.rdbuf();

        // Test it's working
        auto sbuf = replace_cout();
        std::cout << "stream buffer test 1 2 1 2" << std::endl;
        BOOST_TEST(sbuf.str() == "stream buffer test 1 2 1 2\n");
        reinstate_cout();
    }

    /// Replaces cout stream with a custom stream-saving buffer
    Savebuf replace_cout () const {
        auto sbuf = Savebuf(regular_coutbuf);
        std::cout.rdbuf(&sbuf);
        return sbuf;
    }

    /// Reinstates the originally used cout buffer
    void reinstate_cout () const {
        std::cout.rdbuf(regular_coutbuf);
    }

    /// Upon destruction, take care of resetting the stream buffer
    ~Infrastructure () {
        reinstate_cout();
    }
};


// -- Tests -------------------------------------------------------------------

/// Test the MonitorTimer class
BOOST_AUTO_TEST_CASE(test_MonitorTimer) {
    // Create a MonitorTimer that measures in milliseconds
    MonitorTimer mt(0.002);

    // It is not time to emit ...
    BOOST_TEST(mt.time_has_come());

    // ... but not if you reset
    mt.reset();
    BOOST_TEST(not mt.time_has_come());

    // ... if you wait for three milliseconds
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(3ms);

    // it is time to emit!
    BOOST_TEST(mt.time_has_come());
    mt.reset();

    // Of course directly afterwards, there is again no time to emit.
    BOOST_TEST(not mt.time_has_come());
    mt.reset();
    BOOST_TEST(not mt.time_has_come());
}


/// Test a simple monitoring setup: single manager and single monitor
BOOST_FIXTURE_TEST_CASE(test_monitoring, Infrastructure) {
    using namespace std::chrono_literals;

    auto rm = std::make_shared<MonitorManager>(0.002);
    Monitor m("m", rm);

    m.set_entry("some_int", 1);
    m.set_entry("some_array", std::vector<int>({1, 2, 3}));

    // Keep track of output to cout, then trigger first emit (should fire)
    {
        auto sbuf = replace_cout();
        rm->emit_if_enabled();
        BOOST_TEST(sbuf.str() ==
            "!!map {m: {some_int: 1, some_array: [1, 2, 3]}}\n"
        );
        reinstate_cout();
    }

    // Set some more entries
    m.set_entry("some_array", std::vector<int>({3, 4}));
    m.set_entry("some_string", "foo");

    // Subsequent emit should not fire (too soon)
    rm->check_timer();
    {
        auto sbuf = replace_cout();
        rm->emit_if_enabled();
        BOOST_TEST(sbuf.str() == "");
        reinstate_cout();
    }

    // ... but should fire after a sufficiently large delay
    std::this_thread::sleep_for(20ms);
    rm->check_timer();
    {
        auto sbuf = replace_cout();
        rm->emit_if_enabled();
        BOOST_TEST(sbuf.str() ==
            "!!map {m: {some_int: 1, some_array: [3, 4], some_string: foo}}\n"
        );
        reinstate_cout();
    }
}


/// Test monitoring when having nested monitors
BOOST_FIXTURE_TEST_CASE(test_monitoring_nested, Infrastructure) {
    using namespace std::chrono_literals;

    // Create monitor hierarchy
    auto rm = std::make_shared<MonitorManager>(0.002);
    Monitor m("m", rm);
    Monitor mm("mm", m);
    Monitor mn("mn", m);
    Monitor mmm("mmm", mm);
    Monitor n("n", rm);

    // Define floats
    const double a_double = 3.578;
    const std::array<float, 3> an_array{.1, .2, .3};

    // Set some values
    m.set_entry("an_int", 1);  // Is set to another value below!
    mm.set_entry("a_double", a_double);
    mn.set_entry("a_vector", [](){return std::vector<int>{1,2,3};});
    mn.set_entry("an_array", an_array);
    mmm.set_entry("a_string", [](){return "string";});


    // Overwrite a previously set value
    m.set_entry("an_int", [](){return 3;});

    // Now emit. Because it is the first one, this should always fire.
    auto sbuf = replace_cout();
    rm->emit_if_enabled();

    // Prepare expected output for floats, which may differ depending on the
    // supported precision of the yaml-cpp library
    constexpr auto prec_float = std::numeric_limits<float>::max_digits10;
    constexpr auto prec_double = std::numeric_limits<double>::max_digits10;

    std::stringstream double_sstr;
    double_sstr << std::setprecision(prec_double) <<  a_double;

    std::stringstream arr_sstr;
    arr_sstr << std::setprecision(prec_float)
               << "[" << an_array[0] << ", "
               << an_array[1] << ", "
               << an_array[2] << "]";

    // Now check output
    std::string expected_output = (
    #ifdef PRECISION_OUTPUT
        "!!map {m: {an_int: 3, mm: {a_double: " + double_sstr.str() + ", "
        "mmm: {a_string: string}}, mn: {a_vector: [1, 2, 3], "
        "an_array: " + arr_sstr.str() + "}}}\n"
    #else
        "!!map {m: {an_int: 3, mm: {a_double: 3.578, "
        "mmm: {a_string: string}}, mn: {a_vector: [1, 2, 3], "
        "an_array: [0.1, 0.2, 0.3]}}}\n"
    #endif
    );

    const auto output = sbuf.str();
    BOOST_TEST(output == expected_output);
}
