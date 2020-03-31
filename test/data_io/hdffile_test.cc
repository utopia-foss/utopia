#define BOOST_TEST_MODULE hdf5_filetest

#include <iostream>

#include "utopia/data_io/hdffile.hh"

#include <utopia/core/utils.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;

struct Fix
{
    void
    setup()
    {
        Utopia::setup_loggers();
    }
};

BOOST_AUTO_TEST_SUITE(Suite, *boost::unit_test::fixture< Fix >())

BOOST_AUTO_TEST_CASE(file_creation)
{
    HDFFile file_w("filetest_w.h5", "w");
    BOOST_TEST(file_w.is_valid());

    HDFFile file_x("filetest_x.h5", "x");
    BOOST_TEST(file_x.is_valid());




    HDFFile file_a("filetest_x.h5", "a");
    BOOST_TEST(file_a.is_valid());

    BOOST_CHECK_EXCEPTION(
        HDFFile("filetest_r.h5", "r"),
        std::invalid_argument,
        [](const std::invalid_argument& e) -> bool {
            BOOST_TEST(e.what() ==
                       std::string("Error: invalid argument! The id given for "
                                   "an object of category file at '' cannot be "
                                   "managed by an HDFObject instance!"));

            return true;
        });

    BOOST_CHECK_EXCEPTION(
        HDFFile("filetest_r+.h5", "r+"),
        std::invalid_argument,
        [](const std::invalid_argument& e) -> bool {
            BOOST_TEST(e.what() ==
                       std::string("Error: invalid argument! The id given for "
                                   "an object of category file at '' cannot be "
                                   "managed by an HDFObject instance!"));

            return true;
        });

    file_w.close();
    file_x.close();
    file_a.close();

    // remove files to clean up stuff
    std::remove("filetest_w.h5");
    std::remove("filetest_x.h5");
    std::remove("filetest_a.h5");
}

BOOST_AUTO_TEST_CASE(file_lifecycle)
{
    HDFFile file("filetest_lifecycle.h5", "w");
    BOOST_TEST(file.is_valid());

    HDFFile moveconstructed_file(std::move(file));
    BOOST_TEST(moveconstructed_file.is_valid());
    BOOST_TEST(moveconstructed_file.get_basegroup() != nullptr);
    BOOST_TEST(moveconstructed_file.get_basegroup()->is_valid());
    BOOST_TEST(moveconstructed_file.get_refcount() == 1);

    BOOST_TEST(not file.is_valid());
    BOOST_TEST(file.get_basegroup() == nullptr);

    HDFFile moveassigned_file = std::move(moveconstructed_file);
    BOOST_TEST(moveassigned_file.is_valid());
    BOOST_TEST(moveassigned_file.get_basegroup() != nullptr);
    BOOST_TEST(moveassigned_file.get_basegroup()->is_valid());
    BOOST_TEST(moveassigned_file.get_refcount() == 1);

    BOOST_TEST(not file.is_valid());
    BOOST_TEST(file.get_basegroup() == nullptr);

    file.close();
    std::remove("filetest_lifecycle.h5");
}

BOOST_AUTO_TEST_CASE(file_functionality)
{
    HDFFile file("filetest_functionality.h5", "w");
    BOOST_TEST(file.is_valid());
    BOOST_TEST(file.get_basegroup()->is_valid());

    auto group = file.open_group("/some/group/anywhere");
    BOOST_TEST(group->is_valid());

    auto dataset = file.open_dataset("/some/dataset/nowhere/at/all/dset");
    BOOST_TEST(not dataset->is_valid());

    dataset->write(std::vector< int >{ 1, 2, 3, 4, 5 });
    BOOST_TEST(dataset->is_valid());

    // test if close has the desired effect of closing the file and making
    // it accessible immediatelly afterwards with the correct data in it
    file.close();
    BOOST_TEST(not file.is_valid());

    file.open("filetest_functionality.h5", "r");
    BOOST_TEST(file.is_valid());
    // file and basegroup open
    BOOST_TEST(H5Fget_obj_count(file.get_C_id(), H5F_OBJ_FILE) == 1);
    BOOST_TEST(H5Fget_obj_count(file.get_C_id(), H5F_OBJ_GROUP) == 1);

    auto [shape, data] = file.open_dataset("/some/dataset/nowhere/at/all/dset")
                             ->read< std::vector< int > >();

    BOOST_TEST(data == (std::vector< int >{ 1, 2, 3, 4, 5 }));
    BOOST_TEST(shape == (std::vector< hsize_t >{ 5 }));

    file.close();
    std::remove("filetest_functionality.h5");
}

BOOST_AUTO_TEST_SUITE_END()
