#include "hdfdataset.hh"
#include "hdfmockclasses.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace Utopia::DataIO;

void read_dataset_tests(HDFFile &file) {
    HDFGroup testgroup1(file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(file.get_basegroup(), "/testgroup2");
    HDFGroup multidimgroup(file.get_basegroup(), "/multi_dim_data");

    HDFDataset<HDFGroup> testdataset(testgroup2, "testdataset");
    HDFDataset<HDFGroup> testdataset2(testgroup1, "testdataset2");
    HDFDataset<HDFGroup> compressed_dataset(testgroup1, "compressed_dataset");
    HDFDataset<HDFGroup> multidimdataset(multidimgroup, "multiddim_dataset");
    HDFDataset<HDFGroup> multidimdataset_compressed(
        multidimgroup, "multiddim_dataset_compressed");
    HDFDataset<HDFGroup> multidimdataset_extendable(
        multidimgroup, "multiddim_dataset_extendable");

    // read entire 1d dataset
    std::vector<double> data(100, 3.14);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] += i;
    }
    auto read_data = testdataset.read<double>();

    assert(data.size() == read_data.size());
    for (std::size_t i = 0; i < data.size(); ++i) {
        assert(std::abs(data[i] - read_data[i]) < 1e-16);
    }
    hsize_t start = 10;
    hsize_t end = 40;
    hsize_t stride = 2;
    auto read_subset = testdataset.read<double>({10}, {40}, {stride});
    std::cout << "read_subset size: " << read_subset.size() << std::endl;
    std::size_t j = 0;
    for (std::size_t i = start; i < end; i += 2, ++j) {
        assert(std::abs(read_subset[j]-data[i]) < 1e-16);
    }
    std::cout << std::endl;
}

int main() {
    HDFFile file("/Users/haraldmack/Desktop/dataset_test.h5", "r");
    read_dataset_tests(file);

    return 0;
}
