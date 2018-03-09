#include "../hdfattribute.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>
#include <string>

using namespace Utopia::DataIO;

int main() {
    // make file
    HDFFile file("testfile.h5", "w");

    // make groups
    HDFGroup low_group = HDFGroup(*file.get_basegroup(), file, "/testgroup");
    HDFGroup low_group2 = HDFGroup(*file.get_basegroup(), file, "/testgroup2");

    // adding attribute1
    std::string attributename = "testattribute";
    std::string attribute_data = "this is a testing attribute";
    HDFAttribute<HDFGroup, std::string> attribute(low_group, attributename);

    // writing a string
    attribute.write(attribute_data);

    // adding attribute2
    std::string attributename2 = "testattribute2";

    HDFAttribute<HDFGroup, int> attribute2(low_group2, "testattribute2");

    // writing primitive type attribute
    int attribute_data2 = 42;
    attribute2.write(attribute_data2);

    return 0;
}