/**
 * @brief Tests writing different kinds of data to an Attribute to a HDFObject.
 *
 * @file hdfattribute_test_write.cc

 */
#include "../hdfattribute.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <fstream>
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
    std::default_random_engine rng(67584327);
    std::normal_distribution<double> dist(1., 2.5);
    std::uniform_int_distribution<std::size_t> idist(20, 50);
    // make file
    HDFFile file("testfile.h5", "w");

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
    std::string attributename8 = "rvalueattribute";

    // making struct data for attribute0
    std::vector<Datastruct> structdata(100);
    std::generate(structdata.begin(), structdata.end(), [&]() {
        return Datastruct{idist(rng), dist(rng), "a"};
    });

    // make attribute_data1
    std::string attribute_data1 = "this is a testing attribute";

    // make attribute_data2
    std::vector<double> attribute_data2(20);
    std::generate(attribute_data2.begin(), attribute_data2.end(),
                  [&]() { return dist(rng); });

    // make attribute_data3
    int attribute_data3 = 42;

    // make attribute_data4
    std::vector<std::vector<double>> attribute_data4(5);
    std::generate(attribute_data4.begin(), attribute_data4.end(), [&]() {
        std::vector<double> data(idist(rng));
        std::generate(data.begin(), data.end(), [&]() { return dist(rng); });
        return data;
    });

    // make attribute_data6
    int arr[20][50];
    for (std::size_t i = 0; i < 20; ++i)
    {
        for (std::size_t j = 0; j < 50; ++j)
        {
            arr[i][j] = i + j;
        }
    }

    // making data for attribute_7
    std::vector<std::string> stringvec{
        attributename0, attributename1, attributename2, attributename3,
        attributename4, attributename5, attributename6, attributename7};

    // make attributes
    HDFAttribute attribute0(low_group, attributename0);

    HDFAttribute attribute1(low_group, attributename1);

    HDFAttribute attribute2(low_group, attributename2);

    HDFAttribute attribute3(low_group, attributename3);

    HDFAttribute attribute4(low_group, attributename4);

    HDFAttribute attribute5(low_group, attributename5);

    HDFAttribute attribute6(low_group, attributename6);

    HDFAttribute attribute7(low_group, attributename7);

    HDFAttribute attribute8(low_group, attributename8);

    // write to each attribute

    // extract field from struct
    attribute0.write(structdata.begin(), structdata.end(),
                     [](auto& compound) { return compound.b; });

    // write simple string
    attribute1.write(attribute_data1);

    // write simple vector of doubles
    attribute2.write(attribute_data2);

    // write simple integer
    attribute3.write(attribute_data3);

    // write vector of vectors
    attribute4.write(attribute_data4);

    // write const char* -> this is not std::string!
    attribute5.write("this is a char* attribute");

    // write 2d array
    attribute6.write(arr, {20, 50});

    // write vector of strings
    attribute7.write(stringvec);

    // writing vector generated in adaptor
    attribute8.write(structdata.begin(), structdata.end(), [](auto& compound) {
        return std::vector<double>{static_cast<double>(compound.a), compound.b};
    });
    return 0;
}