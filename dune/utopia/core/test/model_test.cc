#include <cassert>

#include "model_test.hh"

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // -- Setup model -- //
        // get the test config file (path is relative to executable!)
        std::cout << "Loading config ..." << std::endl;
        Utopia::DataIO::Config cfg("model_test.yml");
        std::cout << "Config loaded." << std::endl;

        // create a temporary file and get the basegroup
        auto output_path = cfg["output_path"].as<std::string>();
        std::cout << "  output_path: " << output_path << std::endl;
        auto file = Utopia::DataIO::HDFFile(output_path, "w");
        auto basegroup = file.get_basegroup();

        // initialize an RNG
        auto seed = cfg["seed"].as<int>();
        std::cout << "  seed: " << seed << std::endl;
        auto rng = std::make_shared<std::mt19937>(seed);

        // initial state vector for both model instances
        std::vector<double> state(1E6, 0.0);

        // create the model instances
        std::cout << "Setting up model instances ..." << std::endl;
        
        // the test model
        Utopia::TestModel model("test", cfg, basegroup, rng,
                                state);

        // and the one with an overwritten iterate method
        Utopia::TestModelWithIterate model_it("test_it", cfg, basegroup, rng,
                                              state);
        
        std::cout << "Models initialized." << std::endl;

        // -- Tests begin here -- //
        std::cout << "Commencing tests ..." << std::endl;

        // assert initial state
        assert(compare_containers(model.data(), state));

        // assert state after first iteration
        model.iterate();
        state = std::vector<double>(1E6, 1.0);
        assert(compare_containers(model.data(), state));

        // set boundary conditions and check again
        model.set_boundary_condition(std::vector<double>(1E6, 2.0));
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        // reset states
        state = std::vector<double>(1E6, 1.0);
        model.set_initial_condition(state);
        assert(compare_containers(model.data(), state));

        // check override of iterate function in model_it
        model_it.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.data(), state));

        std::cout << "All done. :)" << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Failed!" << std::endl;
        return 1;
    }
}
