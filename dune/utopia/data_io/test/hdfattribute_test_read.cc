#include "../hdfattribute.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>
#include <random>
#include <string>

using namespace Utopia::DataIO;

struct Datastruct
{
    std::size_t a;
    double b;
    std::string c;
};

int main()
{
    ////////////////////////////////////////////////////////////////////////////
    // preliminary stuff
    ////////////////////////////////////////////////////////////////////////////

    // README: use same seed here as in hdfattribute_test_write!
    std::default_random_engine rng(67584327);
    std::normal_distribution<double> dist(1., 2.5);
    std::uniform_int_distribution<std::size_t> idist(20, 50);

    // open a file
    HDFFile file("testfile.h5", "r");
    // make groups
    HDFGroup low_group = HDFGroup(*file.get_basegroup(), "/testgroup");

    ////////////////////////////////////////////////////////////////////////////
    // making attribute names
    ////////////////////////////////////////////////////////////////////////////

    std::string attributename0 = "coupledattribute";
    std::string attributename1 = "stringattribute";
    std::string attributename2 = "vectorattribute";
    std::string attributename3 = "integerattribute";
    std::string attributename4 = "varlenattribute";
    std::string attributename5 = "charptrattribute";
    std::string attributename6 = "multidimattribute";
    std::string attributename7 = "stringvectorattribute";

    ////////////////////////////////////////////////////////////////////////////
    // making expected data
    ////////////////////////////////////////////////////////////////////////////

    std::vector<Datastruct> expected_structdata(100);
    std::generate(expected_structdata.begin(), expected_structdata.end(), [&]() {
        return Datastruct{idist(rng), dist(rng), "a"};
    });

    std::vector<double> expected_structsubdata;
    expected_structsubdata.reserve(100);

    for (auto& value : expected_structdata)
    {
        expected_structsubdata.push_back(value.b);
    }

    // make string data
    std::string expected_stringdata = "this is a testing attribute";

    // make simple vector
    std::vector<double> expected_vectordata(20);
    std::generate(expected_vectordata.begin(), expected_vectordata.end(),
                  [&]() { return dist(rng); });

    // make int data
    int expected_intdata = 42;

    // make nested vector
    std::vector<std::vector<double>> expected_varlendata(5);
    std::generate(expected_varlendata.begin(), expected_varlendata.end(), [&]() {
        std::vector<double> data(idist(rng));
        std::generate(data.begin(), data.end(), [&]() { return dist(rng); });
        return data;
    });

    // make string which contains what has been written previously as const char*
    std::string expected_charptrdata = "this is a char* attribute";

    // make 2d data pointer
    int expected_multidimdata[20][50];
    for (std::size_t i = 0; i < 20; ++i)
    {
        for (std::size_t j = 0; j < 50; ++j)
        {
            expected_multidimdata[i][j] = i + j;
        }
    }

    // make string vector data written previously
    // making data for attribute_7
    std::vector<std::string> expected_stringvecdata{
        attributename0, attributename1, attributename2, attributename3,
        attributename4, attributename5, attributename6, attributename7};

    ////////////////////////////////////////////////////////////////////////////
    // make attributes
    ////////////////////////////////////////////////////////////////////////////

    HDFAttribute attribute0(low_group, attributename0);
    HDFAttribute attribute1(low_group, attributename1);
    HDFAttribute attribute2(low_group, attributename2);
    HDFAttribute attribute3(low_group, attributename3);
    HDFAttribute attribute4(low_group, attributename4);
    HDFAttribute attribute5(low_group, attributename5);
    HDFAttribute attribute6(low_group, attributename6);
    HDFAttribute attribute7(low_group, attributename7);

    ////////////////////////////////////////////////////////////////////////////
    // trying to read, using c++17 structured bindings
    ////////////////////////////////////////////////////////////////////////////
    auto [shape0, read_structdata] = attribute0.read<std::vector<double>>();
    assert(shape0.size() == 1);
    assert(shape0[0] == 100);

    for (std::size_t i = 0; i < 100; ++i)
    {
        assert(std::abs(read_structdata[i] - expected_structsubdata[i]) < 1e-16);
    }

    auto [shape1, read_string] = attribute1.read<std::string>();
    assert(shape1.size() == 1);
    assert(shape1[0] == 1);
    assert(read_string == expected_stringdata);

    auto [shape2, read_vectordata] = attribute2.read<std::vector<double>>();
    assert(shape2.size() == 1);
    assert(shape2[0] == 20);

    for (std::size_t i = 0; i < 20; ++i)
    {
        assert(std::abs(read_vectordata[i] - expected_vectordata[i]) < 1e-16);
    }

    auto [shape3, read_intdata] = attribute3.read<int>();
    assert(shape3.size() == 1);
    assert(shape3[0] == 1);
    assert(read_intdata == expected_intdata);

    auto [shape4, read_varlendata] =
        attribute4.read<std::vector<std::vector<double>>>();
    assert(shape4.size() == 1);
    assert(shape4[0] == 5);
    for (std::size_t i = 0; i < 5; ++i)
    {
        assert(read_varlendata[i].size() == expected_varlendata[i].size());
        for (std::size_t j = 0; j < expected_varlendata[i].size(); ++j)
        {
            assert(std::abs(read_varlendata[i][j] - expected_varlendata[i][j]) < 1e-16);
        }
    }

    auto [shape5, read_charptrdata] = attribute5.read<const char*>();
    assert(shape5.size() == 1);
    assert(shape5[0] == 1);
    assert(read_charptrdata == expected_charptrdata);

    auto [shape6, read_multidimdata] = attribute6.read<std::vector<int>>();
    assert(shape6.size() == 2);
    assert(shape6[0] = 20);
    assert(shape6[1] = 50);

    for (std::size_t i = 0; i < 20; ++i)
    {
        for (std::size_t j = 0; j < 50; ++j)
        {
            assert(std::abs(read_multidimdata[i * 50 + j] -
                            expected_multidimdata[i][j]) < 1e-16);
        }
    }

    auto [shape7, read_stringvecdata] = attribute7.read<std::vector<std::string>>();
    assert(shape7.size() == 1);
    assert(shape7[0] == 8);
    for (std::size_t i = 0; i < 8; ++i)
    {
        assert(expected_stringvecdata[i] == read_stringvecdata[i]);
    }
    return 0;
}