#include <iostream>       // std::cout, std::endl

#include "SimpleEG.hh"

using namespace Utopia::Models;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Get path to config file
        const std::string cfg_path = argv[1];

        // Initialize the pseudo parent of this model using that config
        Utopia::PseudoParent pp(cfg_path);

        // Still need config for iteration and setup function
        // FIXME ... there must be a nicer way!
        auto cfg = pp.get_cfg();

        // Initialize the main model instance with different template arguments
        // and then iterate it ...
        // FIXME stop it with teh copy-pasta! Can't we do this in a better way?
        // if (cfg["SimpleEG"]["periodic"].as<bool>()) {
            // Periodic grid
            SimpleEGModel model("SimpleEG", pp,
                                setup_manager(cfg["SimpleEG"],
                                                    pp.get_rng()));

            for (std::size_t i = 0; i<cfg["num_steps"].as<std::size_t>(); i++){
                model.iterate();
            }

        // }
        // else {
        //     // Non-periodic grid
        //     SimpleEGModel model("SimpleEG", pp,
        //                         setup_manager<false>(cfg["SimpleEG"],
        //                                              pp.get_rng()));

        //     for (std::size_t i = 0; i<cfg["num_steps"].as<std::size_t>(); i++){
        //         model.iterate();
        //     }
        // }

        std::cout << "Done." << std::endl;

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
