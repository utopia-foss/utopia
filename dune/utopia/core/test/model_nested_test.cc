#include <cassert>

#include <dune/utopia/data_io/hdffile.hh>

#include "model_nested_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // -- Setup model -- //
        // create a pseudo parent
        std::cout << "Initializing pseudo parent ..." << std::endl;

        Utopia::PseudoParent pp("model_nested_test.yml");

        // create the model instances
        std::cout << "Initializing RootModel instance ..." << std::endl;
        
        Utopia::RootModel root("root", pp);

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

        // Cleanup
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());

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
