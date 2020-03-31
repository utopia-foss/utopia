#include <cassert>
#include <iostream>

#include "cell_manager_integration_test.hh"
#include "testtools.hh"

using namespace Utopia::Test;

int main() {
    Utopia::setup_loggers();
    try {
        std::cout << "Initializing pseudo parent ..." << std::endl;
        Utopia::PseudoParent pp("cell_manager_integration_test.yml");
        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing cell manager integration ... ------"
                  << std::endl;
        
        // Initialize the test model from the pseudo parent
        CMTest cm_test("cm_test", pp);

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
