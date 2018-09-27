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
    MonitorTimer mt(2);
    
    const bool reset_timer = true;

    // It is not time to emit but ...
    assert(mt.time_has_come(reset_timer) == false);

    // ... if you wait for three milliseconds
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(3ms);
    
    // it is time to emit!
    assert(mt.time_has_come(reset_timer) == true);

    // Of course directly afterwards, there is again no time to emit.
    assert(mt.time_has_come(reset_timer) == false);
    assert(mt.time_has_come(reset_timer) == false);
}


// Test the MonitorData functionality
void test_MonitorData(){
    // Create a MonitorData object, which holds an empty YAML::Node 
    MonitorData md;

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


void test_RootMonitor_and_Monitor(){
    // Create a RootMonitor object
    RootMonitor rm("name", 2);

    // Create a Monitor object from a RootMonitor and other Monitors
    Monitor m("m", rm);
    Monitor mm("mm", m);
    Monitor mn("mn", m);
    Monitor mmm("mmm", mm);
    Monitor n("n", rm);

    m.set_entry("an_int", 1);
    mm.set_entry("a_double", 3.578);
    mn.set_entry("a_vector", std::vector<int>{1,2,3});
    mmm.set_entry("a_string", "string");

    // Not enough time has passed, so do not write this entry into the MonitorData
    // object
    m.set_entry_if_time_is_ripe("hopefully_not_written!", "undesired_info");

    // Check that the data is emited in the desired form
    // For this track the buffer of std::cout 
    std::streambuf* coutbuf = std::cout.rdbuf();
    Savebuf sbuf(coutbuf);
    std::cout.rdbuf(&sbuf);

    // Nothing should be emitted because not enough time has passed
    rm.perform_emission();

    // After 10ms enough time has passed such that the needed_info 
    // should be written and the whole information should be emitted.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    mmm.set_entry_if_time_is_ripe("hopefully_written", 2);

std::this_thread::sleep_for(10ms);
        rm.perform_emission();

    // Assert that the std::cout buffer has the same values as the data
    // NOTE: From the terminal output one has to cut off the last characters
    //       otherwise the comparison fails.
    std::string expected_output =   "{m.an_int: 1, "
                                    "m.mm.a_double: 3.578, "
                                    "m.mn.a_vector: [1, 2, 3], "
                                    "m.mm.mmm.a_string: string}";
    std::string terminal_output = sbuf.str().substr(0,expected_output.length());
    std::cout << terminal_output << std::endl;
    assert(terminal_output.compare(expected_output) == 0);

    // restore the original stream buffer
    std::cout.rdbuf(coutbuf); 

}

int main(){
    test_MonitorTimer();
    test_MonitorData();
    test_RootMonitor_and_Monitor();

    return 0;
}