#include "../hdffile.hh"
#include <cassert>
#include <iostream>

using namespace Utopia::DataIO;
int file_open_tester()
{
    auto file1 = std::make_shared<HDFFile>("hdf5testfile.h5", "w");
    auto file2 = std::make_shared<HDFFile>("hdf5testfile.h5", "r");
    auto file3 = std::make_shared<HDFFile>("hdf5testfile.h5", "r+");
    bool caught = false;
    try
    {
        HDFFile file4("hdf5testfile.h5", "x");
    }
    catch (const std::exception& e)
    {
        caught = true;
        std::cerr << e.what() << std::endl;
    }
    if (caught == false)
    {
        return -1;
    }
    auto file5 = std::make_shared<HDFFile>("hdftestfile.h5", "a");
    caught = false;
    try
    {
        HDFFile("hdftestfile.h5", "n");
    }
    catch (const std::exception& e)
    {
        caught = true;
        std::cerr << e.what() << std::endl;
    }
    if (caught == false)
    {
        return -1;
    }
    return 0;
}

int file_func_tester()
{
    auto file = std::make_shared<HDFFile>("hdf5testfile_func.h5", "w");

    // check that group opening works
    auto testgroup2 = file->open_group("/testgroup1/testgroup2");
    file->close();
    file = std::make_shared<HDFFile>("hdf5testfile_func.h5", "r+");
    hid_t group_test = H5Gopen(file->get_id(), "/testgroup1/testgroup2", H5P_DEFAULT);

    if (group_test < 0)
    {
        std::cerr << "group opening failed, does not exist at path" << std::endl;
        return -1;
    }
    H5Gclose(group_test);

    file->delete_group("/testgroup1/testgroup2");
    hid_t group_test2 = H5Gopen(file->get_id(), "/testgroup1/testgroup2", H5P_DEFAULT);

    // group may not exist anymore
    assert(group_test2 < 0);

    // check base group stuff
    auto basegroup = file->get_basegroup();
    assert(basegroup->get_path() == "/");

    return 0;
}

int main()
{
    int sentinel = 0;
    sentinel = file_open_tester();
    sentinel = file_func_tester();
    if (sentinel == 0)
    {
        std::cout << "SUCCESSFUL TEST RUN" << std::endl;
    }
    return sentinel;
}