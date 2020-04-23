#ifndef UTOPIA_CORE_TESTTOOLS_HH
#define UTOPIA_CORE_TESTTOOLS_HH

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

#include "../data_io/cfg_utils.hh"

#include "testtools/config.hh"
#include "testtools/exceptions.hh"
#include "testtools/fixtures.hh"
#include "testtools/utils.hh"


namespace Utopia::TestTools {

/// Shortcut for the BOOST unit test framework namespace
namespace utf = boost::unit_test;

/// Shortcut for the BOOST test_tools namespace
namespace tt = boost::test_tools;

/// Make Utopia::get_as directly available
using Utopia::get_as;


} // namespace Utopia::TestTools

#endif // UTOPIA_CORE_TESTTOOLS_HH
