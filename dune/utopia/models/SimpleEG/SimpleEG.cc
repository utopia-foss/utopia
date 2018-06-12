#include <iostream>       // std::cout, std::endl

#include "SimpleEG.hh"

using namespace Utopia::Models;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // -- Initialization -- //
        // Get path to config file
        const std::string cfg_path = argv[1];

        // Initialize the pseudo parent of this model using that config
        Utopia::PseudoParent pp(cfg_path);

        // Still need config for iteration and setup function
        // FIXME ... there must be a nicer way! Let PP do the iteration and
        //       let the setup function take the pseudo parent as argument?
        auto cfg = pp.get_cfg();

        // Initialize the main model instance
        SimpleEGModel model("SimpleEG", pp,
                            setup_manager<>(cfg["SimpleEG"], pp.get_rng()));
        // NOTE The manager is created via a setup function


        // -- Iteration -- //
        // Iterate now ...
        for (unsigned int i = 0; i < cfg["num_steps"].as<unsigned int>(); i++)
        {
            model.iterate();
        }

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
