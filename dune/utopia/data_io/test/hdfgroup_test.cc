#include "../hdfgroup.hh"
#include "../hdfmockclasses.hh"
#include <cassert>
#include <iostream>
#include <hdf5.h>

using namespace Utopia::DataIO;

void throw_stuff ()
{
    throw std::runtime_error("Bullshit");
}

/// Create Test data
std::vector<double> create_test_data()
{
    std::vector<double> data(100, 2);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] += i;
    }
    return data;
}

/// Test HDFGroup class
int main() 
{
    // open a file
    HDFFile file("group_test.h5", "w");

    // open group 
    auto group = HDFGroup(file.get_basegroup(), "testgroup");
    assert(H5Lexists(file.get_id(), "testgroup", H5P_DEFAULT ) == 1);
    
    // open subtestgroup and check whether it exists
    std::shared_ptr<HDFGroup> subgroup = group.open_group("subtestgroup");
    assert(H5Lexists(group.get_id(), "subtestgroup", H5P_DEFAULT ) == 1);
    std::shared_ptr<HDFGroup> subgroup2 = group.open_group("subtestgroup/a/b/c");
    assert(H5Lexists(group.get_id(), "subtestgroup/a/b/c", H5P_DEFAULT ) == 1);

    // test delete_group function
    group.delete_group("subtestgroup/a/b");
    assert(H5Lexists(group.get_id(), "subtestgroup/a/b", H5P_DEFAULT ) == 0);

    // open subsubtestgroup and check whether it exists
    std::shared_ptr<HDFGroup> subsubtestgroup = subgroup->open_group("subsubtestgroup2");
    assert(H5Lexists(subgroup->get_id(), "subsubtestgroup2", H5P_DEFAULT ) == 1);
    
    // open testdataset, write some data and assert that it exists
    std::shared_ptr<HDFDataset<HDFGroup>> dataset = group.open_dataset("testdataset");
    auto data = create_test_data();
    dataset->write(data.begin(), data.end(), [](auto &value) { return value; });
    assert(H5Lexists(group.get_id(), "testdataset", H5P_DEFAULT ) == 1);

    // open group and close it. Check that it does not exist any more
    auto group2 = HDFGroup(file.get_basegroup(), "testgroup2");
    group2.close();
    assert(H5Lexists(file.get_id(), "group2", H5P_DEFAULT) != 1);

    group.info();

    return 0;   
}