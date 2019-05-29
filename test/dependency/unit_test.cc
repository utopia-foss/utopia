#define BOOST_TEST_MODULE unit test
#include <boost/test/unit_test.hpp>

#include <utopia/core/logging.hh>

BOOST_AUTO_TEST_CASE (logging)
{
    auto log = Utopia::init_logger("logger",
                                   spdlog::level::info);
    BOOST_TEST(log);
}
