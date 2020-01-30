#include <cassert>

#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/cfg_utils.hh>
#include <utopia/core/logging.hh>

#include "model_coupled_test.hh"


int main() {
    try {
        // -- Setup model -- //
        // create a pseudo parent
        std::cout << "Initializing pseudo parent ..." << std::endl;

        Utopia::PseudoParent pp("model_coupled_test.yml");

        // get the logger that was created by the pseudo parent
        auto log = spdlog::get(Utopia::log_core);

        // create the model instances
        log->debug("Initializing RootModel instance ...");
        
        Utopia::RootModel root("root", pp);

        log->debug("RootModel 'root' initialized.");

        /** Created model hierarchy:
         *
         *   0               Root (run for 10 steps)
         *                  /   \
         *                 /      ----------------- \
         *   1          One (iterated, until stop)   \
         *               |                         Another (iterated from start)
         *               |                        /               \
         *   2       DoNothing (iterated)      One (iterated)   DoNothing
         *                                      |               (run in prolog)
         *                                      |
         *   3                               DoNothing (iterated)
         */

        // -- Tests begin here -- //
        log->debug("Commencing tests ...");

        // Iterate model; should also iterate submodels
        log->debug("Performing single iteration ...");
        root.run();

        assert(root._prolog_run);
        assert(root._epilog_run);

        assert(root.sub_one._prolog_run);
        assert(root.sub_one._epilog_run);

        assert(root.sub_one.sub_lacy._prolog_run);
        assert(root.sub_one.sub_lacy._epilog_run);


        assert(root.sub_another._prolog_run);
        assert(root.sub_another._epilog_run);

        assert(root.sub_another.sub_lacy._prolog_run);
        assert(root.sub_another.sub_lacy._epilog_run);

        assert(root.sub_another.sub_one._prolog_run);
        assert(root.sub_another.sub_one._epilog_run);

        assert(root.sub_another.sub_one.sub_lacy._prolog_run);
        assert(root.sub_another.sub_one.sub_lacy._epilog_run);


        assert(root.get_time() == 10); // time_max = 10

        assert(root.sub_one.get_time() == 3); // time stop = 3
        assert(root.sub_one.sub_lacy.get_time() == 3);

        assert(root.sub_another.get_time() == 6); // time start = 5
        assert(root.sub_another.sub_one.get_time() == 6);
        assert(root.sub_another.sub_one.sub_lacy.get_time() == 6);

        assert(root.sub_another.sub_lacy.get_time() == 10); // time_max = 10


        // Check that all models were iterated
        log->debug("Asserting correct iteration ...");
        

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
