#include <cassert>

#include "model_setup_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // create pseudo models that will be used as "parent" for the model
        // initializations. Do so with the two different possible constructors
        std::cout << "Initializing pseudo parents..." << std::endl;
        
        // only via config file:
        Utopia::PseudoParent pp1("model_setup_test.yml");
        
        // more granular:
        Utopia::PseudoParent pp2("model_setup_test.yml",         // config
                                 "model_setup_test_tmpfile2.h5", // output
                                 23,                             // seed
                                 "w");                           // access mode
        
        // custom RNG via class template deduction
        Utopia::PseudoParent<std::ranlux48_base>                 // RNG class
                             pp3("model_setup_test.yml",         // config
                                 "model_setup_test_tmpfile3.h5", // output
                                 42,                             // seed
                                 "w");                           // access mode
        // NOTE could also use the simple constructor here, but need to specify
        // a new temporary file for output writing
        std::cout << "Initialization of pseudo parents succeeded."
                  << std::endl << std::endl;

        // initialize the actual models using the different pseudo parents
        std::cout << "Initializing models via pseudo parents ..." << std::endl;
        Utopia::DoNothingModel model1("model1", pp1);
        Utopia::DoNothingModel model2("model2", pp2);
        Utopia::DoNothingModel model3("model3", pp2);

        std::cout << "Initialization of models via pseudo parents succeeded."
                  << std::endl << std::endl;


        // Perform some simple checks
        std::cout << "Performing tests ..." << std::endl;

        // Is the config read in correctly?
        assert(model1.get_cfg()["foo"].as<std::string>() == "bar");
        assert(model2.get_cfg()["bar"].as<std::string>() == "foo");
        assert(model3.get_cfg()["spam"].as<std::string>() == "eggs");

        std::cout << "Tests finished." << std::endl << std::endl;


        // clean up temporary files
        std::cout << "Removing temporary files ..." << std::endl;

        auto pp1_file = pp1.get_hdffile();
        pp1_file->close();
        std::remove(pp1_file->get_path().c_str());

        auto pp2_file = pp2.get_hdffile();
        pp2_file->close();
        std::remove(pp2_file->get_path().c_str());

        auto pp3_file = pp3.get_hdffile();
        pp3_file->close();
        std::remove(pp3_file->get_path().c_str());

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
