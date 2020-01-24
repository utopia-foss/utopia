#include <assert.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <string>
#include <limits>
#include <sstream>
#include <iomanip>

#include <yaml-cpp/yaml.h>

#include <utopia/core/logging.hh>
#include <utopia/data_io/monitor.hh>

#include "testtools.hh"

using namespace Utopia::DataIO;

// Tests ......................................................................
/// Test the MonitorTimer class
void test_MonitorTimer(){
    // Create a MonitorTimer that measures in milliseconds
    MonitorTimer mt(0.002);

    // It is not time to emit ...
    assert(mt.time_has_come() == true);
    
    // ... but not if you reset
    mt.reset();
    assert(mt.time_has_come() == false);

    // ... if you wait for three milliseconds
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(3ms);
    
    // it is time to emit!
    assert(mt.time_has_come() == true);
    mt.reset();

    // Of course directly afterwards, there is again no time to emit.
    assert(mt.time_has_come() == false);
    mt.reset();
    assert(mt.time_has_come() == false);
}


void test_MonitorManager_and_Monitor(){
    // Create a MonitorManager object
    MonitorManager rm(0.002);

    // Create a Monitor object from a MonitorManager and other Monitors
    Monitor m("m", std::make_shared<MonitorManager>(rm));
    Monitor mm("mm", m);
    Monitor mn("mn", m);
    Monitor mmm("mmm", mm);
    Monitor n("n", std::make_shared<MonitorManager>(rm));

    // Define floats
    const double a_double = 3.578;
    const std::array<float, 3> an_array{.1, .2, .3};

    // Set some values
    m.set_entry("an_int", 1);  // Is set to another value below!
    mm.set_entry("a_double", a_double);
    mn.set_entry("a_vector", [](){return std::vector<int>{1,2,3};});
    mn.set_entry("an_array", an_array);
    mmm.set_entry("a_string", [](){return "string";});

    // Check that the data is emited in the desired form
    // For this track the buffer of std::cout 
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
    std::string expected_output = "!!map "
                                  "{m.an_int: 3,"
                                  " m.mm.a_double: "
                                  #ifdef PRECISION_OUTPUT
                                    + double_sstr.str() + ","
                                  #else
                                    "3.578,"
                                  #endif
                                  " m.mn.a_vector: [1, 2, 3],"
                                  " m.mn.an_array: "
                                  #ifdef PRECISION_OUTPUT
                                    + float_sstr.str() + ","
                                  #else
                                    "[0.1, 0.2, 0.3],"
                                  #endif
                                  " m.mm.mmm.a_string: string,"
                                  " m.hopefully_written: needed_info,"
                                  " m.hopefully_again_written: additional_info"
                                  "}\n";
    const auto terminal_output = sbuf.str();
    assert(terminal_output.compare(expected_output) == 0);

    // restore the original stream buffer
    std::cout.rdbuf(coutbuf); 
}



// ............................................................................

int main() {
    try {
        // Setup
        Utopia::setup_loggers();

        // Run the tests
        test_MonitorTimer();
        test_MonitorManager_and_Monitor();

        std::cout << "Tests successful." << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
