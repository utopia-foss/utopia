#include <cassert>

#include "model_setup_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // create pseudo models that will be used as "parent" for the model
        // initializations. Do so with the two different possible constructors
        // only via config file:
        Utopia::PseudoParent pp1("model_setup_test.yml");
        
        // more granular:
        Utopia::PseudoParent pp2("model_setup_test.yml",         // config
                                 "model_setup_test_tmpfile2.h5", // output
                                 23,                             // seed
                                 "x");                           // access mode
        
        // custom RNG via class template deduction
        Utopia::PseudoParent<std::ranlux48_base> pp3             // RNG class
                                ("model_setup_test.yml",         // config
                                 "model_setup_test_tmpfile3.h5", // output
                                 42,                             // seed
                                 "x");                           // access mode
        // NOTE could also use the simple constructor here, but need to specify
        // a new temporary file for output writing

        // initialize the actual models using the different pseudo parents
        Utopia::DoNothingModel model1("model1", pp1);
        Utopia::DoNothingModel model2("model2", pp2);
        Utopia::DoNothingModel model3("model3", pp2);

        std::cout << "Initialization of models via pseudo parents succeeded."
                  << std::endl;

        // clean up temporary files
        pp1.hdffile.close();
        std::remove(pp1.hdffile.get_path().c_str());
        
        pp2.hdffile.close();
        std::remove(pp2.hdffile.get_path().c_str());
        
        pp3.hdffile.close();
        std::remove(pp3.hdffile.get_path().c_str());

        std::cout << "Temporary files removed." << std::endl;
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
