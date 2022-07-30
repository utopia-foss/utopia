#define BOOST_TEST_MODULE hdfobject_test

#include <hdf5.h>

#include "utopia/data_io/hdfobject.hh"
#include "utopia/data_io/hdfutilities.hh"

#include "utopia/core/logging.hh"
#include <utopia/core/type_traits.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;

struct Fix
{
    void
    setup()
    {
        Utopia::setup_loggers();
        spdlog::get("data_io")->set_level(spdlog::level::debug);
    }
};

BOOST_AUTO_TEST_SUITE(Suite, *boost::unit_test::fixture< Fix >())

using Group = HDFObject< HDFCategory::group >;

BOOST_AUTO_TEST_CASE(constructor_test)
{

    hid_t file = H5Fcreate("object_constructor_testfile.h5",
                           H5F_ACC_TRUNC,
                           H5P_DEFAULT,
                           H5P_DEFAULT);

    BOOST_TEST(H5Iis_valid(file) > 0);

    Group object(
        H5Gcreate(file, "/testobject", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        &H5Gclose);

    BOOST_TEST(H5Iis_valid(object.get_C_id()) > 0);

    BOOST_TEST(object.is_valid());
    BOOST_TEST(object.get_refcount() == 1);
    BOOST_TEST(object.get_path() == "/testobject");

    Group copy_construct_object(object);
    BOOST_TEST(copy_construct_object.is_valid());
    BOOST_TEST(copy_construct_object.get_refcount() == 2);
    BOOST_TEST(copy_construct_object.get_path() == "/testobject");

    BOOST_TEST(object.get_refcount() == 2);

    Group copy_assign_object = object;
    BOOST_TEST(copy_assign_object.is_valid());
    BOOST_TEST(copy_assign_object.get_refcount() == 3);
    BOOST_TEST(copy_assign_object.get_path() == "/testobject");

    BOOST_TEST(object.get_refcount() == 3);

    Group move_constructed_object(std::move(copy_assign_object));
    BOOST_TEST(move_constructed_object.is_valid());
    BOOST_TEST(move_constructed_object.get_refcount() == 3);
    BOOST_TEST(move_constructed_object.get_path() == "/testobject");

    BOOST_TEST(copy_assign_object.get_refcount() == -1);
    BOOST_TEST(not copy_assign_object.is_valid());
    BOOST_TEST(copy_assign_object.get_path() == "");

    Group move_assigned_object = std::move(move_constructed_object);
    BOOST_TEST(move_assigned_object.is_valid());
    BOOST_TEST(move_assigned_object.get_refcount() == 3);
    BOOST_TEST(move_assigned_object.get_path() == "/testobject");

    BOOST_TEST(move_constructed_object.get_refcount() == -1);
    BOOST_TEST(not move_constructed_object.is_valid());
    BOOST_TEST(move_constructed_object.get_path() == "");

    // check refcounting system
    BOOST_TEST(copy_construct_object.get_refcount() == 3);

    copy_construct_object.close();

    BOOST_TEST(not copy_construct_object.is_valid());
    BOOST_TEST(copy_construct_object.get_C_id() == -1);
    BOOST_TEST(object.is_valid());
    BOOST_TEST(object.get_refcount() == 2);

    BOOST_TEST(move_assigned_object.is_valid());
    BOOST_TEST(move_assigned_object.get_refcount() == 2);

    BOOST_TEST(copy_construct_object.get_refcount() == -1);
    BOOST_TEST(copy_construct_object.get_path() == "");

    move_assigned_object.close();

    BOOST_TEST(not move_assigned_object.is_valid());
    BOOST_TEST(move_assigned_object.get_C_id() == -1);

    BOOST_TEST(object.is_valid());
    BOOST_TEST(object.get_refcount() == 1);
    BOOST_TEST(move_assigned_object.get_refcount() == -1);
    BOOST_TEST(move_assigned_object.get_path() == "");

    object.close();
    BOOST_TEST(not object.is_valid());
    BOOST_TEST(object.get_refcount() == -1);
    BOOST_TEST(object.get_C_id() == -1);
    BOOST_TEST(object.get_path() == "");



    // create two objects for swapping test
    Group x(
        H5Gcreate(file, "/x", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        &H5Gclose);

    Group y(
        H5Gcreate(file, "/y", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        &H5Gclose);

    auto x_c_id = x.get_C_id();
    auto y_c_id = y.get_C_id();

    swap(x,y);

    BOOST_TEST(x.get_path() == "/y");
    BOOST_TEST(y.get_path() == "/x");
    BOOST_TEST(y.get_C_id() == x_c_id);
    BOOST_TEST(x.get_C_id() == y_c_id);


    H5Fclose(file);
}

BOOST_AUTO_TEST_CASE(access_test)
{

    hid_t file = H5Fcreate(
        "object_access_testfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    hid_t grp = H5Gcreate(
        file, "/access_testobject", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5O_info_t info;

    // version bigger than 1.12.0 -> H5Oget_info3 is used, else H5Oget_info1
    #if H5_VERSION_GE(1, 12, 0)
        H5Oget_info(grp, &info, H5O_INFO_ALL);
    #else
        H5Oget_info(grp, &info);
    #endif

    BOOST_TEST(info.rc == 1);
    BOOST_TEST(H5Iget_ref(grp) == 1);

    auto object = Group(std::move(grp), &H5Gclose);
    BOOST_TEST(object.get_refcount() == 1);

    auto opened_object =
        Group(H5Gopen(file, "/access_testobject", H5P_DEFAULT), &H5Gclose);
    BOOST_TEST(opened_object.get_refcount() == 1);

    auto leaf_object = Group(
        H5Gcreate(
            object.get_C_id(), "leaf", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
        &H5Gclose);

    BOOST_TEST(leaf_object.get_refcount() == 1);
    BOOST_TEST(leaf_object.get_path() == "/access_testobject/leaf");
    BOOST_TEST(leaf_object.is_valid());

    // test now
    object.close();

    BOOST_CHECK_EXCEPTION(Group(H5Gcreate(object.get_C_id(),
                                          "access_testobject_test2",
                                          H5P_DEFAULT,
                                          H5P_DEFAULT,
                                          H5P_DEFAULT),
                                &H5Gclose),
                          std::invalid_argument,
                          [](const std::invalid_argument& e) {
                              std::string errorstring =
                                  "Error: invalid argument! The id given for "
                                  "an object of category group at '' cannot be "
                                  "managed by an HDFObject instance!";
                              BOOST_TEST(e.what() == errorstring);
                              return true;
                          });

    Group second_leafobject(H5Gcreate(opened_object.get_C_id(),
                                      "leaf2",
                                      H5P_DEFAULT,
                                      H5P_DEFAULT,
                                      H5P_DEFAULT),
                            &H5Gclose);

    BOOST_TEST(second_leafobject.get_refcount() == 1);
    BOOST_TEST(second_leafobject.is_valid());
    BOOST_TEST(second_leafobject.get_path() == "/access_testobject/leaf2");

    H5Fclose(file);
}

BOOST_AUTO_TEST_SUITE_END()
