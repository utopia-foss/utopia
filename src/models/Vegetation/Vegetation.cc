#include "Vegetation.hh"

using namespace Utopia::Models::Vegetation;

int main (int, char** argv)
{
    try {
        // Create PseudoParent and config file reference
        Utopia::PseudoParent pp(argv[1]);

        // Set the initial state, then create the model instance
        Vegetation model("Vegetation", pp);

        // Just run!
        model.run();

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Exception occurred!" << std::endl;
        return 1;
    }
}
