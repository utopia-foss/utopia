#define BOOST_TEST_MODULE HDFGROUP_TEST_LIFECYCLE
#include <iostream>

#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>

#include <boost/test/included/unit_test.hpp>

using namespace Utopia::DataIO;

BOOST_AUTO_TEST_CASE(hdfgroup_lifecycle_test)
{
    Utopia::setup_loggers();

    // make file and group to use for copying moving etc.
    HDFFile file("group_test_lifetime.h5", "w");

    HDFGroup first(file, "first");

    // check if copying works
    HDFGroup copied_first(first);

    BOOST_TEST(first.get_path() == copied_first.get_path());
    BOOST_TEST(first.get_C_id() == copied_first.get_C_id());
    BOOST_TEST(first.get_refcount() == 2);

    // copy assignment
    auto second = first;

    BOOST_TEST(first.get_path() == second.get_path());
    BOOST_TEST(first.get_C_id() == second.get_C_id());
    BOOST_TEST(first.get_refcount() == 3);

    // this crosscheck is needed because 'move' changes the object from which
    // something is moved, so we need a copy of it which stays unchanged
    // in order to check correct move.
    // this is needed  for checks -> refcount of first goes one up!
    auto crosscheck(first);
    BOOST_TEST(crosscheck.get_path() == first.get_path());
    BOOST_TEST(crosscheck.get_C_id() == first.get_C_id());
    BOOST_TEST(crosscheck.get_refcount() == 4);

    auto moveassign_from_first = std::move(first);
    BOOST_TEST(crosscheck.get_path() == moveassign_from_first.get_path());
    BOOST_TEST(crosscheck.get_C_id() == moveassign_from_first.get_C_id());
    BOOST_TEST(crosscheck.get_refcount() == 4);

    // check move consturction
    HDFGroup moveconst_second(std::move(second));
    BOOST_TEST(crosscheck.get_path() == moveconst_second.get_path());
    BOOST_TEST(crosscheck.get_C_id() == moveconst_second.get_C_id());
    BOOST_TEST(crosscheck.get_refcount() == 4);

    // check open method
    HDFGroup opened_group;
    while (moveconst_second.is_valid())
    {
        moveconst_second.close();
    }
    BOOST_TEST(not moveconst_second.is_valid());
    opened_group.open(*file.get_basegroup(), "first");
    BOOST_TEST(opened_group.is_valid());
}
