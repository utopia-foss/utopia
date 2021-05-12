#define BOOST_TEST_MODULE CoreGridHexagonal

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

/// Compares two Armadillo vectors for equality
bool check_eq(SpaceVec v1, SpaceVec v2, double prec) {
    if (v1[0] - v2[0] > prec or v1[1] - v2[1] > prec) {
        std::cerr << "ERROR: The given vectors" << std::endl
                  << v1 << "and" << std::endl << v2 << "are not equal with "
                  << "precision " << prec
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
    for (const auto& space_pair : spaces) {
        const auto space_name = std::get<0>(space_pair);
        const auto& space = std::get<1>(space_pair);
        
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

            HexagonalGrid<DefaultSpace> grid(space, grid_cfg);

            if (((int) grid.num_cells()) != enc) {
                std::cerr << "ERROR: Number of cells did not match! Expected "
                          << enc << " but grid returned " << grid.num_cells()
                          << std::endl;

                return false;
            }

            // Also check the shape
            if (arma::any(grid.shape() != exp_shape)) {
                std::cerr << "ERROR: Shape did not match! Expected:"
                          << std::endl << exp_shape << "But grid returned:"
                          << std::endl << grid.shape() << std::endl;

                return false;
            }

            continue; // ... with the next space
        }

        std::string expected_err_msg;
        if (fail_code == -1) {
            expected_err_msg = "Given the extent of the physical space and "
                               "the specified resolution, a mapping with "
                               "hexagonal cells could not be found!";
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
                [&](){ HexagonalGrid<DefaultSpace> grid(space, grid_cfg); },
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

    return true;
}


struct Fixture {
    Config cfg;
    Config cfg_spaces;
    Config cfg_grids;

    SpaceMap spaces; /// A map of spaces defined in cfg_spaces

    HexagonalGrid<DefaultSpace> g44; /// An even 4x4 grid
    HexagonalGrid<DefaultSpace> g33; /// An uneven 3x3 grid

    const double w_p = 1.0746, h_p = 1.2408; // width and height of a pointy cell
    SpaceVec extent;

    Fixture ()
    :
        cfg(YAML::LoadFile("grid_hexagonal_test.yml")),
        cfg_spaces(get_as<Config>("spaces", cfg)),
        cfg_grids(get_as<Config>("grids", cfg)),
        spaces({
            std::make_pair("default",
                           std::make_shared<DefaultSpace>()),
            std::make_pair("even_pointy",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "even_pointy", cfg_spaces))),
            std::make_pair("even_np_pointy",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "even_np_pointy", cfg_spaces))),
            std::make_pair("uneven_pointy",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "uneven_pointy", cfg_spaces))),
            std::make_pair("uneven_np_pointy",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "uneven_np_pointy", cfg_spaces))),
            std::make_pair("nasty",
                           std::make_shared<DefaultSpace>(get_as<Config>(
                               "nasty", cfg_spaces)))
        }),
        g44(spaces["even_pointy"], get_as<Config>("tiny_res", cfg_grids)),
        g33(spaces["uneven_np_pointy"], get_as<Config>("tiny_res", cfg_grids)),
        // g23_np(SquareGrid<DefaultSpace>(spaces["uneven_np"],
        //                              get_as<Config>("tiny_res", cfg_grids))),
        extent({w_p, h_p})
    { }
};

BOOST_FIXTURE_TEST_SUITE (test_space_extent, Fixture)
    BOOST_AUTO_TEST_CASE(default_space) {
        BOOST_TEST(check_eq(spaces["default"]->extent, SpaceVec({1., 1.})));
    }
    BOOST_AUTO_TEST_CASE(even_pointy) {
        BOOST_TEST(check_eq(spaces["even_pointy"]->extent,
                            SpaceVec({4.2983, 3.7224})));
    }
    BOOST_AUTO_TEST_CASE(even_np_pointy) {
        BOOST_TEST(check_eq(spaces["even_np_pointy"]->extent,
                            SpaceVec({4.2983, 3.7224})));
    }
    BOOST_AUTO_TEST_CASE(uneven_pointy) {
        BOOST_TEST(check_eq(spaces["uneven_pointy"]->extent,
                            SpaceVec({3.2237, 2.7918})));
    }
    BOOST_AUTO_TEST_CASE(uneven_np_pointy) {
        BOOST_TEST(check_eq(spaces["uneven_np_pointy"]->extent,
                            SpaceVec({3.2237, 2.7918})));
    }
    BOOST_AUTO_TEST_CASE(nasty) {
        BOOST_TEST(check_eq(spaces["nasty"]->extent,
                            SpaceVec({1.75, 1.9892})));
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE (test_number_of_cells, Fixture)
    BOOST_AUTO_TEST_CASE(tiny_res) {
        BOOST_TEST(check_num_cells_and_shape("tiny_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(small_res) {
        BOOST_TEST(check_num_cells_and_shape("small_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(small_res_high_tolerance) {
        BOOST_TEST(check_num_cells_and_shape("small_res_high_tolerance", spaces,
                                             cfg));
    }
    BOOST_AUTO_TEST_CASE(decimal_res) {
        BOOST_TEST(check_num_cells_and_shape("decimal_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(medium_res) {
        BOOST_TEST(check_num_cells_and_shape("medium_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(invalid_res) {
        BOOST_TEST(check_num_cells_and_shape("invalid_res", spaces, cfg));
    }
    BOOST_AUTO_TEST_CASE(missing_res) {
        BOOST_TEST(check_num_cells_and_shape("missing_res", spaces, cfg));
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE (test_multi_indices, Fixture)
    BOOST_AUTO_TEST_CASE(even_grid) {
        // Default space, only (0, 0) available
        BOOST_TEST(check_eq(g44.midx_of(0), MultiIndex({0, 0})));
        BOOST_TEST(check_eq(g44.midx_of(1), MultiIndex({1, 0})));
        BOOST_TEST(check_eq(g44.midx_of(2), MultiIndex({2, 0})));
        BOOST_TEST(check_eq(g44.midx_of(3), MultiIndex({3, 0})));
        BOOST_TEST(check_eq(g44.midx_of(4), MultiIndex({0, 1})));
        BOOST_TEST(check_eq(g44.midx_of(15), MultiIndex({3, 3})));

        // ... but NO bounds checking, so this is also computed
        BOOST_TEST(check_eq(g44.midx_of(17), MultiIndex({1, 4})));
    }

    BOOST_AUTO_TEST_CASE(uneven_grid) {
        // For the uneven (3x3) space
        BOOST_TEST(check_eq(g33.midx_of(0), MultiIndex({0, 0})));
        BOOST_TEST(check_eq(g33.midx_of(1), MultiIndex({1, 0})));
        BOOST_TEST(check_eq(g33.midx_of(2), MultiIndex({2, 0})));
        BOOST_TEST(check_eq(g33.midx_of(3), MultiIndex({0, 1})));
        BOOST_TEST(check_eq(g33.midx_of(4), MultiIndex({1, 1})));
        BOOST_TEST(check_eq(g33.midx_of(5), MultiIndex({2, 1})));
        BOOST_TEST(check_eq(g33.midx_of(6), MultiIndex({0, 2})));
        BOOST_TEST(check_eq(g33.midx_of(7), MultiIndex({1, 2})));
        BOOST_TEST(check_eq(g33.midx_of(8), MultiIndex({2, 2})));
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE (test_position_methods, Fixture)
    BOOST_AUTO_TEST_CASE(cell_extent) {
        for (unsigned short i=0; i<9; i++) {
            BOOST_TEST(check_eq(g33.extent_of(i), SpaceVec({w_p, h_p}),
                                1.e-3));
            // NOTE w_p = sqrt(3) * size, h_p = 2 * size, size = 0.6204
        }
    }

    BOOST_AUTO_TEST_CASE(barycenters) {
        BOOST_TEST(check_eq(g33.barycenter_of(0), SpaceVec({1.*w_p, 0.5*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(1), SpaceVec({2*w_p, 0.5*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(2), SpaceVec({3*w_p, 0.5*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(3), SpaceVec({0.5*w_p, 1.25*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(4), SpaceVec({1.5*w_p, 1.25*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(5), SpaceVec({2.5*w_p, 1.25*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(6), SpaceVec({1*w_p, 2.5*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(7), SpaceVec({2*w_p, 2.5*h_p}),
                            1.e-3));
        BOOST_TEST(check_eq(g33.barycenter_of(8), SpaceVec({3*w_p, 2.5*h_p}),
                            1.e-3));
    }

    BOOST_AUTO_TEST_CASE(vertex_positions) {
        auto g33_id4_vtcs = g33.vertices_of(4);
        BOOST_TEST(check_eq(g33_id4_vtcs[0], SpaceVec({1.0, 1.0 }) % extent,
                            1.e-3));
        BOOST_TEST(check_eq(g33_id4_vtcs[1], SpaceVec({1.5, 0.75}) % extent,
                            1.e-3));
        BOOST_TEST(check_eq(g33_id4_vtcs[2], SpaceVec({2.0, 1.0 }) % extent,
                            1.e-3));
        BOOST_TEST(check_eq(g33_id4_vtcs[3], SpaceVec({2.0, 1.5 }) % extent,
                            1.e-3));
        BOOST_TEST(check_eq(g33_id4_vtcs[4], SpaceVec({1.5, 1.75}) % extent,
                            1.e-3));
        BOOST_TEST(check_eq(g33_id4_vtcs[5], SpaceVec({1.0, 1.5 }) % extent,
                            1.e-3));
    }
BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE (test_cell_id_retrieval, Fixture)

    BOOST_AUTO_TEST_CASE(from_positive_position) {
        BOOST_TEST(g44.is_periodic());
        BOOST_TEST(check_eq(g44.space()->extent, SpaceVec({4.2983, 3.7224})));

        // The cell centers
        BOOST_TEST(check_pos(g44, SpaceVec({1. , 0.5 }) % extent,        0));
        BOOST_TEST(check_pos(g44, SpaceVec({2. , 0.5 }) % extent,        1));
        BOOST_TEST(check_pos(g44, SpaceVec({0.5, 1.25}) % extent,        4));
        BOOST_TEST(check_pos(g44, SpaceVec({1.5, 1.25}) % extent,        5));
        BOOST_TEST(check_pos(g44, SpaceVec({1. , 2.  }) % extent,        8));
        BOOST_TEST(check_pos(g44, SpaceVec({2. , 2.  }) % extent,        9));
        BOOST_TEST(check_pos(g44, SpaceVec({3.5, 2.75}) % extent,       15));

        // Within the space, cells of size (1., 1.) % extent
        BOOST_TEST(check_pos(g44, SpaceVec({0.8 , 0.4 }) % extent,       0));
        BOOST_TEST(check_pos(g44, SpaceVec({1.2 , 0.8 }) % extent,       0));
        BOOST_TEST(check_pos(g44, SpaceVec({1.126 , 0.758 }) % extent,   0));

        // Within the space, but in the offset geometry of hexagonal grid
        BOOST_TEST(check_pos(g44, SpaceVec({0.1 , 0.6 }) % extent,       3));
        BOOST_TEST(check_pos(g44, SpaceVec({3.6 , 1.3 }) % extent,       7));
        BOOST_TEST(check_pos(g44, SpaceVec({0.55, 0.125 }) % extent,    12));

        // High-level cell boundaries chosen correctly
        BOOST_TEST(check_pos(g44, SpaceVec({1.49, 0.5}) % extent,         0));
        BOOST_TEST(check_pos(g44, SpaceVec({1.50, 0.5}) % extent,         1));
        BOOST_TEST(check_pos(g44, SpaceVec({0.76, 0.874}) % extent,       0));
        // BOOST_TEST(check_pos(g44, SpaceVec({0.75, 0.875}) % extent, 4));
        BOOST_TEST(check_pos(g44, SpaceVec({0.75, 0.875000001}) % extent, 4));
        // NOTE this is a floating point error

        // High-value space boundaries mapped periodically
        BOOST_TEST(check_pos(g44, SpaceVec({4., 0.5 }) % extent,          3));
        BOOST_TEST(check_pos(g44, SpaceVec({4., 1.25}) % extent,          4));
        
        // Positions out of space mapped back into space
        BOOST_TEST(check_pos(g44, SpaceVec({5. , 0.5 }) % extent,         0));
        BOOST_TEST(check_pos(g44, SpaceVec({4.5, 1.25}) % extent,         4));
        BOOST_TEST(check_pos(g44, SpaceVec({1., 3.5}) % extent,           0));

        // Positions waaaay out of space mapped back properly
        BOOST_TEST(check_pos(g44, SpaceVec({25., 0.5 }) % extent,          0));
        BOOST_TEST(check_pos(g44, SpaceVec({1., 12.5 }) % extent,          0));

        // negative positions are mapped correctly in periodic space
        BOOST_TEST(check_pos(g44, SpaceVec({-3.,  0.5 }) % extent,          0));
        BOOST_TEST(check_pos(g44, SpaceVec({ 1., -2.75}) % extent,          0));
        BOOST_TEST(check_pos(g44, SpaceVec({-3., -2.75}) % extent,          0));
        BOOST_TEST(check_pos(g44, SpaceVec({-1.,-2.75}) % extent,           2));
        BOOST_TEST(check_pos(g44, SpaceVec({-0.25, -0.5}) % extent,        15));
        BOOST_TEST(check_pos(g44, SpaceVec({-24.5, -12.5}) % extent,       15));

        // BOOST_TEST(check_pos(g44, SpaceVec({-0.5, 0.5 }) % extent,       3));
        BOOST_TEST(check_pos(g44, SpaceVec({-0.4999, 0.5 }) % extent,       3));
        // NOTE floating point error
    }

    BOOST_AUTO_TEST_CASE(non_periodic) {
        BOOST_TEST(not g33.is_periodic());
        BOOST_TEST(check_eq(g33.space()->extent, SpaceVec({3.2237, 2.7918})));

        // Within the space, cells of size (1., 1.)
        BOOST_TEST(check_pos(g33, SpaceVec({1. , 0.5 }) % extent,        0));
        BOOST_TEST(check_pos(g33, SpaceVec({2. , 0.5 }) % extent,        1));
        BOOST_TEST(check_pos(g33, SpaceVec({0.5, 1.25}) % extent,        3));
        BOOST_TEST(check_pos(g33, SpaceVec({1.5, 1.25}) % extent,        4));
        BOOST_TEST(check_pos(g33, SpaceVec({1. , 2.  }) % extent,        6));
        BOOST_TEST(check_pos(g33, SpaceVec({2. , 2.  }) % extent,        7));
        BOOST_TEST(check_pos(g33, SpaceVec({2.99, 2. }) % extent,        8));

        // Within the space, cells of size (1., 1.) % extent
        BOOST_TEST(check_pos(g33, SpaceVec({0.8 , 0.4 }) % extent,       0));
        BOOST_TEST(check_pos(g33, SpaceVec({1.2 , 0.8 }) % extent,       0));
        BOOST_TEST(check_pos(g33, SpaceVec({1.126 , 0.758 }) % extent,   0));


        // High-level cell boundaries chosen correctly
        BOOST_TEST(check_pos(g33, SpaceVec({1.49, 0.5}) % extent,         0));
        BOOST_TEST(check_pos(g33, SpaceVec({1.50, 0.5}) % extent,         1));
        BOOST_TEST(check_pos(g33, SpaceVec({0.76, 0.874}) % extent,       0));
        // BOOST_TEST(check_pos(g33, SpaceVec({0.75, 0.875}) % extent,    3));
        BOOST_TEST(check_pos(g33, SpaceVec({0.75, 0.875000001}) % extent, 3));
        // NOTE this is a floating point error

        // High-value space boundaries mapped periodically
        BOOST_TEST(check_pos(g33, SpaceVec({2.999, 0.5 }) % extent,         2));
        // BOOST_TEST(check_pos(g33, SpaceVec({3.   , 0.5 }) % extent,         2));
        BOOST_TEST(check_pos(g33, SpaceVec({2.999, 1.25}) % extent,         5));
        // BOOST_TEST(check_pos(g33, SpaceVec({3.   , 1.25}) % extent,         5));
        BOOST_TEST(check_pos(g33, SpaceVec({1.,    2.25}) % extent,         6));

        // Within the space, but in the offset geometry of hexagonal grid
        BOOST_TEST(check_pos(g33, SpaceVec({0.5 , 0.125 }) % extent,        0));
        BOOST_TEST(check_pos(g33, SpaceVec({0.1 , 0.5   }) % extent,        0));
        BOOST_TEST(check_pos(g33, SpaceVec({1.5 , 0.125 }) % extent,        1));
        BOOST_TEST(check_pos(g33, SpaceVec({0.1 , 2.    }) % extent,        6));

        // Querying a position outside the space yields an error
        BOOST_TEST(check_error_message<std::invalid_argument>(
            "position query outside of space (for both arguments)",
            [&](){
                g33.cell_at(SpaceVec({3.0001, 2.2501}) % extent);
            },
            "given position is outside the non-periodic space",
            "   ", true)
        );
        BOOST_TEST(check_error_message<std::invalid_argument>(
            "position query outside of space (for single argument)",
            [&](){
                g33.cell_at(SpaceVec({-0.0001, +0.0001}) % extent);
            },
            "given position is outside the non-periodic space",
            "   ", true)
        );
        BOOST_TEST(check_error_message<std::invalid_argument>(
            "position query outside of space (for single argument)",
            [&](){
                g33.cell_at(SpaceVec({+0.0001, -0.0001}) % extent);
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
        auto gdec_p = HexagonalGrid<DefaultSpace>(spaces["even_pointy"],
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
        auto gdec_np = HexagonalGrid<DefaultSpace>(spaces["even_np_pointy"],
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
        BOOST_TEST(gdec_shape[0] == 40);
        BOOST_TEST(gdec_shape[1] == 40);

        // Bottom row; as the size is correct, only need to check min and max
        auto bc_bottom = gdec_np.boundary_cells("bottom");
        BOOST_TEST(*bc_bottom.begin() == 0);
        BOOST_TEST(*bc_bottom.rbegin() == 40 - 1);

        // Top row; analogously ...
        auto bc_top = gdec_np.boundary_cells("top");
        BOOST_TEST(*bc_top.begin() == 40 * (40-1));
        BOOST_TEST(*bc_top.rbegin() == (40*40) - 1);

        // Left boundary; sporadic checks should be ok (more transparent even)
        auto bc_left = gdec_np.boundary_cells("left");
        BOOST_TEST(*bc_left.begin() == 0);
        BOOST_TEST(bc_left.count(40));
        BOOST_TEST(bc_left.count(80));
        BOOST_TEST(bc_left.count(400));
        BOOST_TEST(bc_left.count(560));
        BOOST_TEST(*bc_left.rbegin() == 1560);

        // Right boundary
        auto bc_right = gdec_np.boundary_cells("right");
        BOOST_TEST(*bc_right.begin() == 40 - 1);
        BOOST_TEST(bc_right.count(79));
        BOOST_TEST(bc_right.count(119));
        BOOST_TEST(bc_right.count(399));
        BOOST_TEST(bc_right.count(559));
        BOOST_TEST(*bc_right.rbegin() == 40*40 - 1);

        // All boundary cells
        auto bc_all = gdec_np.boundary_cells("all");
        BOOST_TEST(*bc_all.begin() == 0);
        BOOST_TEST(bc_all.count(1));
        BOOST_TEST(bc_all.count(2));
        BOOST_TEST(bc_all.count(10));
        BOOST_TEST(bc_all.count(39));
        BOOST_TEST(bc_all.count(40));
        BOOST_TEST(bc_all.count(79));
        BOOST_TEST(bc_all.count(80));
        BOOST_TEST(bc_all.count(400));
        BOOST_TEST(bc_all.count(439));
        BOOST_TEST(bc_all.count(560));
        BOOST_TEST(bc_all.count(599));
        BOOST_TEST(bc_all.count(1560));
        BOOST_TEST(bc_all.count(1561));
        BOOST_TEST(bc_all.count(1580));
        BOOST_TEST(bc_all.count(1588));
        BOOST_TEST(*bc_all.rbegin() == 40*40 - 1);

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
