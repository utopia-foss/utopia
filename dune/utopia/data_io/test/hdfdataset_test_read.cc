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
#include <chrono>
#include <iostream>
#include <thread>
using namespace Utopia::DataIO;
using hsizevec = std::vector<hsize_t>;

struct Point
{
    double x;
    double y;
    double z;
};

int main()
{
    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    HDFFile file("datatset_testfile.h5", "r");
    auto contset = file.open_dataset("/containerdataset");
    auto nestedcontset = file.open_dataset("/containercontainerdataset");
    auto stringset = file.open_dataset("/stringdataset");
    auto ptrset = file.open_dataset("/pointerdataset");
    auto scalarset = file.open_dataset("/scalardataset");
    auto twoDdataset = file.open_dataset("/2ddataset");
    auto adapteddataset = file.open_dataset("/adapteddataset");
    auto fireandforgetdataset = file.open_dataset("/fireandforget");
    auto fireandforgetdataset2d = file.open_dataset("/fireandforget2d");

    // check that parameters are read out correctly
    assert(contset->get_capacity() == hsizevec{100});
    assert(nestedcontset->get_capacity() == hsizevec{100});
    assert(stringset->get_capacity() == hsizevec{100});
    assert(ptrset->get_capacity() == hsizevec{100});
    assert(scalarset->get_capacity() == hsizevec{100});
    assert(twoDdataset->get_capacity() == (hsizevec{10, 100}));
    assert(adapteddataset->get_capacity() == (hsizevec{3, 100}));
    assert(fireandforgetdataset->get_capacity() == hsizevec{H5S_UNLIMITED});
    assert(fireandforgetdataset2d->get_capacity() == (hsizevec{5, 100}));

    assert(contset->get_current_extent() == hsizevec{30});
    assert(nestedcontset->get_current_extent() == hsizevec{40});
    assert(stringset->get_current_extent() == hsizevec{26});
    assert(ptrset->get_current_extent() == hsizevec{15});
    assert(scalarset->get_current_extent() == hsizevec{5});
    assert(twoDdataset->get_current_extent() == (hsizevec{6, 100}));
    assert(adapteddataset->get_current_extent() == (hsizevec{3, 100}));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{50});
    assert(fireandforgetdataset2d->get_current_extent() == (hsizevec{5, 100}));

    assert(contset->get_chunksizes() == hsizevec{5});
    assert(nestedcontset->get_chunksizes() == hsizevec{5});
    assert(stringset->get_chunksizes() == hsizevec{5});
    assert(ptrset->get_chunksizes() == hsizevec{5});
    assert(scalarset->get_chunksizes() == hsizevec{5});
    assert(twoDdataset->get_chunksizes() == (hsizevec{1, 5}));
    assert(adapteddataset->get_chunksizes() == (hsizevec{1, 10}));
    // fireandforget & fireandforget2d unknown...

    // offset should be at end of data currently contained
    assert(contset->get_offset() == hsizevec{30});
    assert(nestedcontset->get_offset() == hsizevec{40});
    assert(stringset->get_offset() == hsizevec{26});
    assert(ptrset->get_offset() == hsizevec{15});
    assert(scalarset->get_offset() == hsizevec{5});
    assert(twoDdataset->get_offset() == (hsizevec{6, 100}));
    assert(adapteddataset->get_offset() == (hsizevec{3, 100}));
    assert(fireandforgetdataset->get_offset() == hsizevec{50});
    assert(fireandforgetdataset2d->get_offset() == (hsizevec{5, 100}));

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE EXPECTED DATA TO TEST AGAINST  ////////////////
    ///////////////////////////////////////////////////////////////////////////
    // ... for simple container
    std::vector<double> contdata;
    contdata.insert(contdata.begin(), 10, 3.14);
    contdata.insert(contdata.begin() + 10, 10, 6.28);
    contdata.insert(contdata.begin() + 20, 10, 9.42);

    std::vector<double> partial_contdata(10);
    std::size_t j = 0;
    for (std::size_t i = 5; i < 25; i += 2, ++j)
    {
        partial_contdata[j] = contdata[i];
    }

    // ... for nested container
    std::array<int, 4> arr{{0, 1, 2, 3}};
    std::array<int, 4> arr2{{4, 5, 6, 7}};

    std::vector<std::array<int, 4>> nestedcontdata(20, arr);
    nestedcontdata.insert(nestedcontdata.begin() + 20, 20, arr2);

    std::vector<std::array<int, 4>> partial_nestedcontdata(10);
    j = 0;
    for (std::size_t i = 0; i < 30; i += 3, ++j)
    {
        partial_nestedcontdata[j] = nestedcontdata[i];
    }

    // ... for 2d dataset
    std::vector<std::vector<double>> twoddata(6, std::vector<double>());
    for (std::size_t i = 0; i < 6; ++i)
    {
        twoddata[i].insert(twoddata[i].begin(), 100, i);
    }

    std::vector<std::vector<double>> partial_twoddata(2, std::vector<double>());
    for (std::size_t i = 0; i < 2; ++i)
    {
        partial_twoddata[i].insert(partial_twoddata[i].begin(), 50, i + 2);
    }
    // ... for stringdata
    std::vector<std::string> stringcontainerdata{
        "testsstring", "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
        "8",           "9",  "10", "11", "12", "13", "14", "15", "16",
        "17",          "18", "19", "20", "21", "22", "23", "24"};

    for (auto& val : stringcontainerdata)
    {
        val.resize(11); // complete size
    }

    // ... for stringdata read into one single string
    std::string onestringdata;
    for (auto& str : stringcontainerdata)
    {
        onestringdata += str;
    }

    // ... for pointer dataset
    std::vector<double> ptrdata{3.14, 3.14, 3.14, 3.14, 3.14, 6.28, 6.28, 6.28,
                                6.28, 6.28, 9.42, 9.42, 9.42, 9.42, 9.42};

    std::vector<double> partial_ptrdata(7);
    for (std::size_t i = 0; i < 7; ++i)
    {
        partial_ptrdata[i] = ptrdata[i + 5];
    }

    // ... for adaptedset
    std::vector<Point> adapteddata(100);
    for (int i = 0; i < 100; ++i)
    {
        adapteddata[i].x = 3.14;
        adapteddata[i].y = 3.14 + 1;
        adapteddata[i].z = 3.14 + 2;
    }

    // ... for fireandforgetdataset
    std::vector<int> fireandforgetdata(50);
    for (std::size_t i = 0; i < 5; ++i)
    {
        std::generate(fireandforgetdata.begin() + i * 10,
                      fireandforgetdata.begin() + (i + 1) * 10,
                      [&]() -> int { return i + 1; });
    }

    // make expected data for fireandforget2d -> 1d vector of size 500, which
    // represents 2d data of dimensions [5, 100]
    std::vector<int> fireandforgetdata2d(500);
    for (std::size_t i = 0; i < 5; ++i)
    {
        std::generate(fireandforgetdata2d.begin() + i * 100,
                      fireandforgetdata2d.begin() + (i + 1) * 100,
                      [&]() -> int { return i + 1; });
    }

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////// FULL READING TAKES PLACE NOW ////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // README: as we tested offset mechanics already, and offset is not needed
    // for reading an entire dataset, the value offset is not tested here
    // as it does not change when reading an entire dataset

    // read simple container data
    auto [contshape, read_contdata] = contset->read<std::vector<double>>();
    assert(contshape.size() == 1);
    assert(contshape[0] = 30);
    assert(contdata == read_contdata);

    // read nested container data
    auto [nestedcontshape, read_nestedcontdata] =
        nestedcontset->read<std::vector<std::array<int, 4>>>();
    assert(nestedcontshape.size() == 1);
    assert(nestedcontshape[0] = 40);
    assert(read_nestedcontdata.size() == 40);
    assert(nestedcontdata == read_nestedcontdata);

    // read stringdataset
    auto [stringcontainershape, read_stringcontainerdata] =
        stringset->read<std::vector<std::string>>();
    assert(stringcontainershape.size() == 1);
    assert(stringcontainershape[0] == stringcontainerdata.size());

    assert(read_stringcontainerdata == stringcontainerdata);

    // read everything into one string
    auto [onestringshape, read_onestringdata] = stringset->read<std::string>();

    assert(onestringshape.size() == 1);
    assert(onestringshape[0] == stringcontainershape[0]);
    assert(read_onestringdata == onestringdata);

    // read everything into a pointer -> this returns a smartpointer.
    auto [ptrshape, read_ptrdata] = ptrset->read<double*>({}, {}, {});
    assert(ptrshape.size() == 1);
    assert(ptrshape[0] = 15);
    for (std::size_t i = 0; i < ptrshape[0]; ++i)
    {
        assert(std::abs(ptrdata[i] - read_ptrdata.get()[i]) < 1e-16);
    }

    // read 2d dataset
    auto [twodshape, read_twoddata] = twoDdataset->read<std::vector<double>>();
    assert(twodshape.size() == 2);
    assert(twodshape[0] == 6);
    assert(twodshape[1] == 100);
    assert(read_twoddata.size() == 600);
    for (std::size_t i = 0; i < 6; ++i)
    {
        for (std::size_t j = 0; j < 100; ++j)
        {
            assert(std::abs(twoddata[i][j] - read_twoddata[i * 100 + j]) < 1e-16);
        }
    }

    // read adaptedset
    auto [adaptedshape, read_adaptedata] = adapteddataset->read<std::vector<double>>();
    assert(adaptedshape.size() == 2);
    assert(adaptedshape[0] == 3);
    assert(adaptedshape[1] == 100);
    for (std::size_t i = 0; i < 100; ++i)
    {
        assert(std::abs(adapteddata[i].x - read_adaptedata[i]) < 1e-16);
    }
    for (std::size_t i = 0; i < 100; ++i)
    {
        assert(std::abs(adapteddata[i].y - read_adaptedata[100 + i]) < 1e-16);
    }
    for (std::size_t i = 0; i < 100; ++i)
    {
        assert(std::abs(adapteddata[i].z - read_adaptedata[2 * 100 + i]) < 1e-16);
    }

    // read fireandforgetdataset
    auto [fireandforgetshape, read_fireandforgetdata] =
        fireandforgetdataset->read<std::vector<int>>();
    assert(fireandforgetshape == std::vector<hsize_t>{50});
    assert(read_fireandforgetdata == fireandforgetdata);

    // read fireandforgetdataset2d
    auto [fireandforget2dshape, read_fireandforgetdata2d] =
        fireandforgetdataset2d->read<std::vector<int>>();
    assert(fireandforget2dshape == (std::vector<hsize_t>{5, 100}));
    assert(fireandforgetdata2d == read_fireandforgetdata2d);

    ////////////////////////////////////////////////////////////////////////////
    /////////////////// PARTIAL READING TAKES PLACE NOW ////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // README: offset is used in partial reads, and hence its value has to
    // be tested again (should equal start always)

    // README: below numpy slice notation is used in comments, at least for 1d

    // read [5:25:2] from container dataset
    auto [partial_contshape, read_partial_contdata] =
        contset->read<std::vector<double>>({5}, {25}, {2});

    assert(contset->get_offset() == hsizevec{5});
    assert(partial_contshape.size() == 1);
    assert(partial_contshape[0] = 20);
    assert(read_partial_contdata == partial_contdata);

    // read subset from nested containerdata
    auto [partial_nestedcontshape, read_partial_nestedcontdata] =
        nestedcontset->read<std::vector<std::array<int, 4>>>({0}, {30}, {3});

    assert(nestedcontset->get_offset() == hsizevec{0});
    assert(partial_nestedcontshape.size() == 1);
    assert(partial_nestedcontshape[0] == 10);
    assert(partial_nestedcontshape[0] == read_partial_nestedcontdata.size());
    assert(partial_nestedcontdata == read_partial_nestedcontdata);

    // read subset from 2d array: [[2,0]:[4, 100]:[1,2]]
    auto [partial2dshape, read_partial2ddata] =
        twoDdataset->read<std::vector<double>>({2, 0}, {4, 100}, {1, 2});

    assert(twoDdataset->get_offset() == (hsizevec{2, 0}));
    assert(partial2dshape.size() == 2);
    assert(partial2dshape[0] == 2);
    assert(partial2dshape[1] == 50);
    assert(read_partial2ddata.size() == partial2dshape[0] * partial2dshape[1]);

    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 50; ++j)
        {
            assert(std::abs(partial_twoddata[i][j] - read_partial2ddata[i * 50 + j]) < 1e-16);
        }
    }

    // read [2:3:1] -> single value from scalardataset
    auto [partial_scalarshape, read_partialscalardata] =
        scalarset->read<int>({2}, {3}, {1});

    assert(scalarset->get_offset() == hsizevec{2});

    assert(partial_scalarshape.size() == 1);
    assert(partial_scalarshape[0] == 1);
    assert(read_partialscalardata == 2);

    // read [5:12:1] from pointerdataset
    auto [partial_ptrshape, read_partial_ptrdata] =
        ptrset->read<double*>({5}, {12}, {1});

    assert(ptrset->get_offset() == hsizevec{5});

    assert(partial_ptrshape.size() == 1);
    assert(partial_ptrshape[0] == 7);
    assert(partial_ptrdata.size() == partial_ptrshape[0]);

    for (std::size_t i = 0; i < partial_ptrshape[0]; ++i)
    {
        assert(std::abs(partial_ptrdata[i] - read_partial_ptrdata.get()[i]) < 1e-16);
    }

    // read a single string from stringdataset
    auto [singlestringshape, singlestring] =
        stringset->read<std::string>({3}, {4}, {1});

    assert(stringset->get_offset() == hsizevec{3});
    assert(singlestringshape.size() == 1);
    assert(singlestringshape[0] == 1);
    assert(singlestring == stringcontainerdata[3]); // reuse value from stringcontainerdata

    return 0;
}
