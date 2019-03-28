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
    Utopia::StateContainer<typename Fix::StateType,
                           Utopia::Update::async> sc(Fix::state_1);
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
    Utopia::StateContainer<typename Fix::StateType,
                           Utopia::Update::sync> sc(Fix::state_1);
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(manual, Fix, StateList, Fix)
{
    // Check initialization
    using StateContainer = Utopia::StateContainer<typename Fix::StateType,
                                                  Utopia::Update::manual>;

    // value init
    StateContainer sc1({Fix::state_1});
    BOOST_TEST(sc1.state == Fix::state_1);

    // copy init
    StateContainer sc2(sc1);
    BOOST_TEST(sc1.state == sc2.state);

    // move init
    StateContainer sc3(std::move(sc1));
    BOOST_TEST(sc2.state == sc3.state);

    // State manipulation
    sc3.state = Fix::state_2;
    BOOST_TEST(sc3.state != sc2.state);
}
