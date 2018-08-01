/**
 * @brief Tests for checking if the lifecycle of an attribute is implemented as it should be.
 *
 * @file hdfattribute_test_write.cc

 */
#include "../hdfattribute.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <dune/common/parallel/mpihelper.hh>
#include <iostream>
#include <random>
#include <string>

using namespace Utopia::DataIO;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    HDFFile file("testfileX.h5", "w");
    auto group = file.open_group("/testgroup");
    HDFAttribute attribute(*group, "testattribute");

    attribute.write("this is a testattribute");
    assert(H5Iis_valid(attribute.get_id()));

    attribute.close();
    assert(!H5Iis_valid(attribute.get_id()));

    attribute.open(*group, "testattribute");
    assert(H5Iis_valid(attribute.get_id()));

    attribute.close();

    attribute.open(*group, "pi");
    assert(attribute.get_id() == -1);
    attribute.write(3.14);

    assert(H5Iis_valid(attribute.get_id()));

    HDFAttribute attribute2(*group, "2pi");
    attribute2.write(2 * 3.14);
    assert(H5Iis_valid(attribute2.get_id()));
    attribute2.close();
    HDFAttribute attribute3(*group, "2pi");

    assert(!H5Iis_valid(attribute2.get_id()));
    assert(H5Iis_valid(attribute3.get_id()));
    auto val = attribute3.read<double>();
    assert(std::abs(std::get<1>(val) - 6.28) < 1e-16);

    // copy constructed
    HDFAttribute attribute_copied(attribute3);
    auto val2 = attribute_copied.read<double>();
    assert(std::abs(std::get<1>(val2) - 6.28) < 1e-16);
    assert(attribute_copied.get_name() == attribute3.get_name());
    assert(&attribute_copied.get_parent() == &attribute3.get_parent());

    // copy assigned
    HDFAttribute attribute_copyassigned = attribute_copied;
    assert(attribute_copied.get_name() == attribute_copyassigned.get_name());
    assert(&attribute_copied.get_parent() == &attribute_copyassigned.get_parent());

    // move constructed
    HDFAttribute attribute_moved(std::move(attribute_copied));
    assert(attribute_moved.get_name() == attribute3.get_name());
    assert(&attribute_moved.get_parent() == &attribute3.get_parent());

    // move assigned
    HDFAttribute attribute_moveassigned = std::move(attribute_moved);
    assert(attribute_moveassigned.get_name() == attribute3.get_name());
    assert(&attribute_moveassigned.get_parent() == &attribute3.get_parent());

    return 0;
}