#include "hdfbufferfactory.hh"
#include <cassert>
#include <hdf5.h>
#include <vector>

using namespace Utopia::DataIO;
struct Test {
    int a;
    double b;
    std::string c;
};

int main() {
    std::vector<Test> data(100);
    int i = 0;
    double j = 0;
    std::string k = "a";
    for (auto &value : data) {
        value.a = i;
        value.b = j;
        value.c = k;
        ++i;
        ++j;
        k += "a";
    }

    std::vector<int> plain_buffer = HDFBufferFactory::buffer(
        data.begin(), data.end(),
        [](auto &complicated_value) { return complicated_value.a; });

    assert(plain_buffer.size() == data.size());
    for (std::size_t l = 0; l < data.size(); ++l) {
        assert(plain_buffer[l] == data[l].a);
    }

    std::vector<std::list<int>> data_lists(100);

    i = 0;
    j = 0;
    std::size_t s = 1;
    for (auto &value : data_lists) {
        std::list<int> temp(s);
        int t = i + j;
        for (auto &val : temp) {
            val = t;
        }
        value = temp;
    }

    // convert lists to vectors
    std::vector<std::vector<int>> complex_buffer = HDFBufferFactory::buffer(
        data_lists.begin(), data_lists.end(),
        [](auto &list) { return std::vector<int>(list.begin(), list.end()); });

    assert(complex_buffer.size() == data_lists.size());
    for (std::size_t l = 0; l < data_lists.size(); ++l) {
        auto lbegin = data_lists[l].begin();
        auto vbegin = complex_buffer[l].begin();
        for (; lbegin != data_lists[l].end(); ++lbegin, ++vbegin) {
            assert(*lbegin == *vbegin);
        }
    }
    return 0;
}