#include <cassert>
#include <iostream>

#include "agent_manager_integration_test.hh"
#include "testtools.hh"

using namespace Utopia::Test;

int main() {
    try {

        std::cout << "Initializing pseudo parent ..." << std::endl;
        Utopia::PseudoParent pp("agent_manager_integration_test.yml");
        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing agent manager integration ... ------"
                  << std::endl;
        
        // Initialize the test model from the pseudo parent
        AMTest am_test("am_test", pp);

        std::cout << "Success." << std::endl << std::endl;
        

        // -------------------------------------------------------------------
        // Done.
        std::cout << "------ Total success. ------" << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
