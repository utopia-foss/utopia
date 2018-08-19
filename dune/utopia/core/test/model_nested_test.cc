#include <cassert>

#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/cfg_utils.hh>
#include <dune/utopia/core/logging.hh>

#include "model_nested_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // -- Setup model -- //
        // create a pseudo parent
        std::cout << "Initializing pseudo parent ..." << std::endl;

        Utopia::PseudoParent pp("model_nested_test.yml");

        // get the logger that was created by the pseudo parent
        auto log = spdlog::get(Utopia::log_core);

        // create the model instances
        log->debug("Initializing RootModel instance ...");
        
        Utopia::RootModel root("root", pp);

        log->debug("RootModel 'root' initialized.");

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
        log->debug("Commencing tests ...");

        // Iterate model; should also iterate submodels
        log->debug("Performing single iteration ...");
        root.iterate();

        // Check that all models were iterated
        log->debug("Asserting correct iteration ...");
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

        // check log level propagation
        assert(root.get_logger()->level() == spdlog::level::debug);
        assert(root.sub_another.get_logger()->level() == spdlog::level::debug);
        assert(root.sub_one.get_logger()->level() == spdlog::level::trace);
        assert(root.sub_one.lazy.get_logger()->level() == spdlog::level::trace);

        // check different random numbers are drawn from each submodel
        assert((*root.get_rng())() != (*root.sub_one.get_rng())());
        assert((*root.sub_one.get_rng())() != (*root.sub_another.get_rng())());
        assert((*root.sub_another.get_rng())() != (*root.sub_one.lazy.get_rng())());
        assert((*root.sub_one.lazy.get_rng())() != (*root.sub_another.sub_one.lazy.get_rng())());

        // check RNG with same seed gives same value
        Utopia::DefaultRNG rng(Utopia::as_<int>(pp.get_cfg()["seed"]));
        rng.discard(8);
        assert(rng() == (*root.get_rng())());

        log->info("Tests successful. :)");

        // Cleanup
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());

        log->debug("Temporary files removed.");

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
