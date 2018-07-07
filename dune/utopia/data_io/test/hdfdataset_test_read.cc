/**
 * @brief Tests reading of data from datasets for mulitple kinds of data.
 *        Executing hdfdataset_test_write.cc is a prerequisite for this file
 *        to work
 * @file hdfdataset_test_read.cc
 */
#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace Utopia::DataIO;
struct Point
{
    double x;
    double y;
    double z;
};

void read_dataset_tests(HDFFile& file)
{
    HDFGroup testgroup1(*file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(*file.get_basegroup(), "/testgroup2");
    HDFGroup multidimgroup(*file.get_basegroup(), "/multi_dim_data");

    HDFDataset testdataset(testgroup2, "testdataset");
    HDFDataset testdataset2(testgroup1, "testdataset2");
    HDFDataset compressed_dataset(testgroup1, "compressed_dataset");
    HDFDataset multidimdataset(multidimgroup, "multiddim_dataset");
    HDFDataset multidimdataset_compressed(multidimgroup,
                                          "multiddim_dataset_compressed");
    HDFDataset multidimdataset_extendable(multidimgroup,
                                          "multiddim_dataset_extendable");

    HDFDataset rvaluedataset(*file.get_basegroup(), "rvalueset");
    HDFDataset varlen_dataset(testgroup1, "varlendataset");

    // read entire 1d dataset
    std::vector<double> data(100, 3.14);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] += i;
    }
    auto read_data = testdataset.read<double>();

    assert(data.size() == read_data.size());
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        assert(std::abs(data[i] - read_data[i]) < 1e-16);
    }
    // read subset of 1d set
    hsize_t start = 10;
    hsize_t end = 40;
    hsize_t stride = 2;
    auto read_subset = testdataset.read<double>({start}, {end}, {stride});
    std::size_t j = 0;
    for (std::size_t i = start; i < end; i += 2, ++j)
    {
        assert(std::abs(read_subset[j] - data[i]) < 1e-16);
    }

    // read entire 2d dataset, flattened
    std::vector<double> flat_2d(200);
    std::vector<double> twod(100, 2.718);
    std::vector<double> twod_comp(100, 3.718);

    double writeval = 100;
    for (std::size_t i = 0; i < 100; ++i)
    {
        flat_2d[i] = writeval;
        writeval += 1;
    }

    double value = 200;
    for (std::size_t i = 100; i < 200; ++i)
    {
        flat_2d[i] = value;
        value += 1;
    }
    auto multidim_simplebuffer = multidimdataset.read<double>();

    assert(twod.size() == multidim_simplebuffer.size());
    for (std::size_t i = 0; i < twod.size(); ++i)
    {
        assert(std::abs(multidim_simplebuffer[i] - twod[i]) < 1e-16);
    }

    // read subset from 1 line 2d set
    std::vector<hsize_t> start2d = {0, 10};
    std::vector<hsize_t> end2d = {1, 40};
    std::vector<hsize_t> stride2d = {1, 2};
    auto read_subset2d = multidimdataset.read<double>(start2d, end2d, stride2d);
    j = 0;
    // read subset from 2 line 2d set

    for (std::size_t i = start; i < end; i += 2, ++j)
    {
        assert(std::abs(read_subset2d[j] - twod[i]) < 1e-16);
    }
    start2d = {0, 5};
    end2d = {2, 50};
    stride2d = {1, 5};
    auto read_subset2d_2 =
        multidimdataset_extendable.read<double>(start2d, end2d, stride2d);
    // test that the data is correctly read
    j = 0;
    for (std::size_t i = 5; i < end2d[1]; i += 5, ++j)
    {
        assert(std::abs(read_subset2d_2[j] - flat_2d[i]) < 1e-16);
    }
    for (std::size_t i = 100 + start2d[1]; i < 100 + end2d[1]; i += 5, ++j)
    {
        assert(std::abs(read_subset2d_2[j] - flat_2d[i]) < 1e-16);
    }

    data = std::vector<double>(100, 3.14);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] += i;
    }
    // check that multirefdataset worked as expected
    HDFGroup multirefgroup(*file.get_basegroup(), "multiref_test");
    HDFDataset multirefdataset(multirefgroup, "multirefdataset");

    std::vector<double> multirefdata(200, 3.14);
    for (std::size_t i = 0; i < 200; ++i)
    {
        multirefdata[i] += i;
    }

    auto datavector = multirefdataset.read<double>();

    assert(datavector.size() == multirefdata.size());

    for (std::size_t i = multirefdata.size(); i < multirefdata.size(); ++i)
    {
        assert(std::abs(datavector[i] - multirefdata[i]) < 1e-16);
    }
    std::string attr1 = "First attribute to multiple reference dataset";

    std::string attr2 = "Second attribute to multirefdataset";

    HDFAttribute multirefattribute(multirefdataset, "Attribute1");
    auto [shape1, multirefattrdata1] = multirefattribute.read<std::string>();
    assert(shape1.size() == 1);
    assert(shape1[0] == 1);
    assert(multirefattrdata1 == attr1);

    HDFAttribute multirefattribute2(multirefdataset, "Attribute2");
    auto [shape2, multirefattrdata2] = multirefattribute2.read<std::string>();
    assert(multirefattrdata2 == attr2);
    assert(shape2.size() == 1);
    assert(shape2[0] == 1);

    // read from varlendataset
    std::vector<std::vector<double>> data_2d(100, std::vector<double>(10, 3.16));

    auto varlendata = varlen_dataset.read<std::vector<double>>();
    assert(varlendata.size() == 100);
    for (std::size_t l = 0; l < 100; ++l)
    {
        assert(varlendata[l].size() == 10);
        for (std::size_t k = 0; k < 10; ++k)
        {
            assert(std::abs(varlendata[l][k] - 3.16) < 1e-16);
        }
    }

    // read from rvalue dataset
    std::vector<Point> points(100, Point{3., 4., 5.});

    auto ptvec = rvaluedataset.read<std::vector<double>>();

    assert(ptvec.size() == 100);
    for (std::size_t l = 0; l < 100; ++l)
    {
        assert(ptvec[l].size() == 3);
        assert(std::abs(points[l].x - ptvec[l][0]) < 1e-16);
        assert(std::abs(points[l].y - ptvec[l][1]) < 1e-16);
        assert(std::abs(points[l].z - ptvec[l][2]) < 1e-16);
    }
}

int main()
{
    HDFFile file("dataset_test.h5", "r");
    read_dataset_tests(file);

    return 0;
}
