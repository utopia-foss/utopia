#define BOOST_TEST_MODULE string test

#include <string>
#include <vector>
#include <list>

#include <boost/test/included/unit_test.hpp>

#include <utopia/core/string.hh>
#include <utopia/core/testtools.hh>

// ----------------------------------------------------------------------------

// Configure BOOST.Test to automatically compare string containers element-wise
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector<std::string>)

using namespace Utopia;
using namespace Utopia::TestTools;


// -- Tests -------------------------------------------------------------------

/// Tests the string joining function
BOOST_AUTO_TEST_CASE(test_join) {
    // May be empty
    BOOST_TEST(join({}) == "");

    // Need not explicitly specify a container type
    BOOST_TEST(join({"foo", "bar", "baz"}) == "foo, bar, baz");

    // ... but can:
    const auto segments = std::list<std::string>{"foo", "bar", "baz", "spam"};
    BOOST_TEST(join(segments) == "foo, bar, baz, spam");

    // Can also pass a custom delimiter
    BOOST_TEST(join(segments, " -> ") == "foo -> bar -> baz -> spam");
}


/// Tests the string splitting function
BOOST_AUTO_TEST_CASE(test_split) {
    // Shortcut for type of string segments
    using S = std::vector<std::string>;

    // Typical cases
    BOOST_TEST(split("foo bar baz") == S({"foo", "bar", "baz"}));
    BOOST_TEST(split("foo bar baz") == S({"foo", "bar", "baz"}));
    BOOST_TEST(split("foo.bar.baz", ".") == S({"foo", "bar", "baz"}));

    // Empty strings or without delimiter
    BOOST_TEST(split("") == S());
    BOOST_TEST(split("foo") == S({"foo"}));
    BOOST_TEST(split("foo bar baz", ",") == S({"foo bar baz"}));

    // Empty segments on the side are preserved
    BOOST_TEST(split("foo bar baz ") == S({"foo", "bar", "baz", ""}));
    BOOST_TEST(split(" foo bar baz") == S({"", "foo", "bar", "baz"}));
    BOOST_TEST(split(" foo bar baz ") == S({"", "foo", "bar", "baz", ""}));

    // ... but multiple delimiter characters side by side are compressed away!
    BOOST_TEST(split("foo  bar baz") == S({"foo", "bar", "baz"}));
    BOOST_TEST(split("foo   bar") == S({"foo", "bar"}));
    BOOST_TEST(split("foo->bar->baz", "->") == S({"foo", "bar", "baz"}));

    // However, they are also allowed in arbitrary order, because that's how
    // boost::algorithm::split works ...
    BOOST_TEST(split("foo->-bar->>->baz", "->") == S({"foo", "bar", "baz"}));

    // Can also request a custom return container
    BOOST_TEST(split<std::list<std::string>>("foo bar baz") ==
               std::list<std::string>({"foo", "bar", "baz"}));

    // How about multi-character delimiters?
    BOOST_TEST(split("foo->bar", "->") == S({"foo", "bar"}));
}
