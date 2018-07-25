
/**
 * @brief In this file, the write functionality of hdfdataset is tested.
 *        Parameter setting from constructors are not tested,
 *        hdfdataset_test_lifecycle is for that. Hence, this file only tests
 *        updates of parameters like current_extent and offset.
 *
 * @date 2018-07-18
 */
#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

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
    auto adapteddataset = file.open_dataset("/adapteddataset", {3, 100}, {1, 10});
    auto fireandforgetdataset = file.open_dataset("/fireandforget");
    auto fireandforgetdataset2d = file.open_dataset("/fireandforget2d", {5, 100});

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

    contset->write(std::vector<double>(10, 3.14));
    assert(contset->get_current_extent() == hsizevec{10});

    contset->write(std::vector<double>(10, 6.28));
    assert(contset->get_current_extent() == hsizevec{20});

    contset->write(std::vector<double>(10, 9.42));
    assert(contset->get_current_extent() == hsizevec{30});

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

    return 0;
}
