#include <cassert>

#include "model_setup_test.hh"

using namespace Utopia;

int main()
{
    Utopia::setup_loggers();
    try {
        // create pseudo models that will be used as "parent" for the model
        // initializations. Do so with the two different possible constructors
        std::cout << "Initializing pseudo parents..." << std::endl;
        
        // only via config file:
        PseudoParent pp1("model_setup_test.yml");
        
        // more granular:
        PseudoParent pp2("model_setup_test.yml",         // config
                         "model_setup_test_tmpfile2.h5", // output
                         23,                             // seed
                         "w",                            // access mode
                         1.);                            // emit interval
        
        // custom RNG via class template deduction
        PseudoParent<std::ranlux48_base>                 // RNG class
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
        DoNothingModel model1("model1", pp1);
        DoNothingModel model2("model2", pp2);
        DoNothingModel model3("model3", pp2);

        std::cout << "Initialization of models via pseudo parents succeeded."
                  << std::endl << std::endl;


        // Perform some simple checks
        std::cout << "Performing tests ..." << std::endl;

        // Is the config read in correctly?
        assert(get_as<std::string>("foo", model1.get_cfg()) == "bar");
        assert(get_as<std::string>("bar", model2.get_cfg()) == "foo");
        assert(get_as<std::string>("spam", model3.get_cfg()) == "eggs");

        // Is the write_every parameter passed along correctly?
        assert(pp1.get_write_every() == 3);
        assert(pp2.get_write_every() == 3);
        assert(pp3.get_write_every() == 3);
        assert(model1.get_write_every() == 3);
        assert(model2.get_write_every() == 1); // set manually
        assert(model3.get_write_every() == 3); // set manually
        // NOTE Write output is asserted on Python side

        // Is the monitor emit interval set correctly?
        assert(pp1.get_monitor_manager()->get_emit_interval().count() == 5.);
        assert(pp2.get_monitor_manager()->get_emit_interval().count() == 1.);
        assert(pp3.get_monitor_manager()->get_emit_interval().count() == 5.);

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
