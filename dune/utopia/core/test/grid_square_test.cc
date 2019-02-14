#include <cassert>
#include <iostream>

#include <armadillo>

#include <dune/utopia/core/logging.hh>
#include <dune/utopia/data_io/cfg_utils.hh>
#include <dune/utopia/core/space.hh>
#include <dune/utopia/core/grids.hh>

#include "testtools.hh"


// Import some types
using Utopia::DataIO::Config;
using namespace Utopia;
using SpaceMap = std::map<std::string, DefaultSpace>;  // 2D space
using MultiIndex = MultiIndexType<2>;


/// Make sure the number of cells is as expected; values taken from config
bool check_num_cells_and_shape(std::string grid_name,
                               SpaceMap spaces,
                               Config cfg)
{
    std::cout << "Testing num_cells() and shape() method for grid '"
              << grid_name << "' ..." << std::endl << std::endl;

    if (not cfg["grids"][grid_name]) {
        throw std::invalid_argument("Missing grid config '"+grid_name+"'!");
    }

    const auto grid_cfg = cfg["grids"][grid_name];

    if (not grid_cfg["expected_shapes"]) {
        throw std::invalid_argument("Missing expected_shapes entry in grid "
                                    "config of grid '" + grid_name + "'!");
    }
    const auto expected_shapes = grid_cfg["expected_shapes"];

    // For each space, retrieve the expected number of cells, and construct
    // a grid from it. Then check the number or, if given negative, whether
    // construction fails in the expected way
    for (const auto& sp: spaces) {
        auto space_name = sp.first;
        auto space = std::make_shared<DefaultSpace>(sp.second);

        std::cout << "... in combination with '" << space_name << "' space ..."
                  << std::endl;

        if (not expected_shapes[space_name]) {
            throw std::invalid_argument("Missing expected_shapes entry for "
                                        "space '" + space_name + "'!");
        }

        // Extract the expected shape or a failure code
        MultiIndex exp_shape;
        int fail_code = 0;

        try {
            exp_shape = as_MultiIndex<2>(expected_shapes[space_name]);
        }
        catch (std::runtime_error&) {
            // No valid shape given; interpret as failure code
            fail_code = as_int(expected_shapes[space_name]);
        }

        if (fail_code == 0) {
            // Calculate expected number of cells
            const auto enc = std::accumulate(exp_shape.begin(),
                                             exp_shape.end(),
                                             1, std::multiplies<IndexType>());

            SquareGrid<DefaultSpace> grid(space, grid_cfg);

            std::cout << "   Grid '" << grid_name << "' constructed "
                      << "successfully with '" << space_name << "' space."
                      << std::endl;

            if (((int) grid.num_cells()) != enc) {
                std::cerr << "ERROR: Number of cells did not match! Expected "
                          << enc << " but grid returned " << grid.num_cells()
                          << std::endl;

                return false;
            }

            std::cout << "   Number of cells match expected number."
                      << std::endl << std::endl;

            // Also check the shape
            if (arma::any(grid.shape() != exp_shape)) {
                std::cerr << "ERROR: Shape did not match! Expected:"
                          << std::endl << exp_shape << "But grid returned:"
                          << std::endl << grid.shape() << std::endl;

                return false;
            }

            continue; // ... with the next space
        }

        // else: expect it to fail
        std::cout << "   Expecting grid construction to fail ..." << std::endl;

        std::string expected_err_msg;
        if (fail_code == -1) {
            expected_err_msg = "Given the extent of the physical space and "
                               "the specified resolution, a mapping with "
                               "exactly square cells could not be found!";
        }
        else if (fail_code == -2) {
            expected_err_msg = "Grid resolution needs to be a positive "
                               "integer!";
        }
        else if (fail_code == -3) {
            expected_err_msg = "Missing grid configuration parameter "
                               "'resolution'!";
        }
        else {
            throw std::invalid_argument("If expected_num_cells is negative, "
                "needs to map to a valid error message via -1, -2, or -3.");
        }

        if (!check_error_message<std::invalid_argument>(
                grid_name + " grid, " + space_name + " space",
                [&](){ SquareGrid<DefaultSpace> grid(space, grid_cfg); },
                expected_err_msg,
                "   ", true))
        {
            // Did not fail as expected
            std::cerr << "ERROR: Construction of grid '" << grid_name
                      << "' with '" << space_name << "' space should have "
                      "failed, but did not!" << std::endl;
            return false;
        }
    }

    std::cout << "Tests succeeded for the above grid-space combinations."
              << std::endl << std::endl;

    return true;
}





// ----------------------------------------------------------------------------

int main(int, char *[]) {
    try {
        std::cout << "Loading config file ..." << std::endl;
        auto cfg = YAML::LoadFile("grid_square_test.yml");
        std::cout << "Success." << std::endl << std::endl;

        // -------------------------------------------------------------------
        std::cout << "Initializing test spaces ..." << std::endl;
        
        SpaceMap spaces;

        spaces.emplace(std::make_pair("default",
                                      DefaultSpace()));
        spaces.emplace(std::make_pair("nice",
                                      DefaultSpace(cfg["spaces"]["nice"])));
        spaces.emplace(std::make_pair("uneven",
                                      DefaultSpace(cfg["spaces"]["uneven"])));
        spaces.emplace(std::make_pair("nasty",
                                      DefaultSpace(cfg["spaces"]["nasty"])));
        spaces.emplace(std::make_pair("devil",
                                      DefaultSpace(cfg["spaces"]["devil"])));
        std::cout << "Success." << std::endl << std::endl;


        std::cout << "Checking extents ..." << std::endl;
        
        assert(spaces["default"].extent[0] == 1.);
        assert(spaces["default"].extent[1] == 1.);
        
        assert(spaces["nice"].extent[0] == 4.);
        assert(spaces["nice"].extent[1] == 4.);
        
        assert(spaces["uneven"].extent[0] == 2.);
        assert(spaces["uneven"].extent[1] == 3.);
        
        assert(spaces["nasty"].extent[0] == 1.25);
        assert(spaces["nasty"].extent[1] == 3.2);
        
        assert(spaces["devil"].extent[0] == 1.23);
        assert(spaces["devil"].extent[1] == 3.14);

        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing number of cells ... ------"
                  << std::endl;

        std::cout << "- - -  Grid:  tiny_res  - - -" << std::endl;
        assert(check_num_cells_and_shape("tiny_res", spaces, cfg));

        std::cout << "- - -  Grid:  small_res  - - -" << std::endl;
        assert(check_num_cells_and_shape("small_res", spaces, cfg));

        std::cout << "- - -  Grid:  medium_res  - - -" << std::endl;
        assert(check_num_cells_and_shape("medium_res", spaces, cfg));

        std::cout << "- - -  Grid:  invalid_res  - - -" << std::endl;
        assert(check_num_cells_and_shape("invalid_res", spaces, cfg));

        std::cout << "- - -  Grid:  missing_res  - - -" << std::endl;
        assert(check_num_cells_and_shape("missing_res", spaces, cfg));




        // -------------------------------------------------------------------
        std::cout << "------ Testing multi-index queries ... ------"
                  << std::endl;

        { // Local test scope

        // Use the tiny_res grid in combination with different spaces
        const auto grid_cfg = cfg["grids"]["tiny_res"]; // resolution: 1

        // Enough to test with these two, have shapes [1,1] and [2,3]
        auto g11 = SquareGrid<DefaultSpace>(spaces["default"], grid_cfg);
        auto g23 = SquareGrid<DefaultSpace>(spaces["uneven"], grid_cfg);

        // Default space, only (0, 0) available
        assert(arma::all(g11.midx_of(0) == MultiIndex({0, 0})));

        // ... but no bounds checking, so this is also computed
        assert(arma::all(g11.midx_of(1) == MultiIndex({0, 1})));

        // For the uneven (2x3) space
        assert(arma::all(g23.midx_of(0) == MultiIndex({0, 0})));
        assert(arma::all(g23.midx_of(1) == MultiIndex({1, 0})));
        assert(arma::all(g23.midx_of(2) == MultiIndex({0, 1})));
        assert(arma::all(g23.midx_of(3) == MultiIndex({1, 1})));
        assert(arma::all(g23.midx_of(4) == MultiIndex({0, 2})));
        assert(arma::all(g23.midx_of(5) == MultiIndex({1, 2})));



        } // End of local test scope

        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing position-related methods ... ------"
                  << std::endl;
        // TODO
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
