#include <iostream>

#include "PredatorPrey.hh"

using namespace Utopia::Models::PredatorPrey;

int main (int, char** argv)
{   
    try {
        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        PredatorPrey("PredatorPrey", pp).run();

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
