#include "hdfmockclasses.hh"
#include <cassert>
#include <iostream>
using namespace Utopia::DataIO;
int main() {
    HDFFile file("/Users/haraldmack/Desktop/utopia_test.h5", "w");
    HDFGroup testgroup1(file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(testgroup1, "/testgroup1/testgroup2");
    HDFGroup testgroup3 = testgroup2;

    return 0;
}