#define BOOST_TEST_MODULE parallel stl test
#include <boost/test/included/unit_test.hpp> // for unit tests
#include <boost/test/data/test_case.hpp>

// NOTE: Can (and should!) be performend independently from HAVE_XXX_PSTL
//       using DISABLE_HAVE_PARALLEL_STL macro
#ifdef DISABLE_HAVE_PARALLEL_STL
#undef USE_INTERNAL_PSTL
#undef HAVE_EXTERNAL_PSTL
#endif

#include <utopia/core/parallel.hh>

#include "parallel_fixtures.hh"

// --- Test STL Algorithm Overloads for all Execution Policies --- //

/// Test copy
BOOST_DATA_TEST_CASE_F(
    vectors,
    copy,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::copy(policy, begin(from), end(from), begin(to));
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test for_each
BOOST_DATA_TEST_CASE_F(
    vectors,
    for_each,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::for_each(policy, begin(from), end(from), [](auto& val) { val = 1.0; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test unary transform
BOOST_DATA_TEST_CASE_F(
    vectors,
    transform_1,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::transform(
      policy, begin(from), end(from), begin(from), [](auto&&) { return 1.0; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}

/// Test binary transform
BOOST_DATA_TEST_CASE_F(
    vectors,
    transform_2,
    boost::unit_test::data::make({ Utopia::ExecPolicy::seq,
                                   Utopia::ExecPolicy::unseq,
                                   Utopia::ExecPolicy::par,
                                   Utopia::ExecPolicy::par_unseq }),
    policy)
{
    std::transform(policy,
                   begin(from),
                   end(from),
                   begin(to),
                   begin(from),
                   [](auto&& lhs, auto&& rhs) { return lhs + rhs; });
    BOOST_TEST(from == to, boost::test_tools::per_element());
}
