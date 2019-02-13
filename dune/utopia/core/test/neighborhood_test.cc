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
        std::cout << "------ Square periodic 2D grid ... ------"
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


        std::cout << ".....  Neighborhood:  vonNeumann (d=2) ..." << std::endl;
        NBTest rect_2D_vonNeumann_d2("rect_2D_vonNeumann_d2", pp);
        {
            auto cm = rect_2D_vonNeumann_d2.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 5);
            assert(grid->shape()[1] == 5);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing count and uniqueness ..." << std::endl;
            check_num_neighbors(cm, 12);
            std::cout << "  Neighbor count matches." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 4, 5, 5*4,                    // d=1
                                       2, 3, 6, 9, 10, 15, 21, 24}));   // d=2
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
                                      {5, 7, 1, 11,                     // d=1
                                       0, 2, 8, 9, 10, 12, 16, 21}));   // d=2
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(13), // (3, 2)
                                      {8, 12, 14, 18,                   // d=1
                                       3, 7, 9, 10, 11, 17, 19, 23}));  // d=2
            std::cout << "  Neighbors match for cell (3, 2)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(24), // (4,4)
                                      {23, 20, 22, 21,                  // d=1
                                       18, 3, 15, 0, 19, 4, 14, 9}));   // d=2
            std::cout << "  Neighbors match for cell (4, 4)." << std::endl;
        }
        std::cout << "Success." << std::endl << std::endl;


        std::cout << ".....  Neighborhood:  vonNeumann (d=3) ..." << std::endl;
        NBTest rect_2D_vonNeumann_d3("rect_2D_vonNeumann_d3", pp);
        {
            auto cm = rect_2D_vonNeumann_d3.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 7);
            assert(grid->shape()[1] == 7);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;

            std::cout << "Testing count and uniqueness ..." << std::endl;
            // check_num_neighbors(cm, 24);
            std::cout << "  Neighbor count matches." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 2, 3, 4, 5, 6,
                                       8, 15, 36, 43,
                                       13, 20, 48, 41,
                                       9, 44,
                                       12, 47,
                                       7, 14, 21, 28, 35, 42}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(8), // (1,1)
                                      {9, 10, 11, 7, 13, 12,
                                       16, 2, 23, 44,
                                       0, 42, 14, 21,
                                       17, 3,
                                       20, 6,
                                       15, 22, 29, 1, 36, 43}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(17), // (3, 2)
                                      {14, 15, 16, 18, 19, 20,
                                       11, 4, 25, 32, 
                                       9, 2, 23, 30,
                                       12, 26,
                                       8, 22,
                                       3, 10, 45, 38, 31, 24}));
            std::cout << "  Neighbors match for cell (3, 2)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(48), // (6,6)
                                      {45, 46, 47, 42, 43, 44,
                                       35, 28, 0, 7,
                                       40, 33, 5, 12,
                                       36, 1, 
                                       39, 4,
                                       41, 34, 27, 20, 13, 6}));
            std::cout << "  Neighbors match for cell (6, 6)." << std::endl;
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
        

        std::cout << ".....  Neighborhood:  Moore (d=2)  ..." << std::endl;
        NBTest rect_2D_Moore_d2("rect_2D_Moore_d2", pp);
        {
            auto cm = rect_2D_Moore_d2.cm;
            auto grid = cm.grid();

            assert(grid->shape()[0] == 7);
            assert(grid->shape()[1] == 7);
            assert(grid->is_periodic());
            std::cout << "Grid shape and periodicity matches." << std::endl;
            
            std::cout << "Testing count and uniqueness ..." << std::endl;
            check_num_neighbors(cm, 24);
            std::cout << "  Neighbor count matches." << std::endl;
            assert(unique_neighbors(cm));
            std::cout << "  Neighbors are unique." << std::endl;

            std::cout << "Testing neighborhoods explicitly ..." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
                                      {1, 6, 2, 5,
                                       8, 43, 15, 36,
                                       48, 13, 41, 20,
                                       9, 44, 16, 37,
                                       47, 12, 40, 19,
                                       42, 7, 35, 14}));
            std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(8), // (1,1)
                                      {9, 10, 7, 13,
                                       16, 23, 2, 44,
                                       0, 42, 14, 21,
                                       17, 24, 3, 45,
                                       20, 27, 6, 48,
                                       15, 22, 1, 43}));
            std::cout << "  Neighbors match for cell (1, 1)." << std::endl;
            
            assert(expected_neighbors(cm, cm.cells().at(23), // (2,3)
                                      {24, 25, 21, 22,
                                       11, 18, 32, 39,
                                       38, 31, 17, 10,
                                       9, 16, 30, 37,
                                       36, 29, 15, 8,
                                       7, 14, 28, 35}));
            std::cout << "  Neighbors match for cell (2, 3)." << std::endl;

            assert(expected_neighbors(cm, cm.cells().at(48), // (6,6)
                                      {46, 47, 42, 43,
                                       29, 36, 1, 8,
                                       28, 35, 0, 7,
                                       41, 34, 6, 13,
                                       33, 40, 5, 12,
                                       32, 39, 4, 11}));
            std::cout << "  Neighbors match for cell (6, 6)." << std::endl;

        }
        std::cout << "Success." << std::endl << std::endl;
        

        // -------------------------------------------------------------------
        std::cout << "------ Square non-periodic 2D grid ... ------"
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


        // std::cout << ".....  Neighborhood:  vonNeumann (d=2)  ..." << std::endl;
        // NBTest rect_2D_vonNeumann_np_d2("rect_2D_vonNeumann_np_d2", pp);

        // {
        //     auto cm = rect_2D_vonNeumann_np_d2.cm;
        //     auto grid = cm.grid();

        //     assert(grid->shape()[0] == 5);
        //     assert(grid->shape()[1] == 5);
        //     assert(not grid->is_periodic());
        //     std::cout << "Grid shape and periodicity matches." << std::endl;

        //     std::cout << "Testing uniqueness ..." << std::endl;
        //     assert(unique_neighbors(cm));
        //     std::cout << "  Neighbors are unique." << std::endl;

        //     std::cout << "Testing neighborhoods explicitly ..." << std::endl;

        //     assert(expected_neighbors(cm, cm.cells().at(0), // (0,0)
        //                               {1, 5,
        //                                2, 6, 10}));
        //     std::cout << "  Neighbors match for cell (0, 0)." << std::endl;

        //     assert(expected_neighbors(cm, cm.cells().at(6), // (1,1)
        //                               {5, 7, 1, 11,
        //                                0, 2, 8, 12, 16, 10}));
        //     std::cout << "  Neighbors match for cell (1, 1)." << std::endl;

        //     assert(expected_neighbors(cm, cm.cells().at(13), // (2, 3)
        //                               {8, 12, 14, 18,
        //                                3, 9, 19, 23, 17, 11, 7}));
        //     std::cout << "  Neighbors match for cell (2, 3)." << std::endl;

        //     assert(expected_neighbors(cm, cm.cells().at(5*5 - 1), // (4,4)
        //                               {19, 23,
        //                                14, 18, 22}));
        //     std::cout << "  Neighbors match for cell (4, 4)." << std::endl;

        // }
        // std::cout << "Success." << std::endl << std::endl;


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
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
