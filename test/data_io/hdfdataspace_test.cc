
#define BOOST_TEST_MODULE dataspace_test

#include <list>
#include <stdexcept>
#include <string>
#include <vector>

#include <hdf5.h>

#include "utopia/core/logging.hh"
#include "utopia/data_io/hdfutilities.hh"
#include <utopia/data_io/hdfdataspace.hh>

#include <utopia/core/type_traits.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;
using namespace Utopia::Utils;

using Dataset = HDFObject< HDFCategory::dataset >;

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

BOOST_AUTO_TEST_CASE(dataspace_lifecycle)
{

    hid_t file = H5Fcreate(
        "dataspace_testfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    hsize_t dims[1]    = { 10 };
    hsize_t maxdims[1] = { 10 };

    hid_t   space   = H5Screate_simple(1, dims, maxdims);
    hid_t   dataset = H5Dcreate(file,
                              "/dataset",
                              H5T_NATIVE_DOUBLE,
                              space,
                              H5P_DEFAULT,
                              H5P_DEFAULT,
                              H5P_DEFAULT);
    Dataset dset(std::move(dataset), &H5Dclose);

    std::vector< double > v(10, 3.14);
    H5Dwrite(dset.get_C_id(),
             H5T_NATIVE_DOUBLE,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             v.data());

    HDFDataspace dspace("testdspace", 1, { 10 }, { 100 });

    BOOST_TEST(dspace.size() == (std::vector< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() == (std::vector< hsize_t >{ 100 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.rank() == 1);
    BOOST_TEST(dspace.get_refcount() == 1);

    auto [size, capacity] = dspace.get_properties();
    BOOST_TEST(size == (std::vector< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(capacity == (std::vector< hsize_t >{ 100 }),
               boost::test_tools::per_element());

    dspace.close();
    BOOST_TEST(dspace.is_valid() == false);

    dspace.open(
        "new_filespace", 2, { 100, 500 }, { H5S_UNLIMITED, H5S_UNLIMITED });
    BOOST_TEST(dspace.size() == (arma::Row< hsize_t >{ 100, 500 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() ==
                   (arma::Row< hsize_t >{ H5S_UNLIMITED, H5S_UNLIMITED }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.get_refcount() == 1);

    dspace.close();
    dspace.open(dset);
    BOOST_TEST(dspace.size() == (arma::Row< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() == (arma::Row< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.get_refcount() == 1);

    HDFDataspace dspace2("dspace2", 3, { 10, 10, 10 }, { 100, 100, 100 });

    swap(dspace, dspace2);
    BOOST_TEST(dspace.rank() == 3);
    BOOST_TEST(dspace.size() == (arma::Row< hsize_t >{ 10, 10, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() == (arma::Row< hsize_t >{ 100, 100, 100 }),
               boost::test_tools::per_element());

    BOOST_TEST(dspace2.rank() == 1);
    BOOST_TEST(dspace2.size() == (arma::Row< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace2.capacity() == (arma::Row< hsize_t >{ 10 }),
               boost::test_tools::per_element());

    HDFDataspace dspace_copied(dspace2);
    BOOST_TEST(dspace_copied.get_C_id() == dspace2.get_C_id());
    BOOST_TEST(dspace_copied.get_path() == dspace2.get_path());
    BOOST_TEST(dspace_copied.get_refcount() == 2);
    BOOST_TEST(dspace2.get_refcount() == 2);

    HDFDataspace dspace_copyassigned = dspace2;
    BOOST_TEST(dspace_copyassigned.get_C_id() == dspace2.get_C_id());
    BOOST_TEST(dspace_copyassigned.get_path() == dspace2.get_path());
    BOOST_TEST(dspace_copyassigned.get_refcount() == 3);
    BOOST_TEST(dspace2.get_refcount() == 3);

    HDFDataspace dspace_moved(std::move(dspace_copied));

    HDFDataspace dspace_moveassigned = std::move(dspace_moved);

    H5Sclose(space);
    H5Fclose(file);

    // TODO: check reference counting stuff
}

BOOST_AUTO_TEST_CASE(dataspace_selection_and_resize)
{
    HDFDataspace dataspace(
        "other_testspace", 3, { 10, 20, 10 }, { 200, 300, 200 });

    BOOST_TEST(dataspace.size() == (arma::Row< hsize_t >{ 10, 20, 10 }),
               boost::test_tools::per_element());

    dataspace.resize({ 100, 100, 100 });

    BOOST_TEST(dataspace.size() == (arma::Row< hsize_t >{ 100, 100, 100 }),
               boost::test_tools::per_element());

    dataspace.select_slice(
        { 10, 0, 10 }, { 20, 20, 20 }, arma::Row< hsize_t >{ 1, 2, 1 });

    auto [begin, end] = dataspace.get_selection_bounds();

    BOOST_TEST(begin == (arma::Row< hsize_t >{ 10, 0, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(
        end == (arma::Row< hsize_t >{ 19, 18, 19 }), // take steps into account
        boost::test_tools::per_element());

    std::tie(begin, end) = dataspace.get_selection_bounds();

    dataspace.select_all();

    std::tie(begin, end) = dataspace.get_selection_bounds();

    BOOST_TEST(begin == (arma::Row< hsize_t >{ 0, 0, 0 }),
               boost::test_tools::per_element());
    BOOST_TEST(end == (arma::Row< hsize_t >{ 99, 99, 99 }),
               boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(exception_test)
{
    // exception testing goes here
    HDFDataspace dspace("testspace", 2, { 10, 20 }, { 100, 100 });

    dspace.close();
    BOOST_CHECK_EXCEPTION(
        dspace.rank(),
        std::runtime_error,
        [](const std::runtime_error& re) -> bool {
            return (
                re.what() ==
                std::string("Error, trying to get rank of invalid dataspace"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.get_properties(),
        std::runtime_error,
        [&dspace](const std::runtime_error& re) -> bool {
            return (
                re.what() ==
                std::string(
                    "Error, trying to get properties of invalid dataspace," +
                    std::to_string(dspace.get_C_id())));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.select_slice({ 1, 0 }, { 7, 4 }, { 2, 1 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (
                re.what() ==
                std::string(
                    "Error, trying to select a slice in an invalid dataspace"));
        });

    dspace.open("testdspace", 2, { 1, 2 }, { 2, 2 });
    dspace.release_selection();
    BOOST_CHECK_EXCEPTION(
        dspace.get_selection_bounds(),
        std::runtime_error,
        [](const std::runtime_error& re) -> bool {
            return (
                re.what() ==
                std::string(
                    "Error, cannot get selection bounds of invalid dataspace"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.select_slice({ 0, 0, 0 }, { 10, 10, 10 }, { 1, 1, 1 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (re.what() ==
                    std::string("Error, dimensionality of start and end has to "
                                "be the same as the dataspace's rank"));
        });

    dspace.close();
    BOOST_CHECK_EXCEPTION(
        dspace.select_all(),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (re.what() ==
                    std::string("Error, trying to select everything of an "
                                "invalid dataspace"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.resize({ 10, 10 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (
                re.what() ==
                std::string(
                    "Error, trying to get properties of invalid dataspace,-1"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.release_selection(),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (
                re.what() ==
                std::string("Cannot reset selection, dataspace is invalid"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.rank(), std::runtime_error, [](const std::runtime_error& re) {
            return (
                re.what() ==
                std::string("Error, trying to get rank of invalid dataspace"));
        });

    dspace.open("testspace other", 2, { 1, 2 }, { 2, 2 });
    BOOST_CHECK_EXCEPTION(
        dspace.resize({ 100, 100 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (re.what() == std::string("Error in resizing dataspace"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.select_slice({ 50, 20, 80 }, { 10, 10, 10 }, { 1, 20, 100 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (re.what() ==
                    std::string("Error, dimensionality of start and end has to "
                                "be the same as the dataspace's rank"));
        });

    BOOST_CHECK_EXCEPTION(
        dspace.open("testspace", 2, { 10, 10 }, { 1, 100 }),
        std::runtime_error,
        [](const std::runtime_error& re) {
            return (re.what() ==
                    std::string("Error: Cannot bind object to new "
                                "identifier while the old is still valid"));
        });
}

BOOST_AUTO_TEST_SUITE_END()
