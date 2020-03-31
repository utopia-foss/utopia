#define BOOST_TEST_MODULE dataset_lifecycle_test
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdfutilities.hh>

#include <boost/test/included/unit_test.hpp>

using namespace Utopia::DataIO;
using namespace std::literals::chrono_literals;
using hsizevec = std::vector< hsize_t >;

template < class LHS, class RHS >
void
check_hdfdatasets(LHS& lhs, RHS& rhs)
{
    BOOST_TEST(lhs.get_path() == rhs.get_path());
    BOOST_TEST(lhs.get_C_id() == rhs.get_C_id());
    BOOST_TEST(lhs.get_parent_id().get_id() == rhs.get_parent_id().get_id());
    BOOST_TEST(lhs.get_rank() == rhs.get_rank());
    BOOST_TEST(lhs.get_capacity() == rhs.get_capacity());
    BOOST_TEST(lhs.get_current_extent() == rhs.get_current_extent());
    BOOST_TEST(lhs.get_chunksizes() == rhs.get_chunksizes());
    BOOST_TEST(lhs.get_compresslevel() == rhs.get_compresslevel());
}

BOOST_AUTO_TEST_CASE(dataset_lifecycle_test)
{
    Utopia::setup_loggers();
    HDFFile file("dataset_test_lifetime.h5", "w");
    spdlog::get("data_io")->set_level(spdlog::level::info);

    HDFGroup           lifecyclegroup(file, "/lifecycletest");
    std::vector< int > data(100, 42);

    HDFDataset first(lifecyclegroup, "first", { 100 }, { 10 }, 5);
    BOOST_TEST(first.get_refcount() == -1);

    first.write(data.begin(), data.end(), [](int& value) { return value; });
    BOOST_TEST(first.get_refcount() == 1);

    first.add_attribute("testattribute_for_refcount", first.get_refcount());
    BOOST_TEST(first.get_refcount() == 1);

    HDFDataset copied(first);
    check_hdfdatasets(first, copied);
    BOOST_TEST(first.get_refcount() == 2);
    BOOST_TEST(copied.get_refcount() == 2);

    HDFDataset copy_assigned = first;
    check_hdfdatasets(first, copy_assigned);
    BOOST_TEST(first.get_refcount() == 3);
    BOOST_TEST(copy_assigned.get_refcount() == 3);

    HDFDataset moved(std::move(copied));
    check_hdfdatasets(first, moved);
    BOOST_TEST(first.get_refcount() == 3);
    BOOST_TEST(moved.get_refcount() == 3);

    HDFDataset move_assigend = std::move(moved);
    check_hdfdatasets(first, move_assigend);
    BOOST_TEST(first.get_refcount() == 3);
    BOOST_TEST(move_assigend.get_refcount() == 3);

    move_assigend.close();
    copy_assigned.close();
    BOOST_TEST(first.get_refcount() == 1);

    HDFDataset second(lifecyclegroup, "second", { 100 }, { 10 }, 5);

    second.add_attribute("testattribute for buffer1", "one");
    second.add_attribute("testattribute for buffer2", "two");
    BOOST_TEST(second.get_refcount() == -1);

    second.write(data);
    BOOST_TEST(second.get_refcount() == 1);

    // create two datasets to test swap
    HDFDataset x(lifecyclegroup, "x", { 2000, 100 }, { 10, 12 }, 7);

    x.add_attribute("testattr_x", "I iz X");

    HDFDataset y(lifecyclegroup, "y", { 1000, 200, 10 }, { 20, 3, 7 }, 2);
    y.add_attribute("testattr_y", "I iz Y");

    swap(x, y);

    BOOST_TEST(x.get_path() == "y");
    BOOST_TEST(y.get_path() == "x");

    BOOST_TEST(x.get_rank() == 3);
    BOOST_TEST(y.get_rank() == 2);

    BOOST_TEST(x.get_current_extent() == (std::vector< hsize_t >{ }),
               boost::test_tools::per_element());
    BOOST_TEST(y.get_current_extent() == (std::vector< hsize_t >{  }),
               boost::test_tools::per_element());

    BOOST_TEST(x.get_compresslevel() == 2);
    BOOST_TEST(y.get_compresslevel() == 7);

    BOOST_TEST(x.get_capacity() == (std::vector< hsize_t >{ 1000, 200, 10 }),
               boost::test_tools::per_element());
    BOOST_TEST(y.get_capacity() == (std::vector< hsize_t >{ 2000, 100 }),
               boost::test_tools::per_element());

    BOOST_TEST(x.get_offset() == (std::vector< hsize_t >{ 0, 0, 0 }),
               boost::test_tools::per_element());
    BOOST_TEST(y.get_offset() == (std::vector< hsize_t >{ 0, 0 }),
               boost::test_tools::per_element());
    BOOST_TEST(
        x.get_attribute_buffer() ==
        (std::vector< std::pair< std::string, typename HDFType::Variant > >{
            { "testattr_y", "I iz Y" } }));

    BOOST_TEST(
        y.get_attribute_buffer() ==
        (std::vector< std::pair< std::string, typename HDFType::Variant > >{
            { "testattr_x", "I iz X" } }));
}
