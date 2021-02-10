#define BOOST_TEST_MODULE test filesystem

#include <boost/test/unit_test.hpp>

#include <utopia/data_io/cfg_utils.hh>
#include <utopia/core/testtools.hh>


using namespace Utopia;
using namespace Utopia::DataIO;
using namespace Utopia::TestTools;


// -- Fixtures ----------------------------------------------------------------

/// The specialized infrastructure fixture
struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure () : BaseInfrastructure<>("cfg_utils_test.yml") {}
};


// -- Test cases --------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(with_infrastructure, Infrastructure)

/// Test the get_as method
BOOST_AUTO_TEST_CASE(test_get_as) {
    // String access
    BOOST_TEST(get_as<std::string>("foo", cfg) == "bar");
    BOOST_TEST(get_as<std::string>("spam", cfg) == "eggs");

    // Double, bool, int
    BOOST_TEST(get_as<double>("a_double", cfg) == 3.14159);
    BOOST_TEST(get_as<bool>("a_bool", cfg));
    BOOST_TEST(get_as<int>("an_int", cfg) == 42);

    // vector
    std::vector vec({1, 2, 3});
    BOOST_TEST(get_as<std::vector<int>>("a_vector", cfg) == vec);

    // array
    using Arr = std::array<std::array<int, 2>, 2>;
    const auto actual = get_as<Arr>("an_array", cfg);
    BOOST_TEST(actual == Arr({{{{1, 2}}, {{3, 4}}}}));

    // Optional value returned if KeyError was raised
    BOOST_TEST(get_as<std::string>("foo", cfg, "foo") == "bar"); // foo exists!
    BOOST_TEST(get_as<std::string>("not_a_key", cfg, "foo") == "foo");

    // armadillo vector
    auto sv1 = get_as_SpaceVec<3>("a_vector", cfg);
    BOOST_TEST(sv1[0] == 1.);
    BOOST_TEST(sv1[1] == 2.);
    BOOST_TEST(sv1[2] == 3.);

    auto mi1 = get_as_MultiIndex<3>("a_vector", cfg);
    BOOST_TEST(mi1[0] == 1);
    BOOST_TEST(mi1[1] == 2);
    BOOST_TEST(mi1[2] == 3);

    std::cout << "Success." << std::endl << std::endl;
}

/// Tests the (partly: custom) exceptions thrown when reading keys
BOOST_AUTO_TEST_CASE(test_exceptions) {
    // Key missing
    check_exception<Utopia::KeyError>(
        [&](){
            get_as<bool>("i_do_not_exist", cfg);
        },
        "The content of the given node is"
    );

    // Empty node
    check_exception<Utopia::KeyError>(
        [&](){
            get_as<bool>("some_key", cfg["empty_map"]);
        },
        "The given node contains no entries!"
    );

    // Zombie nodes are handled by yaml-cpp
    check_exception<YAML::InvalidNode>(
        [&](){
            get_as<bool>("invalid_key2", cfg["invalid_key1"]);
        },
        "invalid node; first invalid key: \"invalid_key1\""
    );

    // Conversion error is still thrown, not intercepted
    check_exception<YAML::Exception>(
        [&](){
            get_as<double>("foo", cfg);
        },
        "Got YAML::TypedBadConversion<double>"
    );

    // Conversion error still thrown, even with default given
    check_exception<YAML::Exception>(
        [&](){
            get_as<double>("foo", cfg, 3.14);
        },
        "Got YAML::TypedBadConversion<double>"
    );
}


/// Tests recursive_getitem
BOOST_AUTO_TEST_CASE (test_recursive_getitem) {
    const Config b = YAML::Clone(cfg["recursive_getitem"]["basics"]);

    BOOST_TEST(recursive_getitem(b, "lvl").as<int>() == 0);
    BOOST_TEST(recursive_getitem(b, "deeper.lvl").as<int>() == 1);
    BOOST_TEST(recursive_getitem(b, "deeper.deeper.lvl").as<int>() == 2);
    BOOST_TEST(recursive_getitem(b, "deeper.deeper.deeper.lvl").as<int>() ==3);

    check_exception<Utopia::KeyError>(
        [&](){
            recursive_getitem(b, "deeper.deeper.bad_key.foo");
        },
        "failed for key or key sequence 'deeper -> deeper -> bad_key -> foo'",
        {__LINE__, __FILE__}
    );
}

/// Tests recursive_setitem
BOOST_AUTO_TEST_CASE (test_recursive_setitem) {
    Config b = YAML::Clone(cfg["recursive_setitem"]["basics"]);

    BOOST_TEST(b["val"].as<int>() == 0);
    BOOST_TEST(b["deeper"]["val"].as<int>() == 1);
    BOOST_TEST(b["deeper"]["deeper"]["val"].as<int>() == 2);
    BOOST_TEST(b["deeper"]["deeper"]["deeper"]["val"].as<int>() == 3);

    recursive_setitem(b, "val", 42);
    BOOST_TEST(b["val"].as<int>() == 42);

    recursive_setitem(b, "deeper.val", 43);
    BOOST_TEST(b["deeper"]["val"].as<int>() == 43);

    recursive_setitem(b, "deeper.deeper.val", "44");
    BOOST_TEST(b["deeper"]["deeper"]["val"].as<std::string>() == "44");

    recursive_setitem(b, "some.new.val", 6.4);
    BOOST_TEST(b["some"]["new"]["val"].as<double>() == 6.4);
}


BOOST_AUTO_TEST_SUITE_END() // with_infrastructure
