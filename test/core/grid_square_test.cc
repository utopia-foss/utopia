#define BOOST_TEST_MODULE CoreGridSquare

#include <assert.h>
#include <iostream>
#include <boost/test/unit_test.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <armadillo>

#include <utopia/core/logging.hh>
#include <utopia/core/space.hh>
#include <utopia/core/grids.hh>
#include <utopia/data_io/cfg_utils.hh>

#include "testtools.hh"


// Import some types
using Utopia::DataIO::Config;
using namespace Utopia;
// 2D space
using SpaceMap = std::map<std::string, std::shared_ptr<DefaultSpace>>;
using MultiIndex = MultiIndexType<2>;
using SpaceVec = SpaceVecType<2>;


/// Compares two Armadillo vectors for equality
template<class T1, class T2>
bool check_eq(T1&& v1, T2&& v2) {
    if (not arma::all(v1 == v2)) {
        std::cerr << "ERROR: The given vectors" << std::endl
                  << v1 << "and" << std::endl << v2 << "are not equal!"
                  << std::endl;
        return false;
    }
    return true;
}

/// Checks whether the given position is mapped to the given cell ID
template<class Grid>
bool check_pos(Grid& grid, SpaceVec pos, IndexType expected_id) {
    IndexType cell_id;
    try {
        cell_id = grid.cell_at(pos);
    }
    catch (std::exception& e) {
        std::cerr << "ERROR: While retrieving the cell ID for position "
                  << std::endl << pos << ", the following exception occurred: "
                  << e.what() << std::endl;
        return false;
    }

    if (cell_id != expected_id) {
        std::cerr << "ERROR: The given position" << std::endl << pos
                  << "was not correctly mapped to the expected cell ID "
                  << expected_id << " but to: " << cell_id << std::endl;
        return false;
    }
    return true;
}



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
        auto& space_name = sp.first;
        auto space = sp.second;

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
            exp_shape = get_as_MultiIndex<2>(space_name, expected_shapes);
        }
        catch (std::runtime_error&) {
            // No valid shape given; interpret as failure code
            fail_code = get_as<int>(space_name, expected_shapes);
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
                               "integer, was < 1!";
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

struct Fixture {
    Config cfg;
    Config cfg_spaces;
    Config cfg_grids;

    SpaceMap spaces; /// A map of spaces defined in cfg_spaces

    SquareGrid<DefaultSpace> g11; /// An even 1x1 grid
    SquareGrid<DefaultSpace> g23; /// An uneven 2x3 grid
    SquareGrid<DefaultSpace> g23_np; /// A non-periodic 2x3 grid

    Fixture ()
    :
        cfg(YAML::LoadFile("grid_square_test.yml")),
        cfg_spaces(get_as<Config>("spaces", cfg)),
        cfg_grids(get_as<Config>("grids", cfg)),
        spaces({
            std::make_pair("default",
                           std::make_shared<DefaultSpace>()),
            std::make_pair("nice",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "nice", cfg_spaces))),
            std::make_pair("uneven",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "uneven", cfg_spaces))),
            std::make_pair("uneven_np",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "uneven_np", cfg_spaces))),
            std::make_pair("nasty",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "nasty", cfg_spaces))),
            std::make_pair("devil",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "devil", cfg_spaces)))
        }),
        g11(spaces["default"], get_as<Config>("tiny_res", cfg_grids)),
        g23(spaces["uneven"], get_as<Config>("tiny_res", cfg_grids)),
        g23_np(spaces["uneven_np"], get_as<Config>("tiny_res", cfg_grids))
    { }
};

BOOST_FIXTURE_TEST_SUITE (test_space_extent, Fixture)
    BOOST_AUTO_TEST_CASE(space_default) {
        BOOST_TEST(check_eq(spaces["default"]->extent, SpaceVec({1., 1.})));
    }
    BOOST_AUTO_TEST_CASE(space_nice) {
        BOOST_TEST(check_eq(spaces["nice"]->extent, SpaceVec({4., 4.})));
    }
    BOOST_AUTO_TEST_CASE(space_uneven) {
        BOOST_TEST(check_eq(spaces["uneven"]->extent, SpaceVec({2., 3.})));
    }
    BOOST_AUTO_TEST_CASE(space_uneven_np) {
        BOOST_TEST(check_eq(spaces["uneven_np"]->extent, SpaceVec({ 2., 3.})));
    }
    BOOST_AUTO_TEST_CASE(space_nasty) {
        BOOST_TEST(check_eq(spaces["nasty"]->extent, SpaceVec({1.25, 3.2})));
    }
    BOOST_AUTO_TEST_CASE(space_devil) {
        BOOST_TEST(check_eq(spaces["devil"]->extent, SpaceVec({1.23, 3.14})));
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE (test_number_of_cells, Fixture)
    BOOST_AUTO_TEST_CASE(space_default) {
        BOOST_TEST(check_num_cells_and_shape("tiny_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(space_nice) {
        BOOST_TEST(check_num_cells_and_shape("small_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(space_uneven) {
        BOOST_TEST(check_num_cells_and_shape("decimal_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(space_uneven_np) {
        BOOST_TEST(check_num_cells_and_shape("medium_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(space_nasty) {
        BOOST_TEST(check_num_cells_and_shape("invalid_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(space_devil) {
        BOOST_TEST(check_num_cells_and_shape("missing_res", spaces, cfg));
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE (test_multi_indices, Fixture)
    BOOST_AUTO_TEST_CASE(even_grid) {
        BOOST_TEST(check_eq(g11.midx_of(0), MultiIndex({0, 0})));
        BOOST_TEST(check_eq(g11.midx_of(1), MultiIndex({0, 1})));
    }

    BOOST_AUTO_TEST_CASE(uneven_grid) {
        BOOST_TEST(check_eq(g23.midx_of(0), MultiIndex({0, 0})));
        BOOST_TEST(check_eq(g23.midx_of(1), MultiIndex({1, 0})));
        BOOST_TEST(check_eq(g23.midx_of(2), MultiIndex({0, 1})));
        BOOST_TEST(check_eq(g23.midx_of(3), MultiIndex({1, 1})));
        BOOST_TEST(check_eq(g23.midx_of(4), MultiIndex({0, 2})));
        BOOST_TEST(check_eq(g23.midx_of(5), MultiIndex({1, 2})));
    }
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE (test_position_methods, Fixture)
    BOOST_AUTO_TEST_CASE(extent_of) {
        BOOST_TEST(check_eq(g11.extent_of(0), SpaceVec({1.0, 1.0})));
        
        for (unsigned short i=0; i<6; i++) {
            BOOST_TEST(check_eq(g23.extent_of(i), SpaceVec({1.0, 1.0})));
        }
    }

    BOOST_AUTO_TEST_CASE(barycenter_of) {
        BOOST_TEST(check_eq(g11.barycenter_of(0), SpaceVec({0.5, 0.5})));

        BOOST_TEST(check_eq(g23.barycenter_of(0), SpaceVec({0.5, 0.5})));
        BOOST_TEST(check_eq(g23.barycenter_of(1), SpaceVec({1.5, 0.5})));
        BOOST_TEST(check_eq(g23.barycenter_of(2), SpaceVec({0.5, 1.5})));
        BOOST_TEST(check_eq(g23.barycenter_of(3), SpaceVec({1.5, 1.5})));
        BOOST_TEST(check_eq(g23.barycenter_of(4), SpaceVec({0.5, 2.5})));
        BOOST_TEST(check_eq(g23.barycenter_of(5), SpaceVec({1.5, 2.5})));
    }

    BOOST_AUTO_TEST_CASE(vertex_positions) {
        auto g11_id0_vtcs = g11.vertices_of(0);
        BOOST_TEST(check_eq(g11_id0_vtcs[0], SpaceVec({0.0, 0.0})));
        BOOST_TEST(check_eq(g11_id0_vtcs[1], SpaceVec({1.0, 0.0})));
        BOOST_TEST(check_eq(g11_id0_vtcs[2], SpaceVec({1.0, 1.0})));
        BOOST_TEST(check_eq(g11_id0_vtcs[3], SpaceVec({0.0, 1.0})));

        auto g23_id5_vtcs = g23.vertices_of(5);
        BOOST_TEST(check_eq(g23_id5_vtcs[0], SpaceVec({1.0, 2.0})));
        BOOST_TEST(check_eq(g23_id5_vtcs[1], SpaceVec({2.0, 2.0})));
        BOOST_TEST(check_eq(g23_id5_vtcs[2], SpaceVec({2.0, 3.0})));
        BOOST_TEST(check_eq(g23_id5_vtcs[3], SpaceVec({1.0, 3.0})));
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE (test_cell_id_retrieval, Fixture)

    BOOST_AUTO_TEST_CASE(from_positive_position) {
        BOOST_TEST(g23.is_periodic());
        BOOST_TEST(g23.space()->extent[0] == 2.);
        BOOST_TEST(g23.space()->extent[1] == 3.);

        // Within the space, cells of size (1., 1.)
        BOOST_TEST(check_pos(g23, {0.0, 0.0},       0));
        BOOST_TEST(check_pos(g23, {0.5, 0.5},       0));
        BOOST_TEST(check_pos(g23, {0.314, 0.756},   0));

        BOOST_TEST(check_pos(g23, {0.1, 0.6},       0));
        BOOST_TEST(check_pos(g23, {1.1, 0.6},       1));
        BOOST_TEST(check_pos(g23, {0.1, 1.6},       2));
        BOOST_TEST(check_pos(g23, {1.1, 1.6},       3));
        BOOST_TEST(check_pos(g23, {0.1, 2.6},       4));
        BOOST_TEST(check_pos(g23, {1.1, 2.6},       5));

        // High-level cell boundaries chosen correctly
        BOOST_TEST(check_pos(g23, {0.99, 0.5},      0));
        BOOST_TEST(check_pos(g23, {1.0, 0.5},       1));
        BOOST_TEST(check_pos(g23, {0.99, 0.99},     0));
        BOOST_TEST(check_pos(g23, {1.0, 1.0},       3));

        // High-value space boundaries mapped periodically
        BOOST_TEST(check_pos(g23, {2.0, 0.0},       0));
        BOOST_TEST(check_pos(g23, {0.0, 3.0},       0));
        BOOST_TEST(check_pos(g23, {2.0, 3.0},       0));

        // Positions out of space mapped back into space
        BOOST_TEST(check_pos(g23, {2.5, 3.5},       0));
        BOOST_TEST(check_pos(g23, {3.5, 3.5},       1));
        BOOST_TEST(check_pos(g23, {2.5, 4.5},       2));
        BOOST_TEST(check_pos(g23, {3.5, 4.5},       3));
        BOOST_TEST(check_pos(g23, {2.5, 5.5},       4));
        BOOST_TEST(check_pos(g23, {3.5, 5.5},       5));

        // Positions waaaay out of space mapped back properly
        BOOST_TEST(check_pos(g23, {22.0, 33.0},     0));
        BOOST_TEST(check_pos(g23, {22.5, 33.5},     0));
        BOOST_TEST(check_pos(g23, {23.5, 33.5},     1));
        BOOST_TEST(check_pos(g23, {23.0, 34.0},     3));
        BOOST_TEST(check_pos(g23, {2222.0, 3333.0}, 0));
        BOOST_TEST(check_pos(g23, {2222.5, 3333.5}, 0));
        BOOST_TEST(check_pos(g23, {2223.0, 3333.5}, 1));
        BOOST_TEST(check_pos(g23, {2223.0, 3334.0}, 3));
    }

    BOOST_AUTO_TEST_CASE(from_negative_position) {
        // Positions out of space mapped back into space
        BOOST_TEST(check_pos(g23, {-1.5, -2.5},     0));
        BOOST_TEST(check_pos(g23, {-0.5, -2.5},     1));
        BOOST_TEST(check_pos(g23, {-1.5, -1.5},     2));
        BOOST_TEST(check_pos(g23, {-0.5, -1.5},     3));
        BOOST_TEST(check_pos(g23, {-1.5, -0.5},     4));
        BOOST_TEST(check_pos(g23, {-0.5, -0.5},     5));
        
        // High-value space boundaries mapped periodically
        BOOST_TEST(check_pos(g23, {-2.0, 0.0},      0));
        BOOST_TEST(check_pos(g23, {0.0, -3.0},      0));
        BOOST_TEST(check_pos(g23, {-2.0, -3.0},     0));

        // High-value cell boundaries mapped back properly
        BOOST_TEST(check_pos(g23, {-1.0, -3.0},     1));
        BOOST_TEST(check_pos(g23, {-1.0, -2.0},     3));
        BOOST_TEST(check_pos(g23, {-1.0, -1.0},     5));
        BOOST_TEST(check_pos(g23, {-2.0, -3.0},     0));
        BOOST_TEST(check_pos(g23, {-2.0, -2.0},     2));
        BOOST_TEST(check_pos(g23, {-2.0, -1.0},     4));
        
        // Positions waaaay out of space mapped back properly
        BOOST_TEST(check_pos(g23, {-19.5, 0.5},     0));
        BOOST_TEST(check_pos(g23, {-20., 0.5},      0));
        BOOST_TEST(check_pos(g23, {-20.5, 0.5},     1));
        BOOST_TEST(check_pos(g23, {-22.0, -33.0},   0));
        BOOST_TEST(check_pos(g23, {-23.0, -34.0},   5));
        BOOST_TEST(check_pos(g23, {-23.0, -35.0},   3));
        BOOST_TEST(check_pos(g23, {-2222., -3333.}, 0));
        BOOST_TEST(check_pos(g23, {-2223., -3335.}, 3));
    }

    BOOST_AUTO_TEST_CASE(non_periodic) {
        BOOST_TEST(not g23_np.is_periodic());
        BOOST_TEST(g23_np.space()->extent[0] == 2.);
        BOOST_TEST(g23_np.space()->extent[1] == 3.);

        // Within the space, cells of size (1., 1.)
        BOOST_TEST(check_pos(g23_np, {0.0, 0.0},       0));
        BOOST_TEST(check_pos(g23_np, {0.5, 0.5},       0));
        BOOST_TEST(check_pos(g23_np, {0.314, 0.756},   0));

        BOOST_TEST(check_pos(g23_np, {0.1, 0.6},       0));
        BOOST_TEST(check_pos(g23_np, {1.1, 0.6},       1));
        BOOST_TEST(check_pos(g23_np, {0.1, 1.6},       2));
        BOOST_TEST(check_pos(g23_np, {1.1, 1.6},       3));
        BOOST_TEST(check_pos(g23_np, {0.1, 2.6},       4));
        BOOST_TEST(check_pos(g23_np, {1.1, 2.6},       5));

        // High-level cell boundaries chosen correctly
        BOOST_TEST(check_pos(g23_np, {0.99, 0.5},      0));
        BOOST_TEST(check_pos(g23_np, {1.0, 0.5},       1));
        BOOST_TEST(check_pos(g23_np, {0.99, 0.99},     0));
        BOOST_TEST(check_pos(g23_np, {1.0, 1.0},       3));

        // High-value space boundaries map to boundary cells
        BOOST_TEST(check_pos(g23_np, {1.999, 0.0},     1));
        BOOST_TEST(check_pos(g23_np, {2.0,   0.0},     1));
        BOOST_TEST(check_pos(g23_np, {0.0,   2.999},   4));
        BOOST_TEST(check_pos(g23_np, {1.999, 2.999},   5));
        BOOST_TEST(check_pos(g23_np, {2.0,   3.0},     5));

        // Querying a position outside the space yields an error

        BOOST_TEST(check_error_message<std::invalid_argument>(
            "position query outside of space (for both arguments)",
            [&](){
                g23_np.cell_at({2.0001, 3.0001});
            },
            "given position is outside the non-periodic space",
            "   ", true)
        );
        BOOST_TEST(check_error_message<std::invalid_argument>(
            "position query outside of space (for single argument)",
            [&](){
                g23_np.cell_at({-0.0001, +0.0001});
            },
            "given position is outside the non-periodic space",
            "   ", true)
        );

    }

BOOST_AUTO_TEST_SUITE_END()


// Testing cell ID retrieval from positive positions
BOOST_FIXTURE_TEST_SUITE (test_boundary_retrieval_methods, Fixture)
    BOOST_AUTO_TEST_CASE(periodic) {
        // Use the decimal resolution (10 cells per length unit) to make
        // calculations easier.
        auto gdec_p = SquareGrid<DefaultSpace>(spaces["uneven"],
                                               get_as<Config>("decimal_res",
                                                               cfg_grids));
        
        // The periodic grid should always return an empty grid container
        BOOST_TEST(gdec_p.boundary_cells().size() == 0);  // == all
        BOOST_TEST(gdec_p.boundary_cells("all").size() == 0);
        BOOST_TEST(gdec_p.boundary_cells("left").size() == 0);
        BOOST_TEST(gdec_p.boundary_cells("right").size() == 0);
        BOOST_TEST(gdec_p.boundary_cells("top").size() == 0);
        BOOST_TEST(gdec_p.boundary_cells("bottom").size() == 0);

        BOOST_TEST(check_error_message<std::invalid_argument>(
            "invalid boundary cell argument does ALSO throw for periodic grid",
            [&](){
                gdec_p.boundary_cells("not a valid argument");
            },
            "Invalid value for argument `select` in call to method", "   ", true)
        );
    }

    BOOST_AUTO_TEST_CASE(non_periodic) {
        auto gdec_np = SquareGrid<DefaultSpace>(spaces["uneven_np"],
                                                get_as<Config>("decimal_res",
                                                               cfg_grids));
        
        auto gdec_shape = gdec_np.shape();

        // Check sizes
        BOOST_TEST(   gdec_np.boundary_cells().size()
                   == 2 * gdec_shape[0] + 2 * gdec_shape[1] - 4);
        BOOST_TEST(   gdec_np.boundary_cells("all").size()
                   == gdec_np.boundary_cells().size());
        BOOST_TEST(   gdec_np.boundary_cells("left").size()
                   == gdec_shape[1]);
        BOOST_TEST(   gdec_np.boundary_cells("right").size()
                   == gdec_shape[1]);
        BOOST_TEST(   gdec_np.boundary_cells("bottom").size()
                   == gdec_shape[0]);
        BOOST_TEST(   gdec_np.boundary_cells("top").size()
                   == gdec_shape[0]);

        // Now check the actual elements for a specific shape
        BOOST_TEST(gdec_shape[0] == 20);
        BOOST_TEST(gdec_shape[1] == 30);

        // Bottom row; as the size is correct, only need to check min and max
        auto bc_bottom = gdec_np.boundary_cells("bottom");
        BOOST_TEST(*bc_bottom.begin() == 0);
        BOOST_TEST(*bc_bottom.rbegin() == 20 - 1);

        // Top row; analogously ...
        auto bc_top = gdec_np.boundary_cells("top");
        BOOST_TEST(*bc_top.begin() == 20 * (30-1));
        BOOST_TEST(*bc_top.rbegin() == (20*30) - 1);

        // Left boundary; sporadic checks should be ok (more transparent even)
        auto bc_left = gdec_np.boundary_cells("left");
        BOOST_TEST(*bc_left.begin() == 0);
        BOOST_TEST(bc_left.count(20));
        BOOST_TEST(bc_left.count(40));
        BOOST_TEST(bc_left.count(300));
        BOOST_TEST(bc_left.count(560));
        BOOST_TEST(*bc_left.rbegin() == 580);

        // Right boundary
        auto bc_right = gdec_np.boundary_cells("right");
        BOOST_TEST(*bc_right.begin() == 20 - 1);
        BOOST_TEST(bc_right.count(39));
        BOOST_TEST(bc_right.count(59));
        BOOST_TEST(bc_right.count(299));
        BOOST_TEST(bc_right.count(539));
        BOOST_TEST(bc_right.count(559));
        BOOST_TEST(*bc_right.rbegin() == 20*30 - 1);

        // All boundary cells
        auto bc_all = gdec_np.boundary_cells("all");
        BOOST_TEST(*bc_all.begin() == 0);
        BOOST_TEST(bc_all.count(1));
        BOOST_TEST(bc_all.count(2));
        BOOST_TEST(bc_all.count(10));
        BOOST_TEST(bc_all.count(19));
        BOOST_TEST(bc_all.count(20));
        BOOST_TEST(bc_all.count(39));
        BOOST_TEST(bc_all.count(40));
        BOOST_TEST(bc_all.count(300));
        BOOST_TEST(bc_all.count(319));
        BOOST_TEST(bc_all.count(560));
        BOOST_TEST(bc_all.count(579));
        BOOST_TEST(bc_all.count(580));
        BOOST_TEST(bc_all.count(581));
        BOOST_TEST(bc_all.count(590));
        BOOST_TEST(bc_all.count(598));
        BOOST_TEST(*bc_all.rbegin() == 20*30 - 1);

        // Test error messages                  
        BOOST_TEST(check_error_message<std::invalid_argument>(
            "invalid boundary cell argument",
            [&](){
                gdec_np.boundary_cells("not a valid argument");
            },
            "Invalid value for argument `select` in call to method",
            "   ", true)
        );
    }

BOOST_AUTO_TEST_SUITE_END()
