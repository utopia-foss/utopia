#include "utopia/core/logging.hh"
#define BOOST_TEST_MODULE HDFINTEGRATION_TEST
#include <iostream>
#include <vector>

#include <utopia/data_io/hdfattribute.hh>
#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>

#include <boost/test/included/unit_test.hpp>

using namespace Utopia::DataIO;
// for getting info

struct Teststruct
{
    double x;
    std::string y;
    std::vector<int> z;
};

void write(std::vector<Teststruct>& data)
{
    HDFFile file("integrationtest_file.h5", "w");

    // open file and read
    auto group = file.get_basegroup()->open_group(
        "first_deeper/second_deeper/third_deeper");

    auto dataset = group->open_dataset("dataset1", {data.size()});
    dataset->write(data.begin(), data.end(), [](auto& value) { return value.x; });
    // std::string attrdata = "this is a testattribute";
    dataset->add_attribute(
        "testattribute",
        std::string("this is an attribute to a double dataset"));

    // write variable length string
    auto dataset2 = group->open_dataset("dataset2", {data.size()});
    dataset2->write(data.begin(), data.end(),
                    [](auto& value) -> std::string& { return value.y; });

    dataset2->add_attribute("stringattribute",
                            "this is an attribute to std::string");

    auto dataset3 = group->open_dataset("dataset3", {data.size()});
    dataset3->write(data.begin(), data.end(),
                    [](auto& value) -> std::vector<int>& { return value.z; });

    dataset3->add_attribute("integer vector attribute",
                            "this is an attribute to an int vector");
}

void read(std::vector<Teststruct>& data)
{
    HDFFile file("integrationtest_file.h5", "r");

    // open file and read
    auto group = file.get_basegroup()->open_group(
        "first_deeper/second_deeper/third_deeper");

    auto dataset1 = group->open_dataset("dataset1");
    auto dataset2 = group->open_dataset("dataset2");
    auto dataset3 = group->open_dataset("dataset3");

    auto [shape1, values] = dataset1->read<std::vector<double>>();
    BOOST_TEST(shape1 == std::vector<hsize_t>{data.size()});

    BOOST_TEST(values.size() == data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        BOOST_TEST(std::abs(values[i] - data[i].x) < 1e-16);
    }

    auto [shape2, read_stringdata] = dataset2->read<std::vector<std::string>>();
    BOOST_TEST(shape2 == std::vector<hsize_t>{data.size()});

    BOOST_TEST(read_stringdata.size() == data.size());
    for (std::size_t i = 0; i < read_stringdata.size(); ++i)
    {
        BOOST_TEST(std::string(read_stringdata[i]) == data[i].y);
    }

    auto [shape3, read_data_int] = dataset3->read<std::vector<std::vector<int>>>();
    BOOST_TEST(shape3 == std::vector<hsize_t>{data.size()});

    for (std::size_t i = 0; i < read_data_int.size(); ++i)
    {
        BOOST_TEST(read_data_int[i].size() == data[i].z.size());
        for (std::size_t j = 0; j < data[i].z.size(); ++j)
        {
            BOOST_TEST(read_data_int[i][j] == data[i].z[j]);
        }
    }

    HDFAttribute attribute(*dataset1, "testattribute");
    auto [shape_attr, read_attribute] = attribute.read<std::string>();
    BOOST_TEST(shape_attr.size() == 1);
    BOOST_TEST(shape_attr[0] == 1);
    BOOST_TEST(read_attribute == "this is an attribute to a double dataset");
}

BOOST_AUTO_TEST_CASE(hdf_integration_test)
{
    Utopia::setup_loggers();
    
    std::vector<Teststruct> data(50);
    double d = 3.14;
    std::string a = "a";
    std::size_t size = 1;
    std::generate(data.begin(), data.end(), [&]() {
        Teststruct object;
        object.x = d;
        object.y = a;
        object.z = std::vector<int>(size, 1);
        a += "a";
        d += 1.;
        size += 1;
        return object;
    });

    write(data);
    read(data);
}
