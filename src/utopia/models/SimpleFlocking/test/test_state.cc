#define BOOST_TEST_MODULE state test

#include <cmath>

#include <boost/test/unit_test.hpp>
#include <utopia/core/model.hh>
#include <utopia/core/testtools.hh>

#include "../state.hh"

namespace Utopia::Models::SimpleFlocking {

using namespace Utopia::TestTools;
using namespace Utopia::Utils;


// -- Type definitions --------------------------------------------------------


// -- Fixtures ----------------------------------------------------------------

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("test_state.yml") {};
};



// -- Tests -------------------------------------------------------------------

/// Test the AgentState interface
BOOST_FIXTURE_TEST_CASE(test_state_interface, Infrastructure,
                        *utf::tolerance(1.e-12))
{
    // Default construction
    auto state = AgentState();
    BOOST_TEST(state.get_speed() == 0.);
    BOOST_TEST(state.get_orientation() == 0.);
    BOOST_TEST(arma::norm(state.get_displacement(), 2) == 0.);

    // Config-based construction and assignment
    auto agent_cfg = cfg["agent_state"];

    state = AgentState(agent_cfg, rng);
    BOOST_TEST(state.get_speed() == get_as<double>("speed", agent_cfg));
    BOOST_TEST(state.get_orientation() != 0.);  // random value
    BOOST_TEST(arma::norm(state.get_displacement(), 2) != 0.);

    // Setting speed or orientation updates the displacement vector
    auto old_displacement = state.get_displacement();
    state.set_speed(0.);
    BOOST_TEST(arma::norm(state.get_displacement(), 2) == 0.);
    state.set_orientation(0.);
    BOOST_TEST(arma::norm(state.get_displacement(), 2) == 0.);

    state.set_speed(23.);
    BOOST_TEST(arma::norm(state.get_displacement(), 2) != 0.);

    // Displacement vector is normalized, but scaled with speed
    BOOST_TEST(arma::norm(state.get_displacement(), 2) == 23.);

    // Construction also works without speed specified
    state = AgentState(agent_cfg["i_do_not_exist"], rng);
    BOOST_TEST(state.get_speed() == 0.);
}


/// Check angles are used according to convention
BOOST_FIXTURE_TEST_CASE(test_state_angles, Infrastructure,
                        *utf::tolerance(1.e-12))
{
    auto state = AgentState(cfg["agent_state"], rng);
    state.set_speed(1.);

    // Zero: Movement in +x direction
    state.set_orientation(0.);
    BOOST_TEST(state.get_displacement()[0] == 1.);
    BOOST_TEST(state.get_displacement()[1] == 0.);

    // ± π/2: movement in ±y direction
    state.set_orientation(+M_PI/2.);
    BOOST_TEST(state.get_displacement()[0] == 0.);
    BOOST_TEST(state.get_displacement()[1] == +1.);

    state.set_orientation(-M_PI/2.);
    BOOST_TEST(state.get_displacement()[0] == 0.);
    BOOST_TEST(state.get_displacement()[1] == -1.);

    // ±M_PI: Movement in -x direction
    state.set_orientation(+M_PI);
    BOOST_TEST(state.get_displacement()[0] == -1.);
    BOOST_TEST(state.get_displacement()[1] == 0.);

    state.set_orientation(-M_PI);
    BOOST_TEST(state.get_displacement()[0] == -1.);
    BOOST_TEST(state.get_displacement()[1] == 0.);
}

} // model namespace
