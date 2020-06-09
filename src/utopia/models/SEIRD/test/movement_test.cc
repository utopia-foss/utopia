#define BOOST_TEST_MODULE SEIRD movement test

#include <functional>
#include <boost/test/unit_test.hpp>

#include <utopia/core/graph/creation.hh>
#include "../movement.hh"

using namespace Utopia;
using namespace Utopia::Models::SEIRD;

// -- Fixtures ----------------------------------------------------------------

// Create a test model fixture
struct TestModelFixture
{};

// -- Actual test -------------------------------------------------------------

/// Test the movement function
BOOST_FIXTURE_TEST_CASE(movement, TestModelFixture) {}
