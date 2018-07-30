/**
 * @brief In this file the functionality of HDFFile is tested.
 *
 * @file hdffile_test.cc
 */
#include "../hdffile.hh"
#include <cassert>
#include <dune/common/parallel/mpihelper.hh>
#include <iostream>

using namespace Utopia::DataIO;

// testing opening
int file_open_tester()
{
    bool caught = false;
    try
    {
        HDFFile file1("hdf5testfile.h5", "w");

        HDFFile file2("hdf5testfile.h5", "r");
        HDFFile file3("hdf5testfile.h5", "r+");
    }
    catch (const std::exception& e)
    {
        caught = true;
        std::cerr << e.what() << std::endl;
    }
    if (!caught)
    {
        return -1;
    }
    caught = false;
    HDFFile file4("hdf5testfile_test.h5", "w");
    file4.close();
    try
    {
        HDFFile file4("hdf5testfile_test.h5", "x");
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
    caught = false;
    HDFFile file5("hdf5testfile_test.h5", "a");

    try
    {
        HDFFile file6("hdf5testfile_test.h5", "grrr");
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

// testing functionality
int file_func_tester()
{
    HDFFile file("hdf5testfile_func.h5", "w");

    // check that group opening works
    auto testgroup2 = file.open_group("/testgroup1/testgroup2");
    file.close();
    file = HDFFile("hdf5testfile_func.h5", "r+");
    hid_t group_test = H5Gopen(file.get_id(), "/testgroup1/testgroup2", H5P_DEFAULT);

    if (group_test < 0)
    {
        std::cerr << "group opening failed, does not exist at path" << std::endl;
        return -1;
    }
    H5Gclose(group_test);

    file.delete_group("/testgroup1/testgroup2");
    hid_t group_test2 = H5Gopen(file.get_id(), "/testgroup1/testgroup2", H5P_DEFAULT);

    // group may not exist anymore
    assert(group_test2 < 0);

    // check base group stuff
    auto basegroup = file.get_basegroup();
    assert(basegroup->get_path() == "/");

    return 0;
}

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    int sentinel = 0;
    sentinel = file_open_tester();
    sentinel = file_func_tester();
    if (sentinel == 0)
    {
        std::cout << "SUCCESSFUL TEST RUN" << std::endl;
    }
    return sentinel;
}