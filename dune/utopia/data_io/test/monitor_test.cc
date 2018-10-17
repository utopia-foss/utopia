#include <assert.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <string>
#include "../monitor.hh"
#include <yaml-cpp/yaml.h>

using namespace Utopia::DataIO;


/// Helper Function

/**
 * @brief Saves the buffer of an std::cout stream
 * 
 * Class taken from https://stackoverflow.com/questions/32079231/check-what-output-is-written-to-standard-output
 * 
 */
class Savebuf:
    public std::streambuf {
    
    /**
     * @brief The streambuffer
     * 
     */
    std::streambuf* sbuf;

    /**
     * @brief The saved stream buffer
     * 
     */
    std::string     save;

    int overflow(int c) {
         if (!traits_type::eq_int_type(c, traits_type::eof())) {
             save.push_back(traits_type::to_char_type(c));
             return sbuf->sputc(c);
         }
         else {
             return traits_type::not_eof(c);
         }
    }

    int sync() { return sbuf->pubsync(); }
public:

    /**
     * @brief Construct a new savebuf object
     * 
     * @param sbuf The stream buffer
     */
    Savebuf(std::streambuf* sbuf): sbuf(sbuf) {}

    /**
     * @brief Get the saved stream buffer as a std::string
     * 
     * @return std::string 
     */
    std::string str() const { return save; }
};


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


// Test the MonitorEntries functionality
void test_MonitorEntries(){
    // Create a MonitorEntries object, which holds an empty YAML::Node 
    MonitorEntries md;

    // Fill the node with a value and using a lambda function
    md.set_entry("testmodel1", "key", 42);
    md.set_entry("testmodel2", "key2", "new_value");
    
    // Check that the data is emited in the desired form
    // For this track the buffer of std::cout 
    std::streambuf* coutbuf = std::cout.rdbuf();
    Savebuf sbuf(coutbuf);
    std::cout.rdbuf(&sbuf);

    // Emit the data to the terminal
    md.emit();

    // Assert that the std::cout buffer has the same values as the data
    // NOTE: From the terminal output one has to cut off the last characters
    //       otherwise the comparison fails.
    std::string expected_output = "{testmodel1.key: 42, testmodel2.key2: new_value}";
    std::string terminal_output = sbuf.str().substr(0,expected_output.length());
    assert(terminal_output.compare(expected_output) == 0);

    // restore the original stream buffer
    std::cout.rdbuf(coutbuf); 
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

    m.set_by_value("an_int", 1);
    mm.set_by_func("a_double", [](){return 3.578;});
    mn.set_by_func("a_vector", [](){return std::vector<int>{1,2,3};});
    mn.set_by_func("an_array", [](){return std::array<float, 3>{{.1,.2,.3}};});
    mmm.set_by_func("a_string", [](){return "string";});

    // Check that the data is emited in the desired form
    // For this track the buffer of std::cout 
    std::streambuf* coutbuf = std::cout.rdbuf();
    Savebuf sbuf(coutbuf);
    std::cout.rdbuf(&sbuf);

    // After 10ms enough time has passed such that the needed_info 
    // should be written and the whole information should be emitted.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    m.set_by_func("hopefully_written", [](){return "needed_info";});
    m.set_by_func("hopefully_again_written", [](){return "additional_info";});
    m.set_by_func("an_int", [](){return 3;});
    rm.emit_if_enabled();

    // Not enough time has passed, so do not write this entry into the MonitorEntries
    // object
    m.set_by_func("hopefully_not_written!", [](){return "undesired_info";});

    // Nothing should be emitted because not enough time has passed
    rm.emit_if_enabled();

    // Assert that the std::cout buffer has the same values as the data
    std::string expected_output =   "{m.an_int: 1, "
                                    "m.mm.a_double: 3.578, "
                                    "m.mn.a_vector: [1, 2, 3], "
                                    "m.mn.an_array: [0.1, 0.2, 0.3], "
                                    "m.mm.mmm.a_string: string, "
                                    "m.hopefully_written: needed_info, "
                                    "m.hopefully_again_written: additional_info}\n";
    std::string terminal_output = sbuf.str();
    assert(terminal_output.compare(expected_output) == 0);

    // restore the original stream buffer
    std::cout.rdbuf(coutbuf); 
}

int main(){
    test_MonitorTimer();
    test_MonitorEntries();
    test_MonitorManager_and_Monitor();

    return 0;
}
