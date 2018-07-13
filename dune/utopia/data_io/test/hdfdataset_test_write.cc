/**
 * @brief Tests writing of data to datasets for mulitple kinds of data
 *
 * @file hdfdataset_test_write.cc
 */
#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace Utopia::DataIO;

// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

// helper function for making a non compressed dataset
template <typename Datatype>
hid_t make_dataset_for_tests(hid_t id,
                             std::string _name,
                             hsize_t _rank,
                             std::vector<hsize_t> _extend,
                             std::vector<hsize_t> _max_extend,
                             hsize_t chunksize)
{
    if (chunksize > 0)
    {
        // create creation property list, set chunksize and compress level
        hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(plist, _rank, &chunksize);

        // make dataspace
        hid_t dspace = H5Screate_simple(_rank, _extend.data(), _max_extend.data());

        // create dataset and return
        return H5Dcreate(id, _name.c_str(), HDFTypeFactory::type<Datatype>(),
                         dspace, H5P_DEFAULT, plist, H5P_DEFAULT);
    }
    else
    {
        // create dataset right away

        return H5Dcreate(id, _name.c_str(), HDFTypeFactory::type<Datatype>(),
                         H5Screate_simple(_rank, _extend.data(), _max_extend.data()),
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
}

void write_dataset_onedimensional(HDFFile& file)
{
    // 1d dataset tests
    HDFGroup testgroup1(*file.get_basegroup(), "/testgroup1");
    HDFGroup testgroup2(*file.get_basegroup(), "/testgroup2");

    // Test for constructor

    // nothing happend until now
    HDFDataset testdataset(testgroup2, "testdataset", 1);
    assert(testdataset.get_id() == -1);

    // make a dummy dataset to which later will be written
    hid_t dummy_dset = make_dataset_for_tests<double>(
        testgroup1.get_id(), "/testgroup1/testdataset2", 1, {100}, {H5S_UNLIMITED}, 50);
    H5Dclose(dummy_dset);
    // open dataset again
    HDFDataset testdataset2(testgroup1, "testdataset2");
    hid_t dummy_dset2 = H5Dopen(file.get_id(), "/testgroup1/testdataset2", H5P_DEFAULT);
    // get its name
    std::string name;
    name.resize(25);
    H5Iget_name(dummy_dset2, name.data(), name.size());
    H5Dclose(dummy_dset2);
    name.pop_back(); // get rid of superfluous \0 hdf5 writes in there

    std::string dsetname =
        testdataset2.get_parent().get_path() + "/" + testdataset2.get_path();
    // check if name is correct
    assert(name == dsetname);

    std::vector<double> data(100, 3.14);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] += i;
    }
    // test write: most simple case
    testdataset.write(data.begin(), data.end(), [](auto& value) { return value; });

    for (auto& value : data)
    {
        value = 6.28;
    }
    testdataset2.write(data.begin(), data.end(),
                       [](auto& value) { return value; });

    HDFDataset compressed_dataset(testgroup1, "compressed_dataset", 1,
                                  {5 * data.size()}, {10}, 5);
    compressed_dataset.write(data.begin(), data.end(),
                             [](auto& value) { return value; });

    compressed_dataset.close();

    HDFDataset compressed_dataset2(testgroup1, "compressed_dataset", 1,
                                   {5 * data.size()}, {10}, 5);

    for (auto& value : data)
    {
        value = 3.14 / 2;
    }
    compressed_dataset2.write(data.begin(), data.end(),
                              [](auto& value) { return value; });

    // 1d dataset variable length
    std::vector<std::vector<double>> data_2d(100, std::vector<double>(10, 3.16));

    HDFDataset varlen_dataset(testgroup1, "varlendataset");
    varlen_dataset.write(
        data_2d.begin(), data_2d.end(),
        [](auto& value) -> std::vector<double>& { return value; });

    // multiple objects refer to the same dataset
    HDFGroup multirefgroup(*file.get_basegroup(), "multiref_test");
    HDFDataset multirefdataset1(multirefgroup, "multirefdataset", 1,
                                {5 * data.size()}, {20});

    data = std::vector<double>(100, 3.14);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] += i;
    }

    // write some stuff to multirefdataset1
    multirefdataset1.write(data.begin(), data.end(),
                           [](auto& value) { return value; });

    std::string attr1 = "First attribute to multiple reference dataset";
    multirefdataset1.add_attribute("Attribute1", attr1);

    HDFDataset multirefdataset2(multirefgroup, "multirefdataset", 1,
                                {5 * data.size()}, {20});

    assert((*multirefdataset1.get_referencecounter())[multirefdataset1.get_address()] == 2);
    assert((*multirefdataset2.get_referencecounter())[multirefdataset1.get_address()] == 2);

    multirefdataset1.close();

    assert((*multirefdataset2.get_referencecounter())[multirefdataset1.get_address()] == 1);
    assert((*multirefdataset1.get_referencecounter())[multirefdataset1.get_address()] == 1);

    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] += i + 100;
    }
    // write some stuff to multirefdataset2
    multirefdataset2.write(data.begin(), data.end(),
                           [](auto& value) { return value; });
    std::string attr2 = "Second attribute to multirefdataset";
    multirefdataset2.add_attribute("Attribute2", attr2);

    std::vector<Point> points(100, Point{3., 4., 5.});

    // writing rvalue to dataset
    HDFDataset rvaluedataset(*file.get_basegroup(), "/rvalueset");

    rvaluedataset.write(points.begin(), points.end(), [](auto& pt) {
        return std::vector<double>{pt.x, pt.y, pt.z};
    });

    HDFDataset fixedsizearr_dataset(*file.get_basegroup(),
                                    "/fixedsize_array_set");

    fixedsizearr_dataset.write(points.begin(), points.end(), [](auto& pt) {
        return std::array<double, 3>{pt.x, pt.y, pt.z};
    });
}

void write_dataset_multidimensional(HDFFile& file)
{
    std::cout << "multidim datatest" << std::endl;
    HDFGroup multidimgroup(*file.get_basegroup(), "/multi_dim_data");
    std::vector<double> data(100, 2.718);

    HDFDataset multidimdataset(multidimgroup, "multiddim_dataset", 2);
    multidimdataset.write(data.begin(), data.end(),
                          [](auto& value) { return value; });

    HDFDataset multidimdataset_compressed(
        multidimgroup, "multiddim_dataset_compressed", 2, {2, 100}, {1, 10}, 5);

    std::for_each(data.begin(), data.end(),
                  [](auto& value) { return value += 1; });
    multidimdataset_compressed.write(data.begin(), data.end(),
                                     [](auto& value) { return value; });

    multidimdataset.close();

    HDFDataset multidimdataset_extendable(
        multidimgroup, "multiddim_dataset_extendable", 2,
        {H5S_UNLIMITED, H5S_UNLIMITED}, {1, 10}, 5);

    double writeval = 100;
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] = writeval;
        writeval += 1;
    }

    multidimdataset_extendable.write(data.begin(), data.end(),
                                     [](auto& value) { return value; });

    multidimdataset_extendable.close();

    HDFDataset multidimdataset_reopened(multidimgroup,
                                        "multiddim_dataset_extendable");

    double value = 200;
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] = value;
        value += 1;
    }
    multidimdataset_reopened.write(data.begin(), data.end(),
                                   [](auto& value) { return value; });
}

int main()
{
    HDFFile file("dataset_test.h5", "w");

    write_dataset_onedimensional(file);

    write_dataset_multidimensional(file);

    return 0;
}
