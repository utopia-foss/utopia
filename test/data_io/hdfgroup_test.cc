/**
 * @brief In this file, the basic functionality of a HDFGroup is tested,
 *        by creating groups, checking if they exist independently,
 *        and adding attributes to them
 * @file hdfgroup_test.cc
 */

#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>

using namespace Utopia::DataIO;

bool check_exists_group(HDFFile& file, std::string path)
{
    herr_t status = H5Gget_objinfo(file.get_id(), path.c_str(), 0, NULL);

    if (status == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    HDFFile file("grouptest_file.h5", "w");

    // open file and read

    auto base_group = file.get_basegroup();
    auto group = base_group->open_group("first_deeper")
                     ->open_group("second_deeper/third_deeper");

    assert(check_exists_group(
               file, "/first_deeper/second_deeper/third_deeper") == true);
    base_group->delete_group("first_deeper/second_deeper/third_deeper");
    assert(check_exists_group(
               file, "/first_deeper/second_deeper/third_deeper") == false);

    // test for reference counting and correct resource management
    auto testgroup = base_group->open_group("/testgroup1/dummygroup");
    auto testgroup2 = base_group->open_group("/testgroup1/dummygroup");
    assert((*testgroup->get_referencecounter())[testgroup->get_address()] == 2);

    testgroup->add_attribute(
        "readme", "this group has been created for testing reference counter");
    testgroup->close();
    assert((*testgroup->get_referencecounter())[testgroup->get_address()] == 1);

    // check if the attributes are there
    testgroup->open(*base_group, "/testgroup1/dummygroup");
    assert(H5LTfind_attribute(testgroup->get_id(), "readme") == 1);

    testgroup->close();

    // check now if stuff can be done with the group
    testgroup2->add_attribute(
        "readme2",
        "because usually opening two objects and closing one of them "
        "released the resources of the other, too!");

    // check if the attributes are there
    assert(H5LTfind_attribute(testgroup2->get_id(), "readme2") == 1);

    return 0;
}
