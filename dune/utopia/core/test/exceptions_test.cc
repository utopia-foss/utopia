#include <cassert>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/exceptions.hh>

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Test basic interface
        Utopia::Exception e0("what");
        assert(e0.what() == "what");
        assert(e0.exit_code == 1);

        // Test specific classes
        // GotSignal
        Utopia::GotSignal gs0(SIGINT);
        assert(gs0.what() == "Received signal: 2");
        assert(gs0.exit_code == 128 + 2);

        Utopia::GotSignal gs1(-2);
        assert(gs1.what() == "Received signal: -2");
        assert(gs1.exit_code == 128 + abs(-2));

        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
