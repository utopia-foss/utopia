#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "dummy.hh"

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        const std::string config_file = argv[1];
        Utopia::DataIO::Config config(config_file);

        // const unsigned int steps = std::stoi(argv[2]);

        std::vector<double> state(1E3, 0.0);
        Utopia::DummyModel model(state, config["dummy"]);

        // for(int i = 0; i < steps; ++i)
            model.iterate();

        // Sleep (to be read by frontend)
        unsigned int sleep_time = 500; // in milliseconds
        unsigned int num_sleeps = 5;

        for (unsigned int i = 0; i < num_sleeps; ++i) {
            std::cout << "Sleep #" << (i+1) << " ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }

        std::cout << "All done." << std::endl << "Really." << std::endl;

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
