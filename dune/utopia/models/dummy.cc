#include <iostream>       // std::cout, std::endl
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <dune/utopia/utopia.hh>

#include "dummy.hh"

int main (int argc, char** argv)
{
    try {
        std::vector<double> state(1E6, 0.0);
        Utopia::DummyModel model(state);

        assert(compare_containers(model.data(), state));

        model.iterate();
        state = std::vector<double>(1E6, 1.0);
        assert(compare_containers(model.data(), state));

        model.set_boundary_condition(std::vector<double>(1E6, 2.0));
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        state = std::vector<double>(1E6, 1.0);
        model.set_initial_condition(state);
        assert(compare_containers(model.data(), state));

        // check override of iterate function
        Utopia::DummyModelWithIterate model_it(state);
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        // Sleep (to be read by frontend)
        unsigned int sleep_time = 1000; // in milliseconds
        unsigned int num_sleeps = 10;

        for (int i = 0; i < num_sleeps; ++i) {
            std::cout << "Sleep #" << (i+1) << " ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }

        std::cout << "All done." << std::endl << "Really." << std::endl;

        return 0;
    }
    catch (...) {
        return 1;
    }
}