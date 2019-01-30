#include <cassert>
#include <iostream>

#include <dune/utopia/core/logging.hh>
#include <dune/utopia/data_io/cfg_utils.hh>
#include <dune/utopia/core/space.hh>
#include <dune/utopia/core/grids.hh>


#include "testtools.hh"

// Import some types
using Utopia::DataIO::Config;
using namespace Utopia;
using SpaceMap = std::map<std::string, DefaultSpace>;


/// Make sure the number of cells is as expected; values taken from config
bool check_num_cells(std::string grid_name, SpaceMap spaces, Config cfg) {
    std::cout << "Testing num_cells() method for grid '" << grid_name
              << "' ..." << std::endl << std::endl;

    if (not cfg["grids"][grid_name]) {
        throw std::invalid_argument("Missing grid config '"+grid_name+"'!");
    }

    const auto grid_cfg = cfg["grids"][grid_name];

    if (not grid_cfg["expected_num_cells"]) {
        throw std::invalid_argument("Missing expected_num_cells entry in grid "
                                    "config of grid '"+grid_name+"'!");
    }
    const auto expected_num_cells = grid_cfg["expected_num_cells"];

    // For each space, retrieve the expected number of cells, and construct
    // a grid from it. Then check the number or, if given negative, whether
    // construction fails in the expected way
    for (const auto& sp: spaces) {
        auto space_name = sp.first;
        auto space = std::make_shared<DefaultSpace>(sp.second);

        std::cout << "... in combination with '" << space_name << "' space ..."
                  << std::endl;

        if (not expected_num_cells[space_name]) {
            throw std::invalid_argument("Missing expected_num_cells entry for "
                                        "space '" + space_name + "'!");
        }
        const auto enc = as_int(expected_num_cells[space_name]);

        if (enc >= 0) {
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
            continue;
        }

        // else: expect it to fail
        std::cout << "   Expecting grid construction to fail ..." << std::endl;

        std::string expected_err_msg;
        if (enc == -1) {
            expected_err_msg = "Given the extent of the physical space and "
                               "the specified resolution, a mapping with "
                               "exactly square cells could not be found!";
        }
        else if (enc == -2) {
            expected_err_msg = "Grid resolution needs to be a positive "
                               "integer!";
        }
        else if (enc == -3) {
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

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

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
        std::cout << "------ Testing grid methods ... ------"
                  << std::endl;

        std::cout << "- - -  Grid:  small_res  - - -" << std::endl;
        assert(check_num_cells("small_res", spaces, cfg));

        std::cout << "- - -  Grid:  medium_res  - - -" << std::endl;
        assert(check_num_cells("medium_res", spaces, cfg));

        std::cout << "- - -  Grid:  invalid_res  - - -" << std::endl;
        assert(check_num_cells("invalid_res", spaces, cfg));

        std::cout << "- - -  Grid:  missing_res  - - -" << std::endl;
        assert(check_num_cells("missing_res", spaces, cfg));



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
