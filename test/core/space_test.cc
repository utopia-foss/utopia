#define BOOST_TEST_MODULE space test

#include <assert.h>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <utopia/core/space.hh>


using namespace Utopia;

const double precision = 1e-12;

// -- Fixtures ----------------------------------------------------------------

struct Fixture {
    DataIO::Config cfg;
    Space<1> space;
    Space<1> space_periodic;

    Space<2> space_2d_periodic;

    Fixture ()
    :
        cfg(YAML::LoadFile("space_test.yml")),
        space(cfg["1D"]["simple"]),
        space_periodic(cfg["1D"]["simple_periodic"]),
        space_2d_periodic(cfg["2D"]["simple_periodic"])
    {}
};

// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOST_FIXTURE_TEST_CASE(test_space, Fixture)
{
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