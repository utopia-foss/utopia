#define BOOST_TEST_MODULE space test

#include <iostream>
#include <tuple>

#include <boost/test/unit_test.hpp>
#include <utopia/core/testtools.hh>
#include <utopia/core/space.hh>

namespace Utopia {

using namespace Utopia::TestTools;

constexpr double precision = 1e-12;
using SpaceTypes = std::tuple<Space<1>, Space<2>, Space<3>, Space<5>>;

// -- Fixtures ----------------------------------------------------------------

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("space_test.yml") {};
};


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

BOOST_FIXTURE_TEST_CASE(test_space_basics, Infrastructure) {
    auto space = Space<1>(cfg["1D"]["simple"]);
    auto space_periodic = Space<1>(cfg["1D"]["simple_periodic"]);
    auto space_2d_periodic = Space<2>(cfg["2D"]["simple_periodic"]);

    BOOST_TEST(space.contains<false>({0.1}));
    BOOST_TEST(not space.contains<false>({1.1}));

    BOOST_CHECK_CLOSE(space_periodic.map_into_space({2.1}).at(0), 0.1,
                      precision);

    BOOST_CHECK_CLOSE(space.displacement({0.1}, {0.3}).at(0), 0.2, precision);
    BOOST_CHECK_CLOSE(space.displacement({0.1}, {0.9}).at(0), 0.8, precision);
    
    BOOST_CHECK_CLOSE(space_periodic.displacement({0.1}, {0.3}).at(0), 0.2,
                      precision);
    BOOST_CHECK_CLOSE(space_periodic.displacement({0.1}, {0.9}).at(0), -0.2,
                      precision);
    
    BOOST_CHECK_CLOSE(space.distance({0.1}, {0.3}), 0.2, precision);
    BOOST_CHECK_CLOSE(space_periodic.distance({0.1}, {0.9}), 0.2, precision);

    BOOST_CHECK_CLOSE(space_2d_periodic.displacement({0., 0.1}, 
                                                     {0., 1.2}).at(1), -0.9,
                      precision);
}


/// Test that setup of spaces with different extent works
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_space_extent, Space, SpaceTypes,
                                 Infrastructure)
{
    test_config_callable(
        [](const auto params){
            auto space = Space(params["space"]);

            const auto expected_extent =
                get_as_SpaceVec<Space::dim>("expected_extent", params);
            BOOST_TEST(space.extent == expected_extent, tt::per_element());
        },
        cfg["extent"][Space::dim],
        "Test cases with different extent and dimensionality",
        {__LINE__, __FILE__}
    );
}


}
