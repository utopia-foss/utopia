#include "../hdfattribute.hh"
#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>
#include <vector>
using namespace Utopia::DataIO;
// for getting info

struct Teststruct {
    double x;
    std::string y;
    std::vector<int> z;
};

void write(std::vector<Teststruct> &data) {
    HDFFile file("integrationtest_file.h5", "w");

    // open file and read
    auto group = file.get_basegroup()
                     ->open_group("first_deeper")
                     ->open_group("second_deeper/third_deeper");

    auto dataset = group->open_dataset("dataset");
    dataset->write(data.begin(), data.end(),
                   [](auto &value) { return value.x; });
    // std::string attrdata = "this is a testattribute";
    dataset->add_attribute(
        "testattribute",
        std::string("this is an attribute to a double dataset"));

    // write variable length string
    auto dataset2 = group->open_dataset("dataset2");
    dataset2->write(data.begin(), data.end(),
                    [](auto &value) -> std::string & { return value.y; });
    dataset2->add_attribute("stringattribute",
                            "this is an attribute to std::string");

    auto dataset3 = group->open_dataset("dataset3");
    dataset3->write(data.begin(), data.end(),
                    [](auto &value) -> std::vector<int> & { return value.z; });

    dataset3->add_attribute("integer vector attribute",
                            "this is an attribute to an int vector");
}

void read(std::vector<Teststruct> &data) {
    HDFFile file("integrationtest_file.h5", "r");

    auto group = file.open_group("/first_deeper/second_deeper/third_deeper");
    auto dataset = group->open_dataset("dataset");

    auto values = dataset->read<double>();

    HDFAttribute<HDFDataset<HDFGroup>, std::string> attribute(*dataset,
                                                              "testattribute");
    auto read_attribute = attribute.read();
    assert(read_attribute == "this is an attribute to a double dataset");
    assert(values.size() == data.size());
    for (std::size_t i = 0; i < data.size(); ++i) {
        assert(std::abs(values[i] - data[i].x) < 1e-16);
    }

    auto dataset2 = group->open_dataset("dataset2");
    auto read_data = dataset2->read<std::string>();
    assert(read_data.size() == data.size());
    for (std::size_t i = 0; i < read_data.size(); ++i) {
        assert(std::string(read_data[i]) == data[i].y);
    }

    auto dataset3 = group->open_dataset("dataset3");
    auto read_data_int = dataset3->read<std::vector<int>>();
    for (std::size_t i = 0; i < read_data.size(); ++i) {
        assert(read_data_int[i].size() == data[i].z.size());
        for (std::size_t j = 0; j < data[i].z.size(); ++j) {
            assert(read_data_int[i][j] == data[i].z[j]);
        }
    }
}
int main() {
    // H5Eset_auto(error_stack, NULL, NULL); // turn off automatic error
    // printings
    std::vector<Teststruct> data(50);
    double d = 3.14;
    std::string a = "a";
    std::size_t size = 1;
    std::generate(data.begin(), data.end(), [&]() {
        Teststruct object;
        object.x = d;
        object.y = a;
        object.z = std::vector<int>(size, 1);
        a += "a";
        d += 1.;
        size += 1;
        return object;
    });

    write(data);
    read(data);
    return 0;
}