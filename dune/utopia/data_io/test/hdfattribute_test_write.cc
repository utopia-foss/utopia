#include "../hdfattribute.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>
#include <random>
#include <string>

using namespace Utopia::DataIO;

int main()
{
    std::default_random_engine rng(67584327);
    std::normal_distribution<double> dist(1., 2.5);
    std::uniform_int_distribution<std::size_t> idist(20, 50);
    // make file
    HDFFile file("testfile.h5", "w");

    // make groups
    HDFGroup low_group = HDFGroup(*file.get_basegroup(), "/testgroup");

    // adding attribute1
    std::string attributename1 = "stringattribute";
    std::string attributename2 = "vectorattribute";
    std::string attributename3 = "integerattribute";
    std::string attributename4 = "varlenattribute";
    std::string attributename5 = "charptrattribute";

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

    // make attributes
    HDFAttribute<HDFGroup> attribute1(low_group, attributename1);
    HDFAttribute<HDFGroup> attribute2(low_group, attributename2);
    HDFAttribute<HDFGroup> attribute3(low_group, attributename3);
    HDFAttribute<HDFGroup> attribute4(low_group, attributename4);
    HDFAttribute<HDFGroup> attribute5(low_group, attributename5);

    // writing a string
    attribute1.write(attribute_data1);

    // writing vector
    attribute2.write(attribute_data2);

    // writing a simple number
    attribute3.write(attribute_data3);

    // writing a nested vector
    attribute4.write(attribute_data4);

    // writing a char*
    attribute5.write("this is a char* attribute");

    // make attributes
    HDFAttribute<HDFGroup> attribute7(low_group, attributename1 + "_rvalue");
    HDFAttribute<HDFGroup> attribute8(low_group, attributename2 + "_rvalue");
    HDFAttribute<HDFGroup> attribute9(low_group, attributename3 + "_rvalue");
    HDFAttribute<HDFGroup> attribute10(low_group, attributename4 + "_rvalue");

    // writing rvalue references
    // writing a string
    attribute7.write(std::string("this is an rvalue string"));

    // writing vector
    attribute8.write(std::vector<double>({dist(rng), dist(rng), dist(rng)}));

    // writing a simple number
    attribute9.write(21);

    // writing a nested vector
    attribute10.write(std::vector<std::vector<double>>{
        {dist(rng), dist(rng), dist(rng)},
        {dist(rng), dist(rng), dist(rng), dist(rng), dist(rng), dist(rng)},
        {dist(rng), dist(rng), dist(rng), dist(rng), dist(rng)}});

    return 0;
}