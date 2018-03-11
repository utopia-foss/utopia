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
    dataset->add_attribute("testattribute",
                           std::string("this is a testattribute"));
}

void read(std::vector<Teststruct> &data) {
    HDFFile file("integrationtest_file.h5", "r");
    // H5O_info_t infobuf;
    // herr_t status;
    // opdata od;

    // status = H5Oget_info(file.get_id(), &infobuf);
    // od.recurs = 0;
    // od.prev = NULL;
    // od.addr = infobuf.addr;

    auto group = file.open_group("/first_deeper/second_deeper/third_deeper");
    auto dataset = group->open_dataset("dataset");

    // status = H5Literate(file.get_id(), H5_INDEX_NAME, H5_ITER_NATIVE, NULL,
    //                     op_func, (void *)&od);

    auto values = dataset->read<double>();

    HDFAttribute<HDFDataset<HDFGroup>, std::string> attribute(*dataset,
                                                              "testattribute");
    auto read_attribute = attribute.read();
    assert(read_attribute == "this is a testattribute");
    assert(values.size() == data.size());
    for (std::size_t i = 0; i < data.size(); ++i) {
        assert(std::abs(values[i] - data[i].x) < 1e-16);
    }
}
int main() {
    std::vector<Teststruct> data(50);
    double d = 3.14;
    std::string a = "a";
    std::generate(data.begin(), data.end(), [&]() {
        Teststruct object;
        object.x = d;
        object.y = a;
        a += "a";
        d += 1.;
        return object;
    });

    write(data);
    read(data);
    return 0;
}