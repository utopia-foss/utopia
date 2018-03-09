#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace Utopia::DataIO;

void read_dataset_tests(HDFFile &file) {
    HDFGroup testgroup1(*file.get_basegroup(), file, "/testgroup1");
    HDFGroup testgroup2(*file.get_basegroup(), file, "/testgroup2");
    HDFGroup multidimgroup(*file.get_basegroup(), file, "/multi_dim_data");

    HDFDataset<HDFGroup, HDFFile> testdataset(testgroup2, file, "testdataset");
    HDFDataset<HDFGroup, HDFFile> testdataset2(testgroup1, file,
                                               "testdataset2");
    HDFDataset<HDFGroup, HDFFile> compressed_dataset(testgroup1, file,
                                                     "compressed_dataset");
    HDFDataset<HDFGroup, HDFFile> multidimdataset(multidimgroup, file,
                                                  "multiddim_dataset");
    HDFDataset<HDFGroup, HDFFile> multidimdataset_compressed(
        multidimgroup, file, "multiddim_dataset_compressed");
    HDFDataset<HDFGroup, HDFFile> multidimdataset_extendable(
        multidimgroup, file, "multiddim_dataset_extendable");

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
    // read subset of 1d set
    hsize_t start = 10;
    hsize_t end = 40;
    hsize_t stride = 2;
    auto read_subset = testdataset.read<double>({start}, {end}, {stride});
    std::size_t j = 0;
    for (std::size_t i = start; i < end; i += 2, ++j) {
        assert(std::abs(read_subset[j] - data[i]) < 1e-16);
    }

    // read entire 2d dataset, flattened
    std::vector<double> flat_2d(200);
    std::vector<double> twod(100, 2.718);
    std::vector<double> twod_comp(100, 3.718);

    double writeval = 100;
    for (std::size_t i = 0; i < 100; ++i) {
        flat_2d[i] = writeval;
        writeval += 1;
    }

    double value = 200;
    for (std::size_t i = 100; i < 200; ++i) {
        flat_2d[i] = value;
        value += 1;
    }
    auto multidim_simplebuffer = multidimdataset.read<double>();

    assert(twod.size() == multidim_simplebuffer.size());
    for (std::size_t i = 0; i < twod.size(); ++i) {
        assert(std::abs(multidim_simplebuffer[i] - twod[i]) < 1e-16);
    }

    // read subset from 1 line 2d set
    std::vector<hsize_t> start2d = {0, 10};
    std::vector<hsize_t> end2d = {1, 40};
    std::vector<hsize_t> stride2d = {1, 2};
    auto read_subset2d = multidimdataset.read<double>(start2d, end2d, stride2d);
    j = 0;
    // read subset from 2 line 2d set

    for (std::size_t i = start; i < end; i += 2, ++j) {
        assert(std::abs(read_subset2d[j] - twod[i]) < 1e-16);
    }
    start2d = {0, 5};
    end2d = {2, 50};
    stride2d = {1, 5};
    auto read_subset2d_2 =
        multidimdataset_extendable.read<double>(start2d, end2d, stride2d);
    // test that the data is correctly read
    j = 0;
    for (std::size_t i = 5; i < end2d[1]; i += 5, ++j) {
        assert(std::abs(read_subset2d_2[j] - flat_2d[i]) < 1e-16);
    }
    for (std::size_t i = 100 + start2d[1]; i < 100 + end2d[1]; i += 5, ++j) {
        assert(std::abs(read_subset2d_2[j] - flat_2d[i]) < 1e-16);
    }
}

int main() {
    HDFFile file("dataset_test.h5", "r");
    read_dataset_tests(file);

    return 0;
}
