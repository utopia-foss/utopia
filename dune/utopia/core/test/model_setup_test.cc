#include <cassert>

#include "model_setup_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // create pseudo model that will be used as "parent" for the model here
        Utopia::PseudoParent pp("model_setup_test.yml");

        // initialize the actual model
        Utopia::DoNothingModel model("my_model", pp);
        
        std::cout << "Initialization complete." << std::endl;
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
