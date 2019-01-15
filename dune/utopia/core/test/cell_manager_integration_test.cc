#include <cassert>
#include <iostream>

#include <dune/utopia/core/cell_manager.hh>

#include "cell_manager_integration_test.hh"

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

        // TODO Initialize a model with a cell manager

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
