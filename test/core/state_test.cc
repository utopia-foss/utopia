#define BOOST_TEST_MODULE state test
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <vector>

#include <utopia/core/state.hh>

struct StateVector {
    using StateType = std::vector<double>;
    StateType state_1 = {0.1, 0.2};
    StateType state_2 = {-0.1, 0.3};
};

struct StateScalar {
    using StateType = double;
    StateType state_1 = 0.1;
    StateType state_2 = -0.2;
};

using StateList = boost::mpl::list<StateScalar, StateVector>;

/// Test a asynchronous state
BOOST_FIXTURE_TEST_CASE_TEMPLATE(asynchronous, Fix, StateList, Fix)
{
    // Check initialization
    StateContainer<typename Fix::StateType, false> sc(Fix::state_1);
    BOOST_TEST(not sc.is_sync());
    BOOST_TEST(sc.state() == Fix::state_1);

    // Check direct update
    auto& state = sc.state();
    state = Fix::state_2;
    BOOST_TEST(sc.state() == Fix::state_2);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(synchronous, Fix, StateList, Fix)
{
    // Check initialization
    StateContainer<typename Fix::StateType, true> sc(Fix::state_1);
    BOOST_TEST(sc.is_sync());
    BOOST_TEST(sc.state() == Fix::state_1);

    // Check independent update of the cache
    auto& state_new = sc.state_new();
    state_new = Fix::state_2;
    BOOST_TEST(sc.state() == Fix::state_1);
    BOOST_TEST(sc.state_new() == Fix::state_2);

    // Check update of the state
    sc.update();
    BOOST_TEST(sc.state() == Fix::state_2);
}
