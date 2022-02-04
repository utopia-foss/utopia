#define BOOST_TEST_MODULE utils test

#include <cmath>
#include <random>
#include <vector>

#include <armadillo>
#include <boost/test/unit_test.hpp>

#include <utopia/core/model.hh>
#include <utopia/core/testtools.hh>

#include "../SimpleFlocking.hh"
#include "../utils.hh"

namespace Utopia::Models::SimpleFlocking {

using namespace Utopia::TestTools;
using namespace Utopia::Utils;


// -- Type definitions and constants ------------------------------------------



// -- Fixtures ----------------------------------------------------------------

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("test_utils.yml") {};
};



// -- Test angle-related functions --------------------------------------------
BOOST_AUTO_TEST_SUITE(test_angles);

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




// -- Test geometry-related functions -----------------------------------------
BOOST_AUTO_TEST_SUITE(test_geometry);


BOOST_AUTO_TEST_CASE(test_absolute_group_velocity)
{
    using Vec = arma::Col<double>;
    using VecCont = std::vector<Vec>;

    const auto zero = Vec(2, arma::fill::zeros);
    const auto one = Vec(2, arma::fill::ones);

    // Empty container yields NaN
    BOOST_TEST(std::isnan(absolute_group_velocity(VecCont{})));

    // Zero-sum group velocities
    BOOST_TEST(
        absolute_group_velocity(VecCont({zero, zero, zero})) == 0.
    );
    BOOST_TEST(
        absolute_group_velocity(VecCont({zero, -1*one, +1*one})) == 0.
    );

    // Non-zero sum group velocities
    BOOST_TEST(
        absolute_group_velocity(VecCont({one})) == std::sqrt(2.),
        tt::tolerance(1.e-10)
    );
}

BOOST_AUTO_TEST_SUITE_END();





// -- Test circular statistic functions ---------------------------------------
BOOST_AUTO_TEST_SUITE(test_circular_stats);

/// Tests the circular_mean function
BOOST_AUTO_TEST_CASE(test_circular_mean)
{
    const double pi = M_PI;

    // Mean at zero
    BOOST_TEST(circular_mean({pi/2, -pi/2}) == 0.);
    BOOST_TEST(circular_mean({pi/2, -pi/2, 0., -1., +1.}) == 0.);

    // Cone of angles not crossing the discontinuity
    BOOST_TEST(circular_mean({1, 1, 1, 2, 0}) == +1);
    BOOST_TEST(circular_mean({-1, -1, -1, -2, -0}) == -1);

    // Check for mean value near or at discontinuity (at ±π)
    BOOST_TEST(circular_mean({.5*pi, -.5*pi}) == 0.);
    BOOST_TEST(circular_mean({.5001*pi, -.5001*pi}) == -pi);
    BOOST_TEST(circular_mean({.9*pi, -.9*pi}) == -pi);

    // No values: will return NaN
    BOOST_TEST(std::isnan(circular_mean({})));
}


/// Tests the circular_mean_and_std function
BOOST_AUTO_TEST_CASE(test_circular_mean_and_std)
{
    const double pi = M_PI;
    auto circ_mean = [](const std::vector<double>& angles){
        return circular_mean_and_std(angles).first;
    };
    auto circ_std = [](const std::vector<double>& angles){
        return circular_mean_and_std(angles).second;
    };

    // No values: will return NaN
    BOOST_TEST(std::isnan(circ_mean({})));
    BOOST_TEST(std::isnan(circ_std({})));

    // Mean is same as in separate function
    BOOST_TEST(circ_mean({1, 1, 1, 2, 0}) == +1);
    BOOST_TEST(circ_mean({.5*pi, -.5*pi}) == 0.);
    BOOST_TEST(circ_mean({.9*pi, -.9*pi}) == -pi);

    // Std. dev. of values distributed near the center of domain, not crossing
    // the discontinuity
    BOOST_TEST(circ_std({0., 0., 0.}) == 0.);
    BOOST_TEST(circ_std({1., 1., 1.}) == 0.);
    BOOST_TEST(circ_std({-1., -1., -1.}) == 0.);

    BOOST_TEST(
        circ_std({-1., 0., 1.}) == 0.855515936, tt::tolerance(1.e-9)
    );
    BOOST_TEST(
        circ_std({0, 0.1*pi/2, 0.001*pi, 0.03*pi/2}) ==
        0.063564063306, tt::tolerance(1.e-9)  // result from scipy example
    );

    // Angles near and crossing the discontinuity (at ±π)
    BOOST_TEST(
        circ_std({+pi-1., +pi, +pi+1.}) == 0.855515936, tt::tolerance(1.e-9)
    );
    BOOST_TEST(
        circ_std({-pi-1., -5*pi, -pi+1.}) == 0.855515936, tt::tolerance(1.e-9)
    );

    BOOST_TEST(
        circ_std({+pi, -pi+0.1*pi/2, -pi+0.001*pi, -pi+0.03*pi/2}) ==
        0.063564063306, tt::tolerance(1.e-10)  // result from scipy example
    );
    BOOST_TEST(
        circ_std({-pi, -3*pi+0.1*pi/2, +pi+0.001*pi, +5*pi+0.03*pi/2}) ==
        0.063564063306, tt::tolerance(1.e-10)  // result from scipy example
    );
}

BOOST_AUTO_TEST_SUITE_END();

} // model namespace
