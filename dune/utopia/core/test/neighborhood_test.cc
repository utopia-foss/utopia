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
        {
            auto cm = rect_2D_vonNeumann.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing count and uniqueness ..." << std::endl;
            check_num_neighbors(cm, 4);
            std::cout << "  Neighbor count matches." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 4, 5, 5*4}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
                                      {5, 7, 1, 11}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(13), // (2, 3)
                                      {8, 12, 14, 18}));
            std::cout << "  Neighbors match for cell (2, 3)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(5*5 - 1), // (4,4)
                                      {5*5-2, 5*4, 5*4-1, 4}));
            std::cout << "  Neighbors match for cell (4, 4)." << std::endl;

        }
        std::cout << "Success." << std::endl << std::endl;


        std::cout << ".....  Neighborhood:  Moore  ..." << std::endl;
        NBTest rect_2D_Moore("rect_2D_Moore", pp);
        {
            auto cm = rect_2D_Moore.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;
            
            std::cout << "Testing count and uniqueness ..." << std::endl;
            check_num_neighbors(cm, 8);
            std::cout << "  Neighbor count matches." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 4, 5, 6, 9, 20, 21, 24}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
                                      {0, 1, 2, 5, 7, 10, 11, 12}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;
            
            assert(expected_neighbors(cm, cm.cells().at(13), // (2,3)
                                      {7, 8, 9, 12, 14, 17, 18, 19}));
            std::cout << "  Neighbors match for cell (2, 3)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(24), // (4,4)
                                      {0, 3, 4, 15, 18, 19, 20, 23}));
            std::cout << "  Neighbors match for cell (4, 4)." << std::endl;

        }
        std::cout << "Success." << std::endl << std::endl;
        


        // -------------------------------------------------------------------
        std::cout << "------ Rectangular non-periodic 2D grid ... ------"
                  << std::endl;


        std::cout << ".....  Neighborhood:  vonNeumann  ..." << std::endl;
        NBTest rect_2D_vonNeumann_np("rect_2D_vonNeumann_np", pp);

        {
            auto cm = rect_2D_vonNeumann_np.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(not grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing uniqueness ..." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 5}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
                                      {5, 7, 1, 11}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(13), // (2, 3)
                                      {8, 12, 14, 18}));
            std::cout << "  Neighbors match for cell (2, 3)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(5*5 - 1), // (4,4)
                                      {19, 23}));
            std::cout << "  Neighbors match for cell (4, 4)." << std::endl;

        }
        std::cout << "Success." << std::endl << std::endl;


        std::cout << ".....  Neighborhood:  Moore  ..." << std::endl;
        NBTest rect_2D_Moore_np("rect_2D_Moore_np", pp);
        {
            auto cm = rect_2D_Moore_np.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(not grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;
            
            std::cout << "Testing uniqueness ..." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 5, 6}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
                                      {0, 1, 2, 5, 7, 10, 11, 12}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;
            
            assert(expected_neighbors(cm, cm.cells().at(13), // (2,3)
                                      {7, 8, 9, 12, 14, 17, 18, 19}));
            std::cout << "  Neighbors match for cell (2, 3)." << std::endl;
            
            assert(expected_neighbors(cm, cm.cells().at(14), // (2,4)
                                      {8, 9, 13, 18, 19}));
            std::cout << "  Neighbors match for cell (2, 4)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(24), // (4,4)
                                      {18, 19, 23}));
            std::cout << "  Neighbors match for cell (4, 4)." << std::endl;

        }
        std::cout << "Success." << std::endl << std::endl;


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
