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


BOOST_AUTO_TEST_CASE(test_MonitorManager_and_Monitor) {
    // Create a MonitorManager object
    MonitorManager rm(0.002);

    // Create a Monitor object from a MonitorManager and other Monitors
    Monitor m("m", std::make_shared<MonitorManager>(rm));
    Monitor mm("m.mm", m);
    Monitor mn("m.mn", m);
    Monitor mmm("m.mm.mmm", mm);
    Monitor n("m.n", std::make_shared<MonitorManager>(rm));

    // Define floats
    const double a_double = 3.578;
    const std::array<float, 3> an_array{.1, .2, .3};

    // Set some values
    m.set_entry("an_int", 1);  // Is set to another value below!
    mm.set_entry("a_double", a_double);
    mn.set_entry("a_vector", [](){return std::vector<int>{1,2,3};});
    mn.set_entry("an_array", an_array);
    mmm.set_entry("a_string", [](){return "string";});

    // Check that the data is emitted in the desired form by tracking the
    // buffer of std::cout
    std::streambuf* coutbuf = std::cout.rdbuf();
    Savebuf sbuf(coutbuf);
    std::cout.rdbuf(&sbuf);

    // After 10ms enough time has passed such that the needed_info
    // should be written and the whole information should be emitted.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);

    m.set_entry("hopefully_written", [](){return "needed_info";});
    m.set_entry("hopefully_again_written", [](){return "additional_info";});

    // Overwrite a previously set value
    m.set_entry("an_int", [](){return 3;});

    // Emit should work now
    rm.emit_if_enabled();

    // Setting an entry should work, but it will not be emitted as not enough
    // time will have passed.
    m.set_entry("hopefully_not_written!", "undesired_info");
    rm.emit_if_enabled();

    // Prepare expected output for floats
    constexpr auto prec_float = std::numeric_limits<float>::max_digits10;
    constexpr auto prec_double = std::numeric_limits<double>::max_digits10;
    std::stringstream double_sstr;
    double_sstr << std::setprecision(prec_double) <<  a_double;
    std::stringstream float_sstr;
    float_sstr << std::setprecision(prec_float)
               << "[" << an_array[0] << ", "
               << an_array[1] << ", "
               << an_array[2] << "]";

    // Assert that the std::cout buffer only contains content from the first
    // emit operation
    std::string expected_output = (
    #ifdef PRECISION_OUTPUT
        "!!map {m: {an_int: 3, mm: {a_double: " + double_sstr.str() + "} "
        "mn: {a_vector: [1, 2, 3], an_array: " + float_sstr.str() + ","
        "m.mm.mmm.a_string: string, "
        "m.hopefully_written: needed_info, "
        "m.hopefully_again_written: additional_info}"
    #else
        "!!map {m: {an_int: 3, mm: {a_double: 3.578} "
        "mn: {a_vector: [1, 2, 3], an_array: [0.1, 0.2, 0.3],"
        "m.mm.mmm.a_string: string, "
        "m.hopefully_written: needed_info, "
        "m.hopefully_again_written: additional_info}"
    #endif
    );

    const auto terminal_output = sbuf.str();
    std::cout << terminal_output << std::endl;
    BOOST_TEST(terminal_output == expected_output);

    // restore the original stream buffer
    std::cout.rdbuf(coutbuf);
}
