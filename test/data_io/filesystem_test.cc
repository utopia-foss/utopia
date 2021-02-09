#define BOOST_TEST_MODULE test filesystem

#include <cstdlib>
#include <string>
#include <filesystem>

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <utopia/data_io/filesystem.hh>
#include <utopia/core/testtools.hh>


using namespace Utopia;
using namespace Utopia::DataIO;
using namespace Utopia::TestTools;


// -- Fixtures ----------------------------------------------------------------

/// The specialized infrastructure fixture
struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure () : BaseInfrastructure<>("filesystem_test.yml") {}
};


// -- Test cases --------------------------------------------------------------

/// Test expansion of user directory
BOOST_AUTO_TEST_CASE (test_expanduser) {
    using namespace std::string_literals;

    BOOST_TEST(expanduser("foo/bar") == "foo/bar");
    BOOST_TEST(expanduser("/foo/bar") == "/foo/bar");
    BOOST_TEST(expanduser("/foo/bar/") == "/foo/bar/");

    const std::string HOME = std::getenv("HOME");
    BOOST_TEST(expanduser("~") == HOME);
    BOOST_TEST(expanduser("~/") == HOME + "/"s);
    BOOST_TEST(expanduser("~/foo/bar") == HOME + "/foo/bar"s);
}


BOOST_FIXTURE_TEST_SUITE(with_cfg, Infrastructure)

/// Test generation of absolute file path from configuration
BOOST_AUTO_TEST_CASE (test_get_abs_filepath) {
    test_config_callable([](const auto& cfg){
        const auto actual = get_abs_filepath(get_as<Config>("input", cfg));

        if (not get_as<bool>("relative_to_cwd", cfg, false)) {
            BOOST_TEST(
                actual == expanduser(get_as<std::string>("expected", cfg, ""))
            );
        }
        else {
            auto cwd = std::filesystem::current_path();
            BOOST_TEST(
                actual == cwd.append(get_as<std::string>("expected", cfg, ""))
            );
        }
    }, cfg["get_abs_filepath"], "get_abs_filepath", {__LINE__, __FILE__});
}

BOOST_AUTO_TEST_SUITE_END() // with cfg
