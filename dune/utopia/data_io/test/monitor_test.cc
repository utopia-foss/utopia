#include "../monitor.hh"
#include <assert.h>
#include <thread>
#include <chrono>

using namespace Utopia::DataIO;

/// Test the MonitorTimer class
void test_MonitorTimer(){
    // Create a MonitorTimer that measures in milliseconds
    MonitorTimer<std::chrono::milliseconds> mt(2);
    
    // It is not time to emit but ...
    assert(mt.time_has_come() == false);

    // ... if you wait for three milliseconds
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(3ms);
    
    // it is time to emit!
    assert(mt.time_has_come() == true);

    // Of course directly afterwards, there is again no time to emit.
    assert(mt.time_has_come() == false);
    assert(mt.time_has_come() == false);

    // The MonitorTimer can also be constructed for other duration types
    MonitorTimer<std::chrono::seconds> mts(1);
}


int main(){

    test_MonitorTimer();
    return 0;
}