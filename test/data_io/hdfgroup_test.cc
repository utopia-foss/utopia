/**
 * @brief In this file, the basic functionality of a HDFGroup is tested,
 *        by creating groups, checking if they exist independently,
 *        and adding attributes to them
 * @file hdfgroup_test.cc
 */

#include <cassert>
#include <iostream>

#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdfutilities.hh>

#include "testtools.hh"

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

template<class HDFObject>
bool check_path_exists(std::shared_ptr<HDFObject> obj, std::string path)
{
    return path_exists(obj->get_id(), path) > 0;
}


int main()
{
    std::cout << "Opening a test file ..." << std::endl;

    HDFFile file("grouptest_file.h5", "w");

    std::cout << "Success." << std::endl << std::endl;


    // Open some groups
    std::cout << "Opening some nested groups ..." << std::endl;

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

    std::cout << "Success." << std::endl << std::endl;


    // Test attribute creation
    std::cout << "Testing attribute creation ..." << std::endl;

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

    std::cout << "Success." << std::endl << std::endl;


    // -- Test the path_exists function from hdfutilities.hh ------------------
    // It is more convenient to do here than in hdfutilities_test.cc ...

    std::cout << "Test path_exists function ..." << std::endl;

    // Attach string buffer to make sure that HDF5 C library does not write
    // any error messages
    std::streambuf* coutbuf = std::cout.rdbuf();
    Savebuf sbuf(coutbuf);
    std::cout.rdbuf(&sbuf);
    
    // Relative paths should work
    assert(check_path_exists(base_group, "first_deeper"));

    // Should work with absolute path as well
    assert(check_path_exists(base_group, "/first_deeper"));
    assert(check_path_exists(base_group, "/"));  // root should exist as well

    // Final slash behaves the same as in H5Lexists
    assert(H5Lexists(base_group->get_id(), "/first_deeper/", H5P_DEFAULT) > 0);
    assert(check_path_exists(base_group, "/first_deeper/"));

    // But not with obviously nonexistant paths
    assert(not check_path_exists(base_group, "../first_deeper"));
    assert(not check_path_exists(base_group, "i_do_not_exist"));
    assert(not check_path_exists(base_group, "/i_do_not_exist"));

    // third_deeper was deleted, should not exist
    assert(not check_path_exists(base_group,
                                 "first_deeper/second_deeper/third_deeper"));
    assert(not check_path_exists(base_group->open_group("first_deeper"),
                                 "second_deeper/third_deeper"));
    
    // second_deeper should still exist, though; created on the way
    assert(check_path_exists(base_group,
                             "first_deeper/second_deeper"));
    assert(check_path_exists(base_group->open_group("first_deeper"),
                             "second_deeper"));

    // Same behaviour as H5Lexists wrt. relative paths that go towards the
    // parent ...
    auto grp_sec_deep = base_group->open_group("first_deeper/second_deeper");
    
    assert(H5Lexists(grp_sec_deep->get_id(), "..", H5P_DEFAULT) == 0);
    assert(not check_path_exists(grp_sec_deep, ".."));

    assert(H5Lexists(grp_sec_deep->get_id(), "../", H5P_DEFAULT) == 0);
    assert(not check_path_exists(grp_sec_deep, "../"));

    // Read and store the stream buffer
    const auto output = sbuf.str();

    // Can now restore the original stream buffer
    std::cout.rdbuf(coutbuf);
    
    // Now, make sure no errors were emitted
    std::cout << "  captured stream output (expected empty): " << output
              << std::endl;
    assert(output.length() == 0);

    std::cout << "Success." << std::endl << std::endl;
    

    std::cout << "Total success." << std::endl << std::endl;
    return 0;
}
