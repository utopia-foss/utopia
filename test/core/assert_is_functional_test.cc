#define BOOST_TEST_MODULE assert functional test

#include <cassert>

#include <boost/test/included/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>  // for execution_exception


void impossible_assert() {
    assert(false);
}

BOOST_AUTO_TEST_CASE(assert_is_functional)
{
    BOOST_CHECK_THROW(
        boost::execution_monitor().vexecute(impossible_assert),
        boost::execution_exception
    );

    // Provide a meaningful error message in case the test fails
    namespace ut = boost::unit_test;
    auto test_id = ut::framework::current_test_case().p_id;
    BOOST_TEST(
        ut::results_collector.results(test_id).passed(),
        "The assert function seems not to be functional, rendering some other "
        "tests useless! Make sure to set the CMake Build Type to 'Debug' "
        "before building the tests."
    );
}


BOOST_AUTO_TEST_CASE(ndebug_not_set)
{
#ifndef NDEBUG
    // NDEBUG is _not_ set -> assert is functional -> all good, yay.
    BOOST_TEST(true);
#else
    BOOST_FAIL(
        "The NDEBUG flag IS set -> assert will NOT be functional, rendering "
        "some other tests useless! Make sure to set the CMake Build Type to "
        "'Debug' before building the tests."
    );
#endif
}
