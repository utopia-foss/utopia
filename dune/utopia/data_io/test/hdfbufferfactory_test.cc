#include "../hdfbufferfactory.hh"
#include "../hdfgroup.hh"
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

  std::vector<int> plain_buffer = HDFBufferFactory::buffer<int>(
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

  // convert lists to vectors first, then use buffering
  std::vector<std::vector<int>> data_vectors(100);
  for (std::size_t i = 0; i < 100; ++i) {
    data_vectors[i] =
        std::vector<int>(data_lists[i].begin(), data_lists[i].end());
  }

  std::vector<hvl_t> complex_buffer =
      HDFBufferFactory::buffer<std::vector<int>>(
          data_vectors.begin(), data_vectors.end(),
          [](auto &vector) -> std::vector<int> & { return vector; });

  assert(complex_buffer.size() == data_vectors.size());
  for (std::size_t l = 0; l < data_vectors.size(); ++l) {
    assert(data_vectors[l].size() == complex_buffer[l].len);
    for (std::size_t i = 0; i < data_vectors[i].size(); ++i) {
      data_vectors[l][i] = reinterpret_cast<int *>(complex_buffer[l].p)[i];
    }
  }
  return 0;
}