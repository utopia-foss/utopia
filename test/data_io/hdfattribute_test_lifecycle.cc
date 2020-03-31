
#include "utopia/data_io/hdfidentifier.hh"
#define BOOST_TEST_MODULE attribute_lifecycle_test

#include <iostream>

#include <boost/test/included/unit_test.hpp>

#include <utopia/data_io/hdfattribute.hh>

using namespace Utopia::DataIO;

namespace Utopia
{
namespace DataIO
{

std::ostream&
boost_test_print_type(std::ostream& ostr, HDFIdentifier const& right)
{
    ostr << right.get_id() << std::endl;
    return ostr;
}
}
}

BOOST_AUTO_TEST_CASE(attribute_lifecycle_test)
{
    Utopia::setup_loggers();

    hid_t file =
        H5Fcreate("testfileX.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // bind group to object
    HDFObject< HDFCategory::group > group(
        H5Gcreate(file, "/testgroup", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        &H5Gclose,
        "/testgroup");
        
    HDFAttribute attribute(group, "testattribute");

    BOOST_TEST(not attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == -1);
    attribute.write("this is a testattribute");

    BOOST_TEST(attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == 1);
    attribute.close();

    BOOST_TEST(not attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == -1);

    attribute.open(group, "testattribute");
    BOOST_TEST(attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == 1);

    attribute.close();
    BOOST_TEST(not attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == -1);

    attribute.open(group, "pi");
    BOOST_TEST(not attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == -1);

    attribute.write(3.14);
    BOOST_TEST(attribute.is_valid());
    BOOST_TEST(attribute.get_refcount() == 1);

    HDFAttribute attribute2(group, "2pi");
    attribute2.write(2 * 3.14);

    BOOST_TEST(attribute2.get_refcount() == 1);
    attribute2.close();

    HDFAttribute attribute3(group, "2pi");

    BOOST_TEST(not attribute2.is_valid());
    BOOST_TEST(attribute3.is_valid());

    auto val = attribute3.read< double >();
    BOOST_TEST(std::abs(std::get< 1 >(val) - 6.28) < 1e-16);

    // copy constructed
    HDFAttribute attribute_copied(attribute3);
    auto         val2 = attribute_copied.read< double >();
    BOOST_TEST(std::abs(std::get< 1 >(val2) - 6.28) < 1e-16);
    BOOST_TEST(attribute_copied.get_path() == attribute3.get_path());
    BOOST_TEST(attribute_copied.get_parent_id() == attribute3.get_parent_id());
    BOOST_TEST(attribute_copied.get_refcount() == 2);
    BOOST_TEST(attribute3.get_refcount() == 2);

    // copy assigned
    HDFAttribute attribute_copyassigned = attribute_copied;
    BOOST_TEST(attribute_copied.get_path() ==
               attribute_copyassigned.get_path());
    BOOST_TEST(attribute_copied.get_parent_id() ==
               attribute_copyassigned.get_parent_id());
    BOOST_TEST(attribute_copied.get_refcount() == 3);
    BOOST_TEST(attribute3.get_refcount() == 3);

    // move constructed
    HDFAttribute attribute_moved(std::move(attribute_copied));
    BOOST_TEST(attribute_moved.get_path() == attribute3.get_path());
    BOOST_TEST(attribute_moved.get_parent_id() == attribute3.get_parent_id());
    BOOST_TEST(attribute_moved.get_refcount() == 3);

    // move assigned
    HDFAttribute attribute_moveassigned = std::move(attribute_moved);
    BOOST_TEST(attribute_moveassigned.get_path() == attribute3.get_path());
    BOOST_TEST(attribute_moveassigned.get_parent_id() ==
               attribute3.get_parent_id());
    BOOST_TEST(attribute_moveassigned.get_refcount() == 3);

    H5Fclose(file);
}
