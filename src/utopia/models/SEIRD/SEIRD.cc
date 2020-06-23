#include <iostream>

#include "SEIRD.hh"

using namespace Utopia::Models::SEIRD;


int main (int, char** argv)
{
    try {

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance and directly run it
        SEIRD("SEIRD", pp).run();

        // Done.
        return 0;
    }
    catch (Utopia::Exception& e) {
        return Utopia::handle_exception(e);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occurred!" << std::endl;
        return 1;
    }
}
