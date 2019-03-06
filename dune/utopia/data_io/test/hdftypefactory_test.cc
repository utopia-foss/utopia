/**
 * @brief Tests for the functionality of hdftypefactory are implemented here.
 *        It is checked if C types are mapped to their corresponding
 *        HDF5 types.
 *
 * @file hdftypefactory_test.cc
 */
#include "../hdftypefactory.hh"
#include <cassert>
#include <hdf5.h>
#include <list>
#include <string>
#include <vector>

using namespace Utopia;
using namespace Utopia::DataIO;

int main()
{
    // sample test for primitive types

    // int
    hid_t dtype = HDFTypeFactory::type<int>();
    assert(H5Tequal(dtype, H5T_NATIVE_INT) > 0);

    // double
    dtype = HDFTypeFactory::type<double>();
    assert(H5Tequal(dtype, H5T_NATIVE_DOUBLE) > 0);

    // vector of doubles
    auto dtypeCv = HDFTypeFactory::type<std::vector<double>>();
    hid_t varlen = H5Tvlen_create(H5T_NATIVE_DOUBLE);
    assert(H5Tequal(dtypeCv, varlen) > 0);

    // test for list of ints
    dtypeCv = HDFTypeFactory::type<std::list<int>>();
    varlen = H5Tvlen_create(H5T_NATIVE_INT);
    assert(H5Tequal(dtypeCv, varlen));

    // fixed size vector
    auto dtypeCf = HDFTypeFactory::type<std::vector<double>>(5);
    hsize_t size[1] = {5};
    hid_t double_arr_type = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, size);

    assert(H5Tget_size(dtypeCf) / sizeof(double) == 5);
    assert(H5Tequal(dtypeCf, double_arr_type));

    // test for strings
    // variable size
    dtypeCv = HDFTypeFactory::type<std::string>();
    hid_t string_varlen = H5Tcopy(H5T_C_S1);
    H5Tset_size(string_varlen, H5T_VARIABLE);
    assert(H5Tequal(dtypeCv, string_varlen));

    // fixed size
    dtypeCf = HDFTypeFactory::type<std::string>(5);
    string_varlen = H5Tcopy(H5T_C_S1);
    H5Tset_size(string_varlen, 5);
    assert(H5Tget_size(dtypeCf) / sizeof(char) == 5);
    assert(H5Tequal(dtypeCf, string_varlen));

    return 0;
}
