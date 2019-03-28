#include <iostream>

#include "Geomorphology.hh"

using namespace Utopia::Models::Geomorphology;

int main(int, char *argv[])
{
    try {
        // Create PseudoParent and get a config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Geomorphology model("Geomorphology", pp);

        // Just run!
        model.run();

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
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
