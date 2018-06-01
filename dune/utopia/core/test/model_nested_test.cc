#include <cassert>

#include <dune/utopia/data_io/hdffile.hh>

#include "model_nested_test.hh"

/// clean up method that is performed after the tests
void cleanup(Utopia::DataIO::HDFFile& tmpfile, std::string tmpfile_path) {
    std::cout << "Cleaning up ..." << std::endl;

    // close and remove the temporary file
    tmpfile.close();
    std::cout << "  tmpfile closed" << std::endl;

    std::remove(tmpfile_path.c_str());
    std::cout << "  tmpfile removed" << std::endl;

    std::cout << "Cleanup finished." << std::endl;
}


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // -- Setup model -- //
        // get the test config file (path is relative to executable!)
        std::cout << "Loading config ..." << std::endl;
        Utopia::DataIO::Config cfg("model_nested_test.yml");
        std::cout << "  Loaded." << std::endl;

        // create a temporary file and get the basegroup
        std::cout << "Creating temporary output file ..." << std::endl;
        auto tmpfile_path = cfg["output_path"].as<std::string>();
        std::cout << "  output_path: " << tmpfile_path << std::endl;

        auto tmpfile = Utopia::DataIO::HDFFile(tmpfile_path, "w");
        std::cout << "  file created" << std::endl;
        auto basegroup = tmpfile.get_basegroup();
        std::cout << "  basegroup created" << std::endl;

        // initialize an RNG
        std::cout << "Creating shared RNG ..." << std::endl;
        auto seed = cfg["seed"].as<int>();
        std::cout << "  seed: " << seed << std::endl;

        auto rng = std::make_shared<std::mt19937>(seed);
        std::cout << "  RNG created" << std::endl;

        // create the model instances
        std::cout << "Initializing RootModel instance ..." << std::endl;
        
        // the test model
        Utopia::RootModel root("root", cfg, basegroup, rng);

        std::cout << "RootModel 'root' initialized." << std::endl;

        /** Created model hierarchy:
         *
         *   0               Root
         *                  /    \
         *                 /      \
         *   1          One        Another
         *               |           |    \
         *   2       DoNothing      One   DoNothing
         *                           |
         *   3                   DoNothing
         */

        // -- Tests begin here -- //
        std::cout << "Commencing tests ..." << std::endl;

        // Iterate model; should also iterate submodels
        std::cout << "  Performing single iteration ..." << std::endl;
        root.iterate();

        // Check that all models were iterated
        std::cout << "  Asserting correct iteration ..." << std::endl;
        // level 0
        assert(root.get_time() == 1);
        // level 1
        assert(root.sub_one.get_time() == 1);
        assert(root.sub_another.get_time() == 1);
        // level 2
        assert(root.sub_one.lazy.get_time() == 1);
        assert(root.sub_another.sub_one.get_time() == 1);
        assert(root.sub_another.sub_lazy.get_time() == 1);
        // level 3
        assert(root.sub_another.sub_one.lazy.get_time() == 1);
        
        std::cout << "  correct" << std::endl;

        std::cout << "Tests successful. :)" << std::endl;

        cleanup(tmpfile, tmpfile_path);

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
