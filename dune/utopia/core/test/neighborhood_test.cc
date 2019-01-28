#include <cassert>
#include <iostream>

#include "neighborhood_test.hh"
#include "testtools.hh"

using namespace Utopia::Test;

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

        std::cout << "Initializing pseudo parent ..." << std::endl;
        Utopia::PseudoParent pp("neighborhood_test.yml");
        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Rectangular periodic 2D grid ... ------"
                  << std::endl;

        std::cout << ".....  Neighborhood:  Empty  ..." << std::endl;
        NBTest rect_2D_empty("rect_2D_empty", pp);
        std::cout << "Test model set up." << std::endl;

        { // Test scope - to keep the main scope clean
            auto cm = rect_2D_empty.cm;
            auto grid = cm.grid();

            // Check grid shape
            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            // Check neighbors count for all cells
            std::cout << "Testing neighbor count ..." << std::endl;
            check_num_neighbors(cm, 0);
            std::cout << "Neighbor count matches." << std::endl;

        } // End of test scope
        std::cout << "Success." << std::endl << std::endl;


        std::cout << ".....  Neighborhood:  vonNeumann  ..." << std::endl;
        NBTest rect_2D_vonNeumann("rect_2D_vonNeumann", pp);
        std::cout << "Test model set up." << std::endl;

        {
            auto cm = rect_2D_vonNeumann.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing neighbor count ..." << std::endl;
            check_num_neighbors(cm, 4);
            std::cout << "Neighbor count matches." << std::endl;

            // Check that the neighbors are the expected neighbors
            // TODO

        }
        std::cout << "Success." << std::endl << std::endl;


        std::cout << ".....  Neighborhood:  Moore  ..." << std::endl;
        NBTest rect_2D_Moore("rect_2D_Moore", pp);
        std::cout << "Test model set up." << std::endl;

        {
            auto cm = rect_2D_Moore.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing neighbor count ..." << std::endl;
            check_num_neighbors(cm, 8);
            std::cout << "Neighbor count matches." << std::endl;

            // Check that the neighbors are the expected neighbors
            // TODO

        }
        std::cout << "Success." << std::endl << std::endl;
        


        // -------------------------------------------------------------------
        std::cout << "------ Rectangular non-periodic 2D grid ... ------"
                  << std::endl;

        // TODO
        // std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        // Done.
        std::cout << "------ Total success. ------" << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occured: " << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
