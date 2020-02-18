#define BOOST_TEST_MODULE dataspace_test

#include <stdexcept>
#include <list>
#include <string>
#include <vector>

#include <hdf5.h>

#include "utopia/core/logging.hh"
#include "utopia/data_io/hdfutilities.hh"
#include <utopia/data_io/hdfdataspace.hh>
#include <utopia/data_io/hdffile.hh>
#include "utopia/data_io/hdfdataset.hh"

#include <utopia/core/utils.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;
using namespace Utopia::Utils;

struct Fix
{
    void setup () { Utopia::setup_loggers(); }
};

BOOST_AUTO_TEST_SUITE(Suite,
                      *boost::unit_test::fixture<Fix>())

BOOST_AUTO_TEST_CASE(dataspace_lifecycle)
{

    HDFFile file("dataspace_testfile.h5", "w");

    auto dataset = file.open_dataset("/dataset");

    dataset->write(std::vector< double >(10, 3.14));

    HDFDataspace dspace(2, { 10, 10 }, { 100, 100 });

    BOOST_TEST(dspace.size() == (std::vector< hsize_t >{ 10, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() == (std::vector< hsize_t >{ 100, 100 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.rank() == 2);

    auto [size, capacity] = dspace.get_properties();
    BOOST_TEST(size == (std::vector< hsize_t >{ 10, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(capacity == (std::vector< hsize_t >{ 100, 100 }),
               boost::test_tools::per_element());

    auto filespace = dataset->get_filespace();

    BOOST_TEST(filespace.size() == (std::vector< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(filespace.capacity() ==
                   (std::vector< hsize_t >{ H5S_UNLIMITED }),
               boost::test_tools::per_element());

    auto memspace = dataset->get_memspace();
    BOOST_TEST(memspace.get_id() == H5S_ALL);

    filespace.close();
    BOOST_TEST(filespace.is_valid() == false);

    filespace.open(2, { 100, 500 }, { H5S_UNLIMITED, H5S_UNLIMITED });
    BOOST_TEST(filespace.size() == (arma::Row< hsize_t >{ 100, 500 }),
               boost::test_tools::per_element());
    BOOST_TEST(filespace.capacity() ==
                   (arma::Row< hsize_t >{ H5S_UNLIMITED, H5S_UNLIMITED }),
               boost::test_tools::per_element());

    filespace.close();
    filespace.open(*dataset);
    BOOST_TEST(filespace.size() == (arma::Row< hsize_t >{ 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(filespace.capacity() == (arma::Row< hsize_t >{ H5S_UNLIMITED }),
               boost::test_tools::per_element());

    HDFDataspace dspace2(3, { 10, 10, 10 }, { 100, 100, 100 });

    swap(dspace, dspace2);
    BOOST_TEST(dspace.rank() == 3);
    BOOST_TEST(dspace.size() == (arma::Row< hsize_t >{ 10, 10, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace.capacity() == (arma::Row< hsize_t >{ 100, 100, 100 }),
               boost::test_tools::per_element());

    BOOST_TEST(dspace2.rank() == 2);
    BOOST_TEST(dspace2.size() == (arma::Row< hsize_t >{ 10, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(dspace2.capacity() == (arma::Row< hsize_t >{ 100, 100 }),
               boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(dataspace_selection_and_resize)
{
    HDFDataspace dataspace(3, { 10, 20, 10 }, { 200, 300, 200 });

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
    HDFDataspace dspace(2, { 10, 20 }, { 100, 100 });

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
                    std::to_string(dspace.get_id())));
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

    dspace.open(2, { 1, 2 }, { 2, 2 });
    dspace.release_selection();
    BOOST_CHECK_EXCEPTION(
        dspace.get_selection_bounds(),
        std::runtime_error,
        [](const std::runtime_error& re) -> bool {
            return (
                re.what() ==
                std::string(
                    "Error when trying to get selection bounds for dataspace"));
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
                std::string("Error, trying to resize an invalid dataspace"));
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

    dspace.open(2, { 1, 2 }, { 2, 2 });
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
}

BOOST_AUTO_TEST_SUITE_END()
