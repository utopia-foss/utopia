#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <cassert>
#include <iostream>
using namespace Utopia::DataIO;

bool check_exists_group(HDFFile &file, std::string path) {

    herr_t status = H5Gget_objinfo(file.get_id(), path.c_str(), 0, NULL);

    if (status == 0) {
        return true;
    } else {
        return false;
    }
}

int main() {
    HDFFile file("grouptest_file.h5", "w");

    // open file and read

    auto base_group = file.get_basegroup();
    auto group = base_group->open_group("first_deeper")
                     ->open_group("second_deeper/third_deeper");

    assert(check_exists_group(
               file, "/first_deeper/second_deeper/third_deeper") == true);
    base_group->delete_group("first_deeper/second_deeper/third_deeper");
    assert(check_exists_group(
               file, "/first_deeper/second_deeper/third_deeper") == false);

    return 0;
}