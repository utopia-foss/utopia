#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std::literals::chrono_literals;
using namespace Utopia::DataIO;
// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

void write()
{
    // FIXME: asserts missing here
    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    HDFFile file("testfile.h5", "w");
    auto contset = file.open_dataset("/containerdataset", {100}, {5});
    auto nestedcontset = file.open_dataset("/containercontainerdataset", {100}, {5});
    auto stringset = file.open_dataset("/stringdataset", {100}, {5});
    auto ptrset = file.open_dataset("/pointerdataset", {100}, {5});
    auto scalarset = file.open_dataset("/scalardataset", {100}, {5});
    auto twoDdataset = file.open_dataset("/2ddataset", {10, 100}, {1, 5});
    auto adapteddataset = file.open_dataset("/adapteddataset", {3, 100}, {1, 10});

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE DATA NEEDED LATER /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    std::array<int, 4> arr{{0, 1, 2, 3}};
    std::array<int, 4> arr2{{4, 5, 6, 7}};

    std::shared_ptr<double> ptr(new double[5]);
    for (std::size_t i = 0; i < 5; ++i)
    {
        ptr.get()[i] = 3.14;
    }

    std::vector<Point> points(100);
    for (int i = 0; i < 100; ++i)
    {
        points[i].x = 3.14;
        points[i].y = 3.14 + 1;
        points[i].z = 3.14 + 2;
    }

    ///////////////////////////////////////////////////////////////////////////
    /////////////////// ACTUAL WRITING TAKES PLACE NOW ////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // good'ol' simple vector of numbers
    /*
        3.14
        3.14
        3.14
        .
        .
        .
        3.14
        6.28
        6.28
        6.28
        .
        .
        .
        6.28
        9.42
        9.42
        9.42
        .
        .
        .
        9.42
    */
    contset->write(std::vector<double>(10, 3.14));
    contset->write(std::vector<double>(10, 6.28));
    contset->write(std::vector<double>(10, 9.42));

    // write array dataset, then append
    /*
        [0,1,2,3]
        [0,1,2,3]
        [0,1,2,3]
        .
        .
        .
        [0,1,2,3]
        [4,5,6,7]
        [4,5,6,7]
        [4,5,6,7]
        .
        .
        .
        [4,5,6,7]
    */
    nestedcontset->write(std::vector<std::array<int, 4>>(20, arr));
    nestedcontset->write(std::vector<std::array<int, 4>>(20, arr2));

    // write a bunch of strings one after another into the dataset.
    // note that missing parts are filled with '\0'(?), and the very first
    // string determines the length
    stringset->write(std::string("niggastring"));

    for (std::size_t i = 0; i < 25; ++i)
    {
        stringset->write(std::to_string(i));
    }

    // write pointers -> 3 times, each time an array of len 5 with different numbers:
    /*
        3.14,3.14,3.14,3.14,3.14,6.28,6.28,6.28,6.28,6.28,9.42,9.42,9.42,9.42,9.42
    */
    ptrset->write(ptr.get(), {5});

    for (std::size_t j = 2; j < 4; ++j)
    {
        for (std::size_t i = 0; i < 5; ++i)
        {
            ptr.get()[i] = j * 3.14;
        }
        ptrset->write(ptr.get(), {5});
    }

    // write 5 scalars (single numbers) one after another into the dataset
    /*
        0
        1
        2
        3
        4
    */
    for (int i = 0; i < 5; ++i)
    {
        scalarset->write(i);
    }

    // write 2d dataset
    // looks like this:
    /*
        0,0,0,0,0,  ...,0
        1,1,1,1,1,  ...,1
        2,2,2,2,2,  ...,2
        3,3,3,3,3,  ...,3
        4,4,4,4,4,  ...,4
        5,5,5,5,5,  ...,5
    */
    for (std::size_t i = 0; i < 6; ++i)
    {
        twoDdataset->write(std::vector<double>(100, i));
    }

    // write each coordinate into one line in adapteddataset:
    // looks like this:
    /*
        x1, x2, x3, ..., x100
        y1, y2, y3, ..., y100
        z1, z2, z3, ..., z100
    */
    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.x; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.y; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.z; });
}

void read()
{
    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    HDFFile file("testfile.h5", "r");
    auto contset = file.open_dataset("/containerdataset");
    auto nestedcontset = file.open_dataset("/containercontainerdataset");
    auto stringset = file.open_dataset("/stringdataset");
    auto ptrset = file.open_dataset("/pointerdataset");
    auto scalarset = file.open_dataset("/scalardataset");
    auto twoDdataset = file.open_dataset("/2ddataset");
    auto adapteddataset = file.open_dataset("/adapteddataset");
    auto adapteddataset2d = file.open_dataset("/adapteddataset2d");

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
        "niggastring", "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
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

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////// FULL READING TAKES PLACE NOW ////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

    ////////////////////////////////////////////////////////////////////////////
    /////////////////// PARTIAL READING TAKES PLACE NOW ////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // below: use numpy slice notation in comments, at least for 1d

    // read [5:25:2] from container dataset
    auto [partial_contshape, read_partial_contdata] =
        contset->read<std::vector<double>>({5}, {25}, {2});
    assert(partial_contshape.size() == 1);
    assert(partial_contshape[0] = 20);
    assert(read_partial_contdata == partial_contdata);

    // read subset from nested containerdata
    auto [partial_nestedcontshape, read_partial_nestedcontdata] =
        nestedcontset->read<std::vector<std::array<int, 4>>>({0}, {30}, {3});

    assert(partial_nestedcontshape.size() == 1);
    assert(partial_nestedcontshape[0] == 10);
    assert(partial_nestedcontshape[0] == read_partial_nestedcontdata.size());

    assert(partial_nestedcontdata == read_partial_nestedcontdata);

    // read subset from 2d array: [[2,0]:[4, 100]:[1,2]]

    auto [partial2dshape, read_partial2ddata] =
        twoDdataset->read<std::vector<double>>({2, 0}, {4, 100}, {1, 2});

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

    assert(partial_scalarshape.size() == 1);
    assert(partial_scalarshape[0] == 1);
    assert(read_partialscalardata == 2);

    // read [5:12:1] from pointerdataset
    auto [partial_ptrshape, read_partial_ptrdata] =
        ptrset->read<double*>({5}, {12}, {1});
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
    assert(singlestringshape.size() == 1);
    assert(singlestringshape[0] == 1);
    assert(singlestring == stringcontainerdata[3]); // reuse value from stringcontainerdata
}

int main()
{
    write();
    read();
    return 0;
}