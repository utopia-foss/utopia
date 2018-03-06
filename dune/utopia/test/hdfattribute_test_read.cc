#include "../data_io/hdfattribute.hh"
#include "../data_io/hdfmockclasses.hh"
#include <cassert>
#include <iostream>
using namespace Utopia::DataIO;

int main() {
    // open a file
    HDFFile file("/Users/haraldmack/testfile.h5", "r");

    // open a group 
    HDFGroup low_group = HDFGroup(file.get_basegroup(), "/testgroup");
    HDFGroup low_group2 = HDFGroup(file.get_basegroup(), "/testgroup2");

    // open attributes
    std::string attributename = "testattribute";
    HDFAttribute<HDFGroup, std::string> attribute(
                low_group, attributename);

    std::string attributename2 = "testattribute2";
    HDFAttribute<HDFGroup, int> attribute2(
                low_group2, "testattribute2");

    // read stuff and test that it is tha same as in hdfattribute_test_write.cc
    std::string read_attribute1 = attribute.read();
    int read_attribute2 = attribute2.read();

    std::string attribute_data1 = "this is a testing attribute";
    int attribute_data2 = 42;

    assert(read_attribute1 == attribute_data1);
    assert(read_attribute2 == attribute_data2);
    return 0;
}