
/**
 * @brief In this file, the write functionality of hdfdataset is tested.
 *        Parameter setting from constructors are not tested,
 *        hdfdataset_test_lifecycle is for that. Hence, this file only tests
 *        updates of parameters like current_extent and offset.
 *
 * @date 2018-07-18
 */
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>

// shorthands
using namespace Utopia::DataIO;
using hsizevec = std::vector<hsize_t>;

// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

int main()
{
    Utopia::setup_loggers();
    spdlog::get("data_io")->set_level(spdlog::level::debug);

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    HDFFile file("datatset_testfile.h5", "w");

    auto contset = file.open_dataset("/containerdataset", {100}, {5});
    auto nestedcontset = file.open_dataset("/containercontainerdataset", {100}, {5});
    auto stringset = file.open_dataset("/stringdataset", {100}, {5});
    auto ptrset = file.open_dataset("/pointerdataset", {100}, {5});
    auto scalarset = file.open_dataset("/scalardataset", {100}, {5});
    auto twoDdataset = file.open_dataset("/2ddataset", {10, 100}, {1, 5});

    // README: capacity of {fixed, H5S_UNLIMITED} is not possible, because hdf5
    // does not know when to start the next line. Use variable length vectors for this
    auto twoDdataset_unlimited =
        file.open_dataset("/2ddataset_unlimited", {H5S_UNLIMITED, 100});
    auto adapteddataset = file.open_dataset("/adapteddataset", {3, 100}, {1, 10});
    auto fireandforgetdataset = file.open_dataset("/fireandforget");
    auto fireandforgetdataset2d = file.open_dataset("/fireandforget2d", {5, 100});
    auto latestarterdataset = file.open_dataset("/latestarter");
    auto latestarterdataset2 = file.open_dataset("/latestarter2");

    ////////////////////////////////////////////////////////////////////////////
    ////////////////// Test attribute writing prior to dataset write ///////////
    ////////////////////////////////////////////////////////////////////////////

    // reassure us that the dataset is in an invalid state from
    // HDF5's point of view
    assert(!check_validity(H5Iis_valid(contset->get_id()), contset->get_path()));

    // use the 'contset' to test the attribute 'writing' capability.
    contset->add_attribute("first attribute", std::vector<int>{1, 2, 3, 4, 5});
    contset->add_attribute("second attribute",
                           std::string(" 'tiz no attrrriboate"));
    contset->add_attribute("third attribute", 3.14);

    // now assert that the attribute buffer of 'contset' has respective entries
    auto attrbuff = contset->get_attribute_buffer();
    assert(attrbuff.size() == 3);

    assert(attrbuff[0].first == "first attribute");
    assert(attrbuff[1].first == "second attribute");
    assert(attrbuff[2].first == "third attribute");

    assert(std::get<std::vector<int>>(attrbuff[0].second).size() == 5);
    for (int i = 0; i < 5; ++i)
    {
        assert(i + 1 == std::get<std::vector<int>>(attrbuff[0].second)[i]);
    }
    assert(std::get<std::string>(attrbuff[1].second) ==
           " 'tiz no attrrriboate");
    assert((std::get<double>(attrbuff[2].second) - 3.14) < 1e-16);

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

    // good ol' simple vector of numbers, should look like this in file
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

    // mind that this actually creates the dataset in the file

    contset->write(std::vector<double>(10, 3.14));
    assert(contset->get_current_extent() == hsizevec{10});

    contset->write(std::vector<double>(10, 6.28));
    assert(contset->get_current_extent() == hsizevec{20});

    contset->write(std::vector<double>(10, 9.42));
    assert(contset->get_current_extent() == hsizevec{30});
    assert(check_validity(H5Iis_valid(contset->get_id()), contset->get_path()));

    // write array dataset, then append, should look like this in file
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
    assert(nestedcontset->get_current_extent() == hsizevec{20});
    assert(nestedcontset->get_offset() == hsizevec{0});

    nestedcontset->write(std::vector<std::array<int, 4>>(20, arr2));
    assert(nestedcontset->get_current_extent() == hsizevec{40});
    assert(nestedcontset->get_offset() == hsizevec{20});

    // write a bunch of strings one after another into the dataset.
    // note that missing parts are filled with '\0'(?), and the very first
    // string determines the length. Can't do shit about the \0 unfortunatly
    // should look like this in file
    /*
        testsstring
        0\0\0\0\0\0\0\0\0\0\0
        1\0\0\0\0\0\0\0\0\0\0
        2\0\0\0\0\0\0\0\0\0\0
        3\0\0\0\0\0\0\0\0\0\0
        .
        .
        .
        25\0\0\0\0\0\0\0\0\0\0
    */
    stringset->write(std::string("testsstring"));
    assert(stringset->get_current_extent() == hsizevec{1});
    assert(stringset->get_offset() == hsizevec{0});
    for (std::size_t i = 0; i < 25; ++i)
    {
        stringset->write(std::to_string(i));
        assert(stringset->get_current_extent() == hsizevec{i + 2});
        assert(stringset->get_offset() == hsizevec{i + 1});
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
        assert(twoDdataset->get_current_extent() == (hsizevec{i + 1, 100}));
        assert(twoDdataset->get_offset() == (hsizevec{i, 0}));
    }

    // write 2d dataset with unlimited first dimension
    for (std::size_t i = 0; i < 55; ++i)
    {
        twoDdataset_unlimited->write(std::vector<int>(100, i));
        assert(twoDdataset_unlimited->get_current_extent() == (hsizevec{i + 1, 100}));
        assert(twoDdataset_unlimited->get_offset() == (hsizevec{i, 0}));
    }

    // README: we now tested  the current_extent/offset update in all occuring
    // cases, hence it is not repeted blow (ptr/adapted and scalar just repeats container and string logic)

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

    // write good ol' vector into dataset where everything is automatically
    // determined. Includes ability to extend it, like for contset
    fireandforgetdataset->write(std::vector<int>(10, 1));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{10});
    fireandforgetdataset->write(std::vector<int>(10, 2));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{20});
    fireandforgetdataset->write(std::vector<int>(10, 3));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{30});
    fireandforgetdataset->write(std::vector<int>(10, 4));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{40});
    fireandforgetdataset->write(std::vector<int>(10, 5));
    assert(fireandforgetdataset->get_current_extent() == hsizevec{50});

    // write good ol' vector into 2d fireandforget dataset which determines
    // chunksize automatically. Works like twoddataset
    for (std::size_t i = 0; i < 5; ++i)
    {
        fireandforgetdataset2d->write(std::vector<int>(100, i + 1));
        assert(fireandforgetdataset2d->get_current_extent() == (hsizevec{i + 1, 100}));
        assert(fireandforgetdataset2d->get_offset() == (hsizevec{i, 0}));
    }

    latestarterdataset->set_capacity({500}); // as it is now, chunksize will be automatically determined
    latestarterdataset->write(std::vector<int>{1, 2, 3, 4, 5});
    latestarterdataset->write(std::vector<int>{-1, -2, -3, -4, -5});

    // test that the exception is correctly thrown
    try
    {
        latestarterdataset->set_capacity({700});
    }
    catch (std::exception& e)
    {
        std::string message =
            "Dataset latestarter: Cannot set capacity after dataset has been "
            "created";
        assert(e.what() == message);
    }

    latestarterdataset2->set_capacity({500}); // as it is now, chunksize will be automatically determined

    // try to set it wrongly
    try
    {
        latestarterdataset2->set_chunksize({5, 30, 7});
    }
    catch (std::exception& e)
    {
        std::string message =
            "Dataset latestarter2: Chunksizes size has to be equal to dataset "
            "rank";
        assert(e.what() == message);
    }

    latestarterdataset2->set_chunksize({}); // automatically determined, should work out

    latestarterdataset2->set_chunksize({10}); // as it is now, chunksize will be automatically determined
    latestarterdataset2->write(std::vector<int>(25, 12));

    // test that the exception is correctly thrown

    try
    {
        latestarterdataset2->set_chunksize({30});
    }
    catch (std::exception& e)
    {
        std::string message =
            "Dataset latestarter2: Cannot set chunksize after dataset has been "
            "created";
        assert(e.what() == message);
    }

    // now close everything. Then write attributes
    // to 'contset' while it is closed => reopen file & contset -> check if
    // writing works
    contset->close();
    nestedcontset->close();
    stringset->close();
    ptrset->close();
    scalarset->close();
    twoDdataset->close();
    twoDdataset_unlimited->close();
    adapteddataset->close();
    fireandforgetdataset->close();
    fireandforgetdataset2d->close();
    latestarterdataset->close();
    latestarterdataset2->close();

    assert(!check_validity(H5Iis_valid(contset->get_id()), contset->get_path()));
    attrbuff = contset->get_attribute_buffer();
    assert(attrbuff.size() == 0);

    file.close(); // force-write data to hard disk

    file = HDFFile("datatset_testfile.h5", "r+");

    // now write attributes while it is closed
    contset->add_attribute("forth attribute", 478953ul);
    contset->add_attribute("fifth attribute", std::vector<double>(10, 3.14));

    attrbuff = contset->get_attribute_buffer();

    assert(attrbuff.size() == 2);
    assert(attrbuff[0].first == "forth attribute");
    assert(attrbuff[1].first == "fifth attribute");
    assert(std::get<unsigned long>(attrbuff[0].second) == 478953ul);
    assert(std::get<std::vector<double>>(attrbuff[1].second).size() == 10);

    contset->open(*file.get_basegroup(), "containerdataset");
    assert(check_validity(H5Iis_valid(contset->get_id()), contset->get_path()));

    contset->close();
    assert(!check_validity(H5Iis_valid(contset->get_id()), contset->get_path()));

    attrbuff = contset->get_attribute_buffer();
    assert(attrbuff.size() == 0);

    return 0;
}
