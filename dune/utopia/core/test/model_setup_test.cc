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
        std::cout << "Initialization succeeded." << std::endl;

        // clean up temporary file
        pp.hdffile.close();
        std::remove(pp.hdffile.get_path().c_str());
        std::cout << "Temporary file removed." << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }

    std::cout << "Test ran through." << std::endl;
    return 0;
}
