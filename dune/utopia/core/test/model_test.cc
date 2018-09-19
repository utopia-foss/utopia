#include <cassert>

#include <dune/utopia/data_io/hdffile.hh>

#include "model_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        // -- Setup model -- //
        // create a pseudo parent
        std::cout << "Initializing pseudo parent ..." << std::endl;

        Utopia::PseudoParent pp("model_test.yml");

        // initial state vector for both model instances
        std::vector<double> state(1E6, 0.0);

        // create the model instances
        std::cout << "Setting up model instances ..." << std::endl;
        
        // the test model
        Utopia::TestModel model("test", pp, state);

        // and the one with an overwritten iterate method
        Utopia::TestModelWithIterate model_it("test_it", pp, state);
        
        std::cout << "Models initialized." << std::endl;

        // -- Tests begin here -- //
        std::cout << "Commencing tests ..." << std::endl;

        // assert initial state
        std::cout << "  initial state" << std::endl;
        assert(compare_containers(model.state(), state));
        std::cout << "  correct" << std::endl;

        // assert state after first iteration
        std::cout << "  after one iteration" << std::endl;
        model.iterate();
        state = std::vector<double>(1E6, 1.0);
        assert(compare_containers(model.state(), state));
        std::cout << "  correct" << std::endl;

        // set boundary conditions and check again
        std::cout << "  setting boundary condition + iterate" << std::endl;
        model.set_bc(std::vector<double>(1E6, 2.0));
        model.iterate();
        state = std::vector<double>(1E6, 3.0);
        assert(compare_containers(model.state(), state));
        std::cout << "  correct" << std::endl;

        // set state manually and assert it worked
        std::cout << "  setting initial condition" << std::endl;
        state = std::vector<double>(1E6, 1.0);
        model.set_state(state);
        assert(compare_containers(model.state(), state));
        std::cout << "  correct" << std::endl;

        // check override of iterate function in model_it, which was not
        // iterated yet
        std::cout << "  iterate model with custom iterate method" << std::endl;
        model_it.iterate();
        state = std::vector<double>(1E6, 2.0);
        assert(compare_containers(model_it.state(), state));
        std::cout << "  correct" << std::endl;

        std::cout << "Tests successful. :)" << std::endl;

        
        // Cleanup
        auto pp_file = pp.get_hdffile();
        pp_file->close();
        std::remove(pp_file->get_path().c_str());

        std::cout << "Temporary files removed." << std::endl;

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
