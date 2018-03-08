#include "hdfdataset.hh"
#include "hdfmockclasses.hh"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace Utopia::DataIO;

// helper function for making a non compressed dataset
template <typename Datatype>
hid_t make_dataset_for_tests(hid_t id, std::string _name, hsize_t _rank,
                             std::vector<hsize_t> _extend,
                             std::vector<hsize_t> _max_extend,
                             hsize_t chunksize) {
    if (chunksize > 0) {

        // create creation property list, set chunksize and compress level
        hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(plist, _rank, &chunksize);

        // make dataspace
        hid_t dspace =
            H5Screate_simple(_rank, _extend.data(), _max_extend.data());

        // create dataset and return
        return H5Dcreate(id, _name.c_str(), HDFTypeFactory::type<Datatype>(),
                         dspace, H5P_DEFAULT, plist, H5P_DEFAULT);
    } else {
        // create dataset right away

        return H5Dcreate(
            id, _name.c_str(), HDFTypeFactory::type<Datatype>(),
            H5Screate_simple(_rank, _extend.data(), _max_extend.data()),
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
}

void write_dataset_onedimensional(HDFFile &file) {
    std::vector<double> data(100, 3.14);

    // 1d dataset tests
    HDFGroup testgroup1(file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(file.get_basegroup(), "/testgroup2");

    // Test for constructor

    // nothing happend until now
    HDFDataset<HDFGroup> testdataset(testgroup2, "testdataset");
    assert(testdataset.get_id() == -1);

    // make a dummy dataset to which later will be written
    hid_t dummy_dset = make_dataset_for_tests<double>(
        testgroup1.get_id(), "/testgroup1/testdataset2", 1, {100},
        {H5S_UNLIMITED}, 50);
    H5Dclose(dummy_dset);
    // open dataset again
    HDFDataset<HDFGroup> testdataset2(testgroup1, "testdataset2");
    hid_t dummy_dset2 =
        H5Dopen(file.get_id(), "/testgroup1/testdataset2", H5P_DEFAULT);
    // get its name
    std::string name;
    name.resize(25);
    H5Iget_name(dummy_dset2, name.data(), name.size());
    H5Dclose(dummy_dset2);
    name.pop_back(); // get rid of superfluous \0 hdf5 writes in there
    std::string dsetname = testdataset2.get_parent().lock()->get_path() + "/" +
                           testdataset2.get_name();
    // check if name is correct
    assert(name == dsetname);

    // test write: most simple case
    testdataset.write(data.begin(), data.end(),
                      [](auto &value) { return value; });

    for (auto &value : data) {
        value = 6.28;
    }
    testdataset2.write(data.begin(), data.end(),
                       [](auto &value) { return value; });

    HDFDataset<HDFGroup> compressed_dataset(testgroup1, "compressed_dataset");
    compressed_dataset.write(data.begin(), data.end(),
                             [](auto &value) { return value; }, 1,
                             {data.size()}, {}, 20, 5);

    compressed_dataset.close();

    HDFDataset<HDFGroup> compressed_dataset2(testgroup1, "compressed_dataset");

    for (auto &value : data) {
        value = 3.14 / 2;
    }
    compressed_dataset2.write(data.begin(), data.end(),
                              [](auto &value) { return value; });

    // 2d data
    std::vector<std::vector<double>> data_2d(100, std::vector<double>(10));

    // 1d dataset variable length

    HDFDataset<HDFGroup> varlen_dataset(testgroup1, "varlendataset");
    varlen_dataset.write(
        data_2d.begin(), data_2d.end(),
        [](auto &value) -> std::vector<double> & { return value; }, 1, {100});

    // nd dataset, extendible
}

void write_dataset_multidimensional(HDFFile &file) {
    HDFGroup multidimgroup(file.get_basegroup(), "/multi_dim_data");
    std::vector<double> data(100, 2.718);

    HDFDataset<HDFGroup> multidimdataset(multidimgroup, "multiddim_dataset");
    multidimdataset.write(data.begin(), data.end(),
                          [](auto &value) { return value; }, 2, {1, 100});

    HDFDataset<HDFGroup> multidimdataset_compressed(
        multidimgroup, "multiddim_dataset_compressed");

    std::for_each(data.begin(), data.end(),
                  [](auto &value) { return value += 1; });
    multidimdataset_compressed.write(data.begin(), data.end(),
                                     [](auto &value) { return value; }, 2,
                                     {1, 100}, {}, 50, 5);
}

void read_dataset_tests(HDFFile &file) {
    HDFGroup testgroup1(file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(file.get_basegroup(), "/testgroup2");
    HDFDataset<HDFGroup> testdataset(testgroup2, "testdataset");
    HDFDataset<HDFGroup> testdataset2(testgroup1, "testdataset2");
    HDFDataset<HDFGroup> compressed_dataset(testgroup1, "compressed_dataset");
    HDFGroup multidimgroup(file.get_basegroup(), "/multi_dim_data");
    HDFDataset<HDFGroup> multidimdataset(multidimgroup, "/multiddim_dataset");
}
int main() {
    HDFFile file("/Users/haraldmack/Desktop/dataset_test.h5", "w");

    write_dataset_onedimensional(file);

    write_dataset_multidimensional(file);

    read_dataset_tests(file);

    return 0;
}
