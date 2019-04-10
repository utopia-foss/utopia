#define BOOST_TEST_MODULE agent test
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <iostream>
#include <tuple>

#include <armadillo>
#include <utopia/core/agent.hh>

/// Custom state type for this test
struct AgentState {
    int foo;
};

/// Comparison operator for the custom state
bool operator== (const AgentState& lhs, const AgentState& rhs)
{
    return lhs.foo == rhs.foo;
}

/// Tell Boost how to print the custom state
/** The signature of this function is fixed: One may only manipulate the type
 *  of argument `right`.
 */
std::ostream& boost_test_print_type (std::ostream& ostr,
                                     AgentState const& right)
{
    ostr << right.foo;
    return ostr;
}


using namespace Utopia;
using SpaceVec = typename DefaultSpace::SpaceVec;

/// Initial condition for agents in all tests
struct InitialCondition
{
    SpaceVec pos = {4.2, 0.0};
    AgentState state = {42};
    IndexType index = 0;
};

/// Start a test suite with InitialCondition as fixture for all tests.
/** Public and protected members of the fixture are directly available inside
 *  the test cases.
 */
BOOST_FIXTURE_TEST_SUITE(initialization, InitialCondition)

using AgentTraitsSync = AgentTraits<AgentState, Update::sync>;
using AgentTraitsAsync = AgentTraits<AgentState, Update::async>;
using AgentTraitsManual = AgentTraits<AgentState, Update::manual>;

/// Define the types of agents used in a template test function
using AgentTypes = boost::mpl::list<Agent<AgentTraitsSync, DefaultSpace>,
                                    Agent<AgentTraitsAsync, DefaultSpace>>;

/// Check initialization of all Agent types.
/** Templated test function. Will be executed for all `AgentTypes`.
 *  The single type used within every function is called `ThisAgent`.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE (initialize, ThisAgent, AgentTypes)
{
    ThisAgent agent(index, state, pos);
    BOOST_TEST(agent.state() == state);
    BOOST_TEST(agent.id() == index);
    BOOST_TEST(arma::approx_equal(agent.position(), pos, "absdiff", 0.0));
}

/// Check initialization of a synchronous agent
BOOST_AUTO_TEST_CASE (synchronous)
{
    Agent<AgentTraitsSync, DefaultSpace> agent(index, state, pos);
    BOOST_TEST(arma::approx_equal(agent.position_new(), pos, "absdiff", 0.0));
}

/// Check initialization of a agent with Update::manual
BOOST_AUTO_TEST_CASE (manual)
{
    Agent<AgentTraitsManual, DefaultSpace> agent(index, state, pos);
    BOOST_TEST(agent.state == state);
    BOOST_TEST(agent.id() == index);
    BOOST_TEST(arma::approx_equal(agent.position(), pos, "absdiff", 0.0));
}

/// Let suite end here
BOOST_AUTO_TEST_SUITE_END()
