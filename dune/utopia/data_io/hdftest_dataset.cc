#include "hdfdataset.hh"
#include "hdfmockclasses.hh"
#include <cassert>
#include <iostream>
#include <vector>
using namespace Utopia::DataIO;

int main() {
    HDFFile file("/Users/haraldmack/Desktop/dataset_test.h5", "w");
    HDFGroup testgroup1(file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(file.get_basegroup(), "/testgroup2");

    // Test for constructor

    // nothing happend until now
    HDFDataset<HDFGroup> testdataset(testgroup2, "testdataset");
    assert(testdataset.get_id() == -1);

    // make a dummy dataset
    hsize_t size = 100;
    hid_t dummy_space = H5Screate_simple(1, &size, nullptr);
    hid_t dummy_dset =
        H5Dcreate(testgroup1.get_id(), "dummy_dataset", H5T_NATIVE_INT,
                  dummy_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Sclose(dummy_space);
    H5Dclose(dummy_dset);

    // open dataset again
    HDFDataset<HDFGroup> dummy_dataset(testgroup1, "dummy_dataset");
    hid_t dummy_dset2 =
        H5Dopen(file.get_id(), "/testgroup1/dummy_dataset", H5P_DEFAULT);
    // get its name
    std::string name;
    name.resize(26);
    H5Iget_name(dummy_dset2, name.data(), name.size());
    H5Dclose(dummy_dset2);
    std::string dsetname = dummy_dataset.get_parent().lock()->get_path() + "/" +
                           dummy_dataset.get_name() +
                           '\0'; // hdf5 C lib. reads 0 char!
    // check if name is correct
    assert(name == dsetname);

    return 0;
}
