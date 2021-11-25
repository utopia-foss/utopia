#define BOOST_TEST_MODULE movement test

#include <cmath>
#include <random>

#include <boost/test/unit_test.hpp>
#include <utopia/core/model.hh>
#include <utopia/core/testtools.hh>

#include "../SimpleFlocking.hh"
#include "../utils.hh"

namespace Utopia::Models::SimpleFlocking {

using namespace Utopia::TestTools;
using namespace Utopia::Utils;


// -- Type definitions --------------------------------------------------------


// -- Fixtures ----------------------------------------------------------------

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("test_movement.yml") {};
};



// -- Test general movement-related functions ---------------------------------
BOOST_AUTO_TEST_SUITE(test_movement_utils);

/// Assert the basic working of the regularisation function for angles
/** Values should always be in [-π, +π)
 */
BOOST_AUTO_TEST_CASE(test_constrain_angle, *utf::tolerance(1.e-12))
{
    BOOST_TEST(constrain_angle(0.) == 0.);
    BOOST_TEST(constrain_angle(+1.) == +1.);
    BOOST_TEST(constrain_angle(-1.) == -1.);
    BOOST_TEST(constrain_angle(+M_PI) == -M_PI);
    BOOST_TEST(constrain_angle(-M_PI) == -M_PI);
    BOOST_TEST(constrain_angle(+3*M_PI) == -M_PI);
    BOOST_TEST(constrain_angle(-3*M_PI) == -M_PI);

    BOOST_TEST(constrain_angle(+2*M_PI)  == 0.);
    BOOST_TEST(constrain_angle(+4*M_PI)  == 0.);
    BOOST_TEST(constrain_angle(+40*M_PI) == 0.);

    BOOST_TEST(constrain_angle(-2*M_PI)  == 0.);
    BOOST_TEST(constrain_angle(-4*M_PI)  == 0.);
    BOOST_TEST(constrain_angle(-40*M_PI) == 0.);

    BOOST_TEST(constrain_angle(+M_PI + 1.) == -M_PI + 1.);
    BOOST_TEST(constrain_angle(-M_PI - 1.) == +M_PI - 1.);
}

/// Assert that there is no bias in the random angle function
/** This is not to test the properties of uniform_real_distribution or the RNG
 *  but of the hard-coded interval in the random_angle function, which
 *  should for consistency's sake be symmetric around zero.
 */
BOOST_FIXTURE_TEST_CASE(test_random_angle, Infrastructure)
{
    auto agg_angle = 0.;
    auto val = 0.;
    auto N = 100000u;

    for (auto i = 0u; i < N; i++) {
        val = random_angle(rng);
        BOOST_TEST(val <  +M_PI);
        BOOST_TEST(val >= -M_PI);
        agg_angle += val;
    }

    BOOST_TEST(std::abs(agg_angle/N) < 0.02);
}

BOOST_AUTO_TEST_SUITE_END();



// -- Test general movement-related functions ---------------------------------
BOOST_AUTO_TEST_SUITE(test_movement_rules);

BOOST_AUTO_TEST_SUITE_END();


} // model namespace
