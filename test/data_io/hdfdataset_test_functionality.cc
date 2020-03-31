#define BOOST_TEST_MODULE dataset_functionality_test

#include <iostream>

#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

// shorthands
using namespace Utopia::DataIO;
using hsizevec = std::vector< hsize_t >;

// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

BOOST_AUTO_TEST_CASE(dataset_write_test)
{
    Utopia::setup_loggers();
    spdlog::get("data_io")->set_level(spdlog::level::debug);

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    HDFFile file("dataset_testfile.h5", "w");

    auto contset = file.open_dataset("/containerdataset", { 100 }, { 5 });
    auto nestedcontset =
        file.open_dataset("/containercontainerdataset", { 100 }, { 5 });
    auto stringset   = file.open_dataset("/stringdataset", { 100 }, { 5 });
    auto ptrset      = file.open_dataset("/pointerdataset", { 100 }, { 5 });
    auto scalarset   = file.open_dataset("/scalardataset", { 100 }, { 5 });
    auto twoDdataset = file.open_dataset("/2ddataset", { 10, 100 }, { 1, 5 });

    // README: capacity of {fixed, H5S_UNLIMITED} is not possible, because hdf5
    // does not know when to start the next line. Use variable length vectors
    // for this
    auto twoDdataset_unlimited =
        file.open_dataset("/2ddataset_unlimited", { H5S_UNLIMITED, 100 });

    auto adapteddataset =
        file.open_dataset("/adapteddataset", { 3, 100 }, { 1, 10 });
    auto fireandforgetdataset = file.open_dataset("/fireandforget");
    auto fireandforgetdataset2d =
        file.open_dataset("/fireandforget2d", { 5, 100 });
    auto latestarterdataset  = file.open_dataset("/latestarter");
    auto latestarterdataset2 = file.open_dataset("/latestarter2");

    ////////////////////////////////////////////////////////////////////////////
    ////////////////// Test attribute writing prior to dataset write ///////////
    ////////////////////////////////////////////////////////////////////////////

    // reassure us that the dataset is in an invalid state from
    // HDF5's point of view
    BOOST_TEST(not contset->is_valid());

    // use the 'contset' to test the attribute 'writing' capability.
    contset->add_attribute("first attribute",
                           std::vector< int >{ 1, 2, 3, 4, 5 });
    contset->add_attribute("second attribute",
                           std::string(" 'tiz no attrrriboate"));
    contset->add_attribute("third attribute", 3.14);

    // now BOOST_TEST that the attribute buffer of 'contset' has respective
    // entries
    auto attrbuff = contset->get_attribute_buffer();
    BOOST_TEST(attrbuff.size() == 3);

    BOOST_TEST(attrbuff[0].first == "first attribute");
    BOOST_TEST(attrbuff[1].first == "second attribute");
    BOOST_TEST(attrbuff[2].first == "third attribute");

    BOOST_TEST(std::get< std::vector< int > >(attrbuff[0].second).size() == 5);
    for (int i = 0; i < 5; ++i)
    {
        BOOST_TEST(i + 1 ==
                   std::get< std::vector< int > >(attrbuff[0].second)[i]);
    }
    BOOST_TEST(std::get< std::string >(attrbuff[1].second) ==
               " 'tiz no attrrriboate");
    BOOST_TEST((std::get< double >(attrbuff[2].second) - 3.14) < 1e-16);

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE DATA NEEDED LATER /////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    std::array< int, 4 > arr{ { 0, 1, 2, 3 } };
    std::array< int, 4 > arr2{ { 4, 5, 6, 7 } };

    std::shared_ptr< double > ptr(new double[5]);
    for (std::size_t i = 0; i < 5; ++i)
    {
        ptr.get()[i] = 3.14;
    }

    std::vector< Point > points(100);
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

    contset->write(std::vector< double >(10, 3.14));
    BOOST_TEST(contset->get_current_extent() == hsizevec{ 10 });

    contset->write(std::vector< double >(10, 6.28));
    BOOST_TEST(contset->get_current_extent() == hsizevec{ 20 });

    contset->write(std::vector< double >(10, 9.42));
    BOOST_TEST(contset->get_current_extent() == hsizevec{ 30 });
    BOOST_TEST(contset->is_valid());

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
    nestedcontset->write(std::vector< std::array< int, 4 > >(20, arr));
    BOOST_TEST(nestedcontset->get_current_extent() == hsizevec{ 20 });
    BOOST_TEST(nestedcontset->get_offset() == hsizevec{ 0 });

    nestedcontset->write(std::vector< std::array< int, 4 > >(20, arr2));
    BOOST_TEST(nestedcontset->get_current_extent() == hsizevec{ 40 });
    BOOST_TEST(nestedcontset->get_offset() == hsizevec{ 20 });

    // write a bunch of strings one after another into the dataset.
    // note that missing parts are filled with '\0'(?), and the very first
    // string determines the length. Can't do anything about the \0 unfortunatly
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
    stringset->write(std::string("test_strings"));
    BOOST_TEST(stringset->get_current_extent() == hsizevec{ 1 });
    BOOST_TEST(stringset->get_offset() == hsizevec{ 0 });

    // use stringstream to convert to a string of correct size
    std::stringstream s;
    for (std::size_t i = 0; i < 25; ++i)
    {
        s << std::setw(12) << i;
        stringset->write(s.str());
        s.str(std::string());
        BOOST_TEST(stringset->get_current_extent() == hsizevec{ i + 2 });
        BOOST_TEST(stringset->get_offset() == hsizevec{ i + 1 });
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
        twoDdataset->write(std::vector< double >(100, i));
        BOOST_TEST(twoDdataset->get_current_extent() ==
                   (hsizevec{ i + 1, 100 }));
        BOOST_TEST(twoDdataset->get_offset() == (hsizevec{ i, 0 }));
    }

    // write 2d dataset with unlimited first dimension
    for (std::size_t i = 0; i < 55; ++i)
    {
        twoDdataset_unlimited->write(std::vector< int >(100, i));
        BOOST_TEST(twoDdataset_unlimited->get_current_extent() ==
                   (hsizevec{ i + 1, 100 }));
        BOOST_TEST(twoDdataset_unlimited->get_offset() == (hsizevec{ i, 0 }));
    }

    // README: we now tested  the current_extent/offset update in all occuring
    // cases, hence it is not repeted blow (ptr/adapted and scalar just repeats
    // container and string logic)

    // write pointers -> 3 times, each time an array of len 5 with different
    // numbers:
    /*
        3.14,3.14,3.14,3.14,3.14,6.28,6.28,6.28,6.28,6.28,9.42,9.42,9.42,9.42,9.42
    */
    ptrset->write(ptr.get(), { 5 });
    for (std::size_t j = 2; j < 4; ++j)
    {
        for (std::size_t i = 0; i < 5; ++i)
        {
            ptr.get()[i] = j * 3.14;
        }
        ptrset->write(ptr.get(), { 5 });
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
    adapteddataset->write(
        points.begin(), points.end(), [](auto& pt) { return pt.x; });

    adapteddataset->write(
        points.begin(), points.end(), [](auto& pt) { return pt.y; });

    adapteddataset->write(
        points.begin(), points.end(), [](auto& pt) { return pt.z; });

    // write good ol' vector into dataset where everything is automatically
    // determined. Includes ability to extend it, like for contset
    fireandforgetdataset->write(std::vector< int >(10, 1));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 10 });
    fireandforgetdataset->write(std::vector< int >(10, 2));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 20 });
    fireandforgetdataset->write(std::vector< int >(10, 3));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 30 });
    fireandforgetdataset->write(std::vector< int >(10, 4));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 40 });
    fireandforgetdataset->write(std::vector< int >(10, 5));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 50 });

    // write good ol' vector into 2d fireandforget dataset which
    // determines
    // chunksize automatically. Works like twoddataset
    for (std::size_t i = 0; i < 5; ++i)
    {
        fireandforgetdataset2d->write(std::vector< int >(100, i + 1));
        BOOST_TEST(fireandforgetdataset2d->get_current_extent() ==
                   (hsizevec{ i + 1, 100 }));
        BOOST_TEST(fireandforgetdataset2d->get_offset() == (hsizevec{ i, 0 }));
    }

    latestarterdataset->set_capacity(
        { 500 }); // as it is now, chunksize will be automatically determined
    latestarterdataset->write(std::vector< int >{ 1, 2, 3, 4, 5 });
    latestarterdataset->write(std::vector< int >{ -1, -2, -3, -4, -5 });

    // test that the exception is correctly thrown
    BOOST_CHECK_EXCEPTION(latestarterdataset->set_capacity({ 700 }),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              // this approach makes sure that formatting does
                              // not influence equality check, which is  the
                              // case for string literals apparently..
                              std::string message(
                                  "Dataset /latestarter: Cannot set "
                                  "capacity after dataset has been "
                                  "created");
                              return e.what() == message;
                          });

    // as it is now, chunksize will be automatically determined
    latestarterdataset2->set_capacity({ 500 });

    BOOST_CHECK_EXCEPTION(latestarterdataset2->set_chunksize({ 5, 30, 7 }),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              std::string message(
                                  "Dataset latestarter2: Chunksizes size has "
                                  "to be equal to dataset rank");
                              return e.what() == message;
                          });

    latestarterdataset2->set_chunksize(
        {}); // automatically determined, should work out

    // as it is now, chunksize will be automatically determined
    latestarterdataset2->set_chunksize({ 10 });
    latestarterdataset2->write(std::vector< int >(25, 12));


    ///////////////////////////////////////////////////////////////////////////
    // check exceptions
    ///////////////////////////////////////////////////////////////////////////

    BOOST_CHECK_EXCEPTION(
        latestarterdataset2->set_chunksize({ 30 }),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string message(
                "Dataset /latestarter2: Cannot set chunksize after "
                "dataset has been created");
            return e.what() == message;
        });

    double ptr2[5] = { 3., 2., 1., -1., -2. };

    BOOST_CHECK_EXCEPTION(
        ptrset->write(&ptr2[0], {}),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string message("Dataset /pointerdataset: shape has to be "
                                "given explicitly when writing pointer types");
            return e.what() == message;
        });

    std::shared_ptr< double > ptrdata(new double[200]);
    BOOST_CHECK_EXCEPTION(
        ptrset->write(ptrdata.get(), { 200 }),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string message(
                "Dataset /pointerdataset: Cannot append data, _new_extent "
                "larger than capacity in dimension 0");
            return e.what() == message;
        });

    // create dataset for additional pointer exception checks
    auto ptrset2 = file.open_dataset("/ptrset2", { 100, 100, 100 });
    BOOST_CHECK_EXCEPTION(ptrset2->write(ptr2, { 5 }),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              std::string message("Rank > 2 not supported");
                              return e.what() == message;
                          });

    auto otherdataset = file.open_dataset("otherdataset", { 10 });
    otherdataset->write(std::vector< int >{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    BOOST_CHECK_EXCEPTION(
        otherdataset->write(
            std::vector< int >{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string message("Dataset /otherdataset: Error, dataset cannot "
                                "be extended because it reached its capacity");
            return e.what() == message;
        });

    ///////////////////////////////////////////////////////////////////////////
    // now close everything. Then write attributes
    ///////////////////////////////////////////////////////////////////////////

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

    BOOST_TEST(not contset->is_valid());
    attrbuff = contset->get_attribute_buffer();
    BOOST_TEST(attrbuff.size() == 0);

    file.close(); // force-write data to hard disk

    file = HDFFile("dataset_testfile.h5", "r+");

    // now write attributes while it is closed
    contset->add_attribute("forth attribute", 478953ul);
    contset->add_attribute("fifth attribute", std::vector< double >(10, 3.14));

    attrbuff = contset->get_attribute_buffer();

    BOOST_TEST(attrbuff.size() == 2);
    BOOST_TEST(attrbuff[0].first == "forth attribute");
    BOOST_TEST(attrbuff[1].first == "fifth attribute");
    BOOST_TEST(std::get< unsigned long >(attrbuff[0].second) == 478953ul);
    BOOST_TEST(std::get< std::vector< double > >(attrbuff[1].second).size() ==
               10);

    contset->open(*file.get_basegroup(), "containerdataset");
    BOOST_TEST(contset->is_valid());

    contset->close();
    BOOST_TEST(not contset->is_valid());

    attrbuff = contset->get_attribute_buffer();
    BOOST_TEST(attrbuff.size() == 0);

    auto except_dataset = file.open_dataset("exceptiondataset", { 100 }, { 5 });

    except_dataset->write(100);

    BOOST_CHECK_EXCEPTION(
        except_dataset->write("hello"),
        std::runtime_error,
        [](const std::runtime_error& err) {
            std::string m("Error, cannot write string data of a different type "
                          "into dataset /exceptiondataset");
            return err.what() == m;
        });
}

BOOST_AUTO_TEST_CASE(dataset_read_test)
{
    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE FILE, OPEN DATASETS  //////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    HDFFile file("dataset_testfile.h5", "r");

    auto contset       = file.open_dataset("/containerdataset");
    auto nestedcontset = file.open_dataset("/containercontainerdataset");
    auto stringset     = file.open_dataset("/stringdataset");
    auto ptrset        = file.open_dataset("/pointerdataset");
    auto scalarset     = file.open_dataset("/scalardataset");
    auto twoDdataset   = file.open_dataset("/2ddataset");
    auto twoDdataset_unlimited = file.open_dataset("/2ddataset_unlimited");

    auto adapteddataset         = file.open_dataset("/adapteddataset");
    auto fireandforgetdataset   = file.open_dataset("/fireandforget");
    auto fireandforgetdataset2d = file.open_dataset("/fireandforget2d");

    // this is only used to check that capacity and chunksize setter work
    // it is not read, as the read functionality for it is tested at contset
    auto latestarterdataset2 = file.open_dataset("/latestarter2");

    // check that parameters are read out correctly
    BOOST_TEST(contset->get_capacity() == hsizevec{ 100 });
    BOOST_TEST(nestedcontset->get_capacity() == hsizevec{ 100 });
    BOOST_TEST(stringset->get_capacity() == hsizevec{ 100 });
    BOOST_TEST(ptrset->get_capacity() == hsizevec{ 100 });
    BOOST_TEST(scalarset->get_capacity() == hsizevec{ 100 });
    BOOST_TEST(twoDdataset->get_capacity() == (hsizevec{ 10, 100 }));
    BOOST_TEST(adapteddataset->get_capacity() == (hsizevec{ 3, 100 }));
    BOOST_TEST(fireandforgetdataset->get_capacity() ==
               hsizevec{ H5S_UNLIMITED });
    BOOST_TEST(fireandforgetdataset2d->get_capacity() == (hsizevec{ 5, 100 }));
    BOOST_TEST(latestarterdataset2->get_capacity() == (hsizevec{ 500 }));

    BOOST_TEST(contset->get_current_extent() == hsizevec{ 30 });
    BOOST_TEST(nestedcontset->get_current_extent() == hsizevec{ 40 });
    BOOST_TEST(stringset->get_current_extent() == hsizevec{ 26 });
    BOOST_TEST(ptrset->get_current_extent() == hsizevec{ 15 });
    BOOST_TEST(scalarset->get_current_extent() == hsizevec{ 5 });
    BOOST_TEST(twoDdataset->get_current_extent() == (hsizevec{ 6, 100 }));
    BOOST_TEST(adapteddataset->get_current_extent() == (hsizevec{ 3, 100 }));
    BOOST_TEST(fireandforgetdataset->get_current_extent() == hsizevec{ 50 });
    BOOST_TEST(fireandforgetdataset2d->get_current_extent() ==
               (hsizevec{ 5, 100 }));

    BOOST_TEST(contset->get_chunksizes() == hsizevec{ 5 });
    BOOST_TEST(nestedcontset->get_chunksizes() == hsizevec{ 5 });
    BOOST_TEST(stringset->get_chunksizes() == hsizevec{ 5 });
    BOOST_TEST(ptrset->get_chunksizes() == hsizevec{ 5 });
    BOOST_TEST(scalarset->get_chunksizes() == hsizevec{ 5 });
    BOOST_TEST(twoDdataset->get_chunksizes() == (hsizevec{ 1, 5 }));
    BOOST_TEST(adapteddataset->get_chunksizes() == (hsizevec{ 1, 10 }));
    BOOST_TEST(latestarterdataset2->get_chunksizes() == (hsizevec{ 10 }));

    // offset should be at end of data currently contained
    BOOST_TEST(contset->get_offset() == hsizevec{ 30 });
    BOOST_TEST(nestedcontset->get_offset() == hsizevec{ 40 });
    BOOST_TEST(stringset->get_offset() == hsizevec{ 26 });
    BOOST_TEST(ptrset->get_offset() == hsizevec{ 15 });
    BOOST_TEST(scalarset->get_offset() == hsizevec{ 5 });
    BOOST_TEST(twoDdataset->get_offset() == (hsizevec{ 6, 100 }));
    BOOST_TEST(adapteddataset->get_offset() == (hsizevec{ 3, 100 }));
    BOOST_TEST(fireandforgetdataset->get_offset() == hsizevec{ 50 });
    BOOST_TEST(fireandforgetdataset2d->get_offset() == (hsizevec{ 5, 100 }));

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////// MAKE EXPECTED DATA TO TEST AGAINST ////////////////
    ///////////////////////////////////////////////////////////////////////////
    // ... for simple container
    std::vector< double > contdata;
    contdata.insert(contdata.begin(), 10, 3.14);
    contdata.insert(contdata.begin() + 10, 10, 6.28);
    contdata.insert(contdata.begin() + 20, 10, 9.42);

    std::vector< double > partial_contdata(10);
    std::size_t           j = 0;
    for (std::size_t i = 5; i < 25; i += 2, ++j)
    {
        partial_contdata[j] = contdata[i];
    }

    // ... for nested container
    std::array< int, 4 > arr{ { 0, 1, 2, 3 } };
    std::array< int, 4 > arr2{ { 4, 5, 6, 7 } };

    std::vector< std::array< int, 4 > > nestedcontdata(20, arr);
    nestedcontdata.insert(nestedcontdata.begin() + 20, 20, arr2);

    std::vector< std::array< int, 4 > > partial_nestedcontdata(10);
    j = 0;
    for (std::size_t i = 0; i < 30; i += 3, ++j)
    {
        partial_nestedcontdata[j] = nestedcontdata[i];
    }

    // ... for 2d dataset
    std::vector< std::vector< double > > twoddata(6, std::vector< double >());
    for (std::size_t i = 0; i < 6; ++i)
    {
        twoddata[i].insert(twoddata[i].begin(), 100, i);
    }

    // ... for 2d dataset unlimited
    std::vector< std::vector< int > > twoddata_unlimited(55,
                                                         std::vector< int >());
    for (std::size_t i = 0; i < 55; ++i)
    {
        twoddata_unlimited[i] = std::vector< int >(100, i);
    }
    std::vector< std::vector< double > > partial_twoddata(
        2, std::vector< double >());
    for (std::size_t i = 0; i < 2; ++i)
    {
        partial_twoddata[i].insert(partial_twoddata[i].begin(), 50, i + 2);
    }
    // ... for stringdata
    std::vector< std::string > stringcontainerdata;

    stringcontainerdata.push_back(std::string("test_strings"));
    std::stringstream s;

    for (std::size_t i = 0; i < 25; ++i)
    {
        s << std::setw(12) << i;
        stringcontainerdata.push_back(s.str());
        s.str(std::string());
    }

    // ... for stringdata read into one single string
    std::string onestringdata;
    for (auto& str : stringcontainerdata)
    {
        onestringdata += str;
    }

    // ... for pointer dataset
    std::vector< double > ptrdata{ 3.14, 3.14, 3.14, 3.14, 3.14,
                                   6.28, 6.28, 6.28, 6.28, 6.28,
                                   9.42, 9.42, 9.42, 9.42, 9.42 };

    std::vector< double > partial_ptrdata(7);
    for (std::size_t i = 0; i < 7; ++i)
    {
        partial_ptrdata[i] = ptrdata[i + 5];
    }

    // ... for adaptedset
    std::vector< Point > adapteddata(100);
    for (int i = 0; i < 100; ++i)
    {
        adapteddata[i].x = 3.14;
        adapteddata[i].y = 3.14 + 1;
        adapteddata[i].z = 3.14 + 2;
    }

    // ... for fireandforgetdataset
    std::vector< int > fireandforgetdata(50);
    for (std::size_t i = 0; i < 5; ++i)
    {
        std::generate(fireandforgetdata.begin() + i * 10,
                      fireandforgetdata.begin() + (i + 1) * 10,
                      [&]() -> int { return i + 1; });
    }

    // make expected data for fireandforget2d -> 1d vector of size 500, which
    // represents 2d data of dimensions [5, 100]
    std::vector< int > fireandforgetdata2d(500);
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
    auto [contshape, read_contdata] = contset->read< std::vector< double > >();
    BOOST_TEST(contshape.size() == 1);
    BOOST_TEST(contshape[0] = 30);
    BOOST_TEST(contdata == read_contdata);

    // read nested container data
    auto [nestedcontshape, read_nestedcontdata] =
        nestedcontset->read< std::vector< std::array< int, 4 > > >();
    BOOST_TEST(nestedcontshape.size() == 1);
    BOOST_TEST(nestedcontshape[0] = 40);
    BOOST_TEST(read_nestedcontdata.size() == 40);
    BOOST_TEST(nestedcontdata == read_nestedcontdata);

    // read stringdataset
    auto [stringcontainershape, read_stringcontainerdata] =
        stringset->read< std::vector< std::string > >();
    BOOST_TEST(stringcontainershape.size() == 1);
    BOOST_TEST(stringcontainershape[0] == stringcontainerdata.size());

    BOOST_TEST(read_stringcontainerdata == stringcontainerdata);

    // read everything into one string
    auto [onestringshape, read_onestringdata] =
        stringset->read< std::string >();

    BOOST_TEST(onestringshape.size() == 1);
    BOOST_TEST(onestringshape[0] == stringcontainershape[0]);
    BOOST_TEST(read_onestringdata == onestringdata);

    // read everything into a pointer -> this returns a smartpointer.
    auto [ptrshape, read_ptrdata] = ptrset->read< double* >({}, {}, {});
    BOOST_TEST(ptrshape.size() == 1);
    BOOST_TEST(ptrshape[0] = 15);
    for (std::size_t i = 0; i < ptrshape[0]; ++i)
    {
        BOOST_TEST(std::abs(ptrdata[i] - read_ptrdata.get()[i]) < 1e-16);
    }

    // read 2d dataset
    auto [twodshape, read_twoddata] =
        twoDdataset->read< std::vector< double > >();
    BOOST_TEST(twodshape.size() == 2);
    BOOST_TEST(twodshape[0] == 6);
    BOOST_TEST(twodshape[1] == 100);
    BOOST_TEST(read_twoddata.size() == 600);
    for (std::size_t i = 0; i < 6; ++i)
    {
        for (std::size_t j = 0; j < 100; ++j)
        {
            BOOST_TEST(std::abs(twoddata[i][j] - read_twoddata[i * 100 + j]) <
                       1e-16);
        }
    }

    // read 2d unlimited dataset
    auto [twodshape_unlimited, read_twoddata_unlimited] =
        twoDdataset_unlimited->read< std::vector< int > >();
    BOOST_TEST(twodshape_unlimited.size() == 2);
    BOOST_TEST(twodshape_unlimited[0] == 55);
    BOOST_TEST(twodshape_unlimited[1] == 100);
    BOOST_TEST(read_twoddata_unlimited.size() == 5500);
    for (std::size_t i = 0; i < 55; ++i)
    {
        for (std::size_t j = 0; j < 100; ++j)
        {
            BOOST_TEST(twoddata_unlimited[i][j] ==
                       read_twoddata_unlimited[i * 100 + j]);
        }
    }

    // read adaptedset
    auto [adaptedshape, read_adaptedata] =
        adapteddataset->read< std::vector< double > >();
    BOOST_TEST(adaptedshape.size() == 2);
    BOOST_TEST(adaptedshape[0] == 3);
    BOOST_TEST(adaptedshape[1] == 100);
    for (std::size_t i = 0; i < 100; ++i)
    {
        BOOST_TEST(std::abs(adapteddata[i].x - read_adaptedata[i]) < 1e-16);
    }
    for (std::size_t i = 0; i < 100; ++i)
    {
        BOOST_TEST(std::abs(adapteddata[i].y - read_adaptedata[100 + i]) <
                   1e-16);
    }
    for (std::size_t i = 0; i < 100; ++i)
    {
        BOOST_TEST(std::abs(adapteddata[i].z - read_adaptedata[2 * 100 + i]) <
                   1e-16);
    }

    // read fireandforgetdataset
    auto [fireandforgetshape, read_fireandforgetdata] =
        fireandforgetdataset->read< std::vector< int > >();
    BOOST_TEST(fireandforgetshape == std::vector< hsize_t >{ 50 });
    BOOST_TEST(read_fireandforgetdata == fireandforgetdata);

    // read fireandforgetdataset2d
    auto [fireandforget2dshape, read_fireandforgetdata2d] =
        fireandforgetdataset2d->read< std::vector< int > >();
    BOOST_TEST(fireandforget2dshape == (std::vector< hsize_t >{ 5, 100 }));
    BOOST_TEST(fireandforgetdata2d == read_fireandforgetdata2d);

    ////////////////////////////////////////////////////////////////////////////
    /////////////////// Read excepiton testing here ////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    fireandforgetdataset2d->close(); // dataset invalid

    BOOST_CHECK_EXCEPTION(
        fireandforgetdataset2d->read< std::vector< int > >(),
        std::runtime_error,
        [](const std::runtime_error& err) {
            std::string m("Dataset : Dataset id is invalid"); // no name because close deletes the member
            BOOST_TEST(m == err.what());
            return true;
        });

    BOOST_CHECK_EXCEPTION(
        fireandforgetdataset->read< std::vector< int > >(
            { 0, 0, 0 }, { 10, 10, 10 }, { 2, 2, 2 }),
        std::invalid_argument,
        [](const std::invalid_argument& err) {
            std::string m("Dataset /fireandforget: start, end, stride have "
                          "to be same size as dataset rank, which is 1");
            BOOST_TEST(m == err.what());
            return true;
        });

    ////////////////////////////////////////////////////////////////////////////
    /////////////////// PARTIAL READING TAKES PLACE NOW ////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // README: offset is used in partial reads, and hence its value has to
    // be tested again (should equal start always)

    // README: below numpy slice notation is used in comments, at least for 1d

    // read [5:25:2] from container dataset
    auto [partial_contshape, read_partial_contdata] =
        contset->read< std::vector< double > >({ 5 }, { 25 }, { 2 });

    BOOST_TEST(contset->get_offset() == hsizevec{ 5 });
    BOOST_TEST(partial_contshape.size() == 1);
    BOOST_TEST(partial_contshape[0] = 20);
    BOOST_TEST(read_partial_contdata == partial_contdata);

    // read subset from nested containerdata
    auto [partial_nestedcontshape, read_partial_nestedcontdata] =
        nestedcontset->read< std::vector< std::array< int, 4 > > >(
            { 0 }, { 30 }, { 3 });

    BOOST_TEST(nestedcontset->get_offset() == hsizevec{ 0 });
    BOOST_TEST(partial_nestedcontshape.size() == 1);
    BOOST_TEST(partial_nestedcontshape[0] == 10);
    BOOST_TEST(partial_nestedcontshape[0] ==
               read_partial_nestedcontdata.size());
    BOOST_TEST(partial_nestedcontdata == read_partial_nestedcontdata);

    // read subset from 2d array: [[2,0]:[4, 100]:[1,2]]
    auto [partial2dshape, read_partial2ddata] =
        twoDdataset->read< std::vector< double > >(
            { 2, 0 }, { 4, 100 }, { 1, 2 });

    BOOST_TEST(twoDdataset->get_offset() == (hsizevec{ 2, 0 }));
    BOOST_TEST(partial2dshape.size() == 2);
    BOOST_TEST(partial2dshape[0] == 2);
    BOOST_TEST(partial2dshape[1] == 50);
    BOOST_TEST(read_partial2ddata.size() ==
               partial2dshape[0] * partial2dshape[1]);

    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 50; ++j)
        {
            BOOST_TEST(std::abs(partial_twoddata[i][j] -
                                read_partial2ddata[i * 50 + j]) < 1e-16);
        }
    }

    // read [2:3:1] -> single value from scalardataset
    auto [partial_scalarshape, read_partialscalardata] =
        scalarset->read< int >({ 2 }, { 3 }, { 1 });

    BOOST_TEST(scalarset->get_offset() == hsizevec{ 2 });

    BOOST_TEST(partial_scalarshape.size() == 1);
    BOOST_TEST(partial_scalarshape[0] == 1);
    BOOST_TEST(read_partialscalardata == 2);

    // read [5:12:1] from pointerdataset
    auto [partial_ptrshape, read_partial_ptrdata] =
        ptrset->read< double* >({ 5 }, { 12 }, { 1 });

    BOOST_TEST(ptrset->get_offset() == hsizevec{ 5 });

    BOOST_TEST(partial_ptrshape.size() == 1);
    BOOST_TEST(partial_ptrshape[0] == 7);
    BOOST_TEST(partial_ptrdata.size() == partial_ptrshape[0]);

    for (std::size_t i = 0; i < partial_ptrshape[0]; ++i)
    {
        BOOST_TEST(std::abs(partial_ptrdata[i] -
                            read_partial_ptrdata.get()[i]) < 1e-16);
    }

    // read a single string from stringdataset
    auto [singlestringshape, singlestring] =
        stringset->read< std::string >({ 3 }, { 4 }, { 1 });

    BOOST_TEST(stringset->get_offset() == hsizevec{ 3 });
    BOOST_TEST(singlestringshape.size() == 1);
    BOOST_TEST(singlestringshape[0] == 1);
    BOOST_TEST(singlestring ==
               stringcontainerdata[3]); // reuse value from stringcontainerdata

    // check that the attributes written into conset are there

    HDFAttribute attr(*contset, "first attribute");
    auto [firstshape, firstdata] = attr.read< std::vector< int > >();
    attr.close();

    attr.open(*contset, "second attribute");
    auto [secondshape, seconddata] = attr.read< std::string >();
    attr.close();

    attr.open(*contset, "third attribute");
    auto [thirdshape, thirddata] = attr.read< double >();
    attr.close();

    attr.open(*contset, "forth attribute");
    auto [forthshape, forthdata] = attr.read< unsigned long >();
    attr.close();

    attr.open(*contset, "fifth attribute");
    auto [fithshape, fithdata] = attr.read< std::vector< double > >();
    attr.close();

    BOOST_TEST(firstshape.size() == 1);
    BOOST_TEST(firstshape[0] == 5);
    BOOST_TEST(firstdata.size() == 5);
    for (int i = 0; i < 5; ++i)
    {
        BOOST_TEST(firstdata[i] == i + 1);
    }

    BOOST_TEST(secondshape.size() == 1);
    BOOST_TEST(secondshape[0] == 1);
    BOOST_TEST(seconddata == " 'tiz no attrrriboate");

    BOOST_TEST(thirdshape.size() == 1);
    BOOST_TEST(thirdshape[0] == 1);
    BOOST_TEST(std::abs(thirddata - 3.14) < 1e-16);

    BOOST_TEST(forthshape.size() == 1);
    BOOST_TEST(forthshape[0] == 1);
    BOOST_TEST(forthdata == 478953ul);

    BOOST_TEST(fithshape.size() == 1);
    BOOST_TEST(fithshape[0] == 10);
    BOOST_TEST(fithdata.size() == 10);
    for (std::size_t i = 0; i < fithdata.size(); ++i)
    {
        BOOST_TEST(std::abs(fithdata[i] - 3.14) < 1e-16);
    }
}
