
#include "utopia/data_io/hdfobject.hh"
#include <H5Tpublic.h>
#include <H5version.h>
#define BOOST_TEST_MODULE typefactory_test

#include <list>
#include <string>
#include <vector>

#include <hdf5.h>

#include "utopia/core/logging.hh"
#include <utopia/data_io/hdftype.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;
using namespace Utopia::Utils;

struct Fix
{
    void
    setup()
    {
        Utopia::setup_loggers(); // init loggers
        spdlog::get("data_mngr")->set_level(spdlog::level::debug);
    }
};

using Dataset = HDFObject< HDFCategory::dataset >;

BOOST_AUTO_TEST_SUITE(Suite, *boost::unit_test::fixture< Fix >())
BOOST_AUTO_TEST_CASE(constructor_and_equality_tests)
{
    hid_t file = H5Fcreate(
        "typefactory_testfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    hid_t varlen_str_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(varlen_str_type, H5T_VARIABLE);

    hid_t fixedsize_str_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(fixedsize_str_type, 5);

    hsize_t dim[1] = {4};
    hid_t array_type = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, dim);

    hid_t varlen_vec_type = H5Tvlen_create(H5T_NATIVE_DOUBLE);

    hsize_t size[] = {2};
    hid_t space = H5Screate_simple(1, size, NULL);

    Dataset fixed_str_dataset = Dataset(H5Dcreate(file,
                                        "/fixed_stringdataset",
                                        fixedsize_str_type,
                                        space,
                                        H5P_DEFAULT,
                                        H5P_DEFAULT,
                                        H5P_DEFAULT), 
                                        &H5Dclose);

    Dataset varlen_str_dataset = Dataset(H5Dcreate(file,
                                       "/varlen_stringdataset",
                                       varlen_str_type,
                                       space,
                                       H5P_DEFAULT,
                                       H5P_DEFAULT,
                                       H5P_DEFAULT),
                                       &H5Dclose);


    Dataset scalar_dataset = Dataset(H5Dcreate(file,
                                     "/scalardataset",
                                     H5T_NATIVE_INT,
                                     space,
                                     H5P_DEFAULT,
                                     H5P_DEFAULT,
                                     H5P_DEFAULT),
                                     &H5Dclose);

    Dataset array_dataset = Dataset(H5Dcreate(file,
                                    "/arraydataset",
                                    array_type,
                                    space,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT), 
                                    &H5Dclose);

    Dataset varlen_vector_dataset = Dataset(H5Dcreate(file,
                                             "/varlen_vectordataset",
                                             varlen_vec_type,
                                             space,
                                             H5P_DEFAULT,
                                             H5P_DEFAULT,
                                             H5P_DEFAULT), 
                                             &H5Dclose);

    int x[2] = {42,21};
    H5Dwrite(scalar_dataset.get_C_id(),
             H5T_NATIVE_INT,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             &x);

    std::vector<const char *> vsd{"hello", "ya"};
    H5Dwrite(varlen_str_dataset.get_C_id(),
             varlen_str_type,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             vsd.data());

    const char* str[2] = {"hiall", "12345"};
    H5Dwrite(fixed_str_dataset.get_C_id(),
             fixedsize_str_type,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             str);

    std::vector<std::array<double, 4>> va{{1, 2, 3, 4},
                                          {-1, -2, -3, -4}};
    H5Dwrite(array_dataset.get_C_id(),
             array_type,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             va.data());

    std::vector<std::vector<double>> vvl{{3., 1., 2.},
                                         {1., 2., 3., 4., 6.}};
    std::vector<hvl_t> vvl_t(2);

    vvl_t[0].len = vvl[0].size();
    vvl_t[1].len = vvl[1].size();

    vvl_t[0].p = vvl[0].data();
    vvl_t[1].p = vvl[1].data();

    H5Dwrite(varlen_vector_dataset.get_C_id(),
             varlen_vec_type,
             H5S_ALL,
             space,
             H5P_DEFAULT,
             vvl_t.data());

    // create test types
    hid_t varlenstr = H5Tcopy(H5T_C_S1);
    H5Tset_size(varlenstr, H5T_VARIABLE);

    hid_t fixedsizestr = H5Tcopy(H5T_C_S1);
    H5Tset_size(fixedsizestr, 5);

    hid_t vlentype = H5Tvlen_create(H5T_NATIVE_DOUBLE);

    hsize_t dim2[1] = {4};
    hid_t arraytype = H5Tarray_create(Detail::get_type<double>(), 1, dim2);

    HDFType scl_type(scalar_dataset);
    BOOST_TEST(H5Tequal(scl_type.get_C_id(), H5T_NATIVE_INT) > 0);
    BOOST_TEST(scl_type.type_category() == H5T_INTEGER);
    BOOST_TEST(scl_type.is_mutable() == true);

    HDFType str_type(varlen_str_dataset);
    BOOST_TEST(H5Tequal(str_type.get_C_id(), varlenstr) > 0);
    BOOST_TEST(str_type.type_category() == H5T_STRING);
    BOOST_TEST(str_type.is_mutable() == true);

    HDFType arr_type(array_dataset);
    BOOST_TEST(H5Tequal(arr_type.get_C_id(), arraytype) > 0);
    BOOST_TEST(arr_type.type_category() == H5T_ARRAY);
    BOOST_TEST(arr_type.is_mutable() == true);

    HDFType varlen_type(varlen_vector_dataset);
    BOOST_TEST(H5Tequal(varlen_type.get_C_id(), vlentype) > 0);
    BOOST_TEST(varlen_type.type_category() == H5T_VLEN);
    BOOST_TEST(varlen_type.is_mutable() == true);

    HDFType fixedsizestr_type(fixed_str_dataset);
    BOOST_TEST(H5Tequal(fixedsizestr_type.get_C_id(), fixedsizestr) > 0);
    BOOST_TEST(fixedsizestr_type.type_category() == H5T_STRING);
    BOOST_TEST(fixedsizestr_type.is_mutable() == true);

    bool    equal = false;
    HDFType scltype;

    scltype.open< int >("testtype_int", 0);

    equal = (scltype == scl_type);
    BOOST_TEST(equal);

    HDFType strtype;
    strtype.open< std::string >("testtype_string", 0);
    equal = (strtype == str_type);
    BOOST_TEST(equal);

    HDFType fixedstrtype;
    fixedstrtype.open< std::string >("testtype_fixedstring", 5);
    equal = fixedstrtype == fixedsizestr_type;
    BOOST_TEST(equal);

    HDFType varlentype;
    varlentype.open< std::vector< double > >("testetype_vector", 0ul);
    equal = varlentype == varlen_type;
    BOOST_TEST(equal);

    HDFType arrtype;
    arrtype.open< std::array< double, 4 > >("testtype_array", 4ul);
    equal = arrtype == arr_type;
    BOOST_TEST(equal);

    // inequality check
    equal = arrtype == scltype;
    BOOST_TEST(not equal);

    bool notequal = arrtype != scltype;
    BOOST_TEST(notequal);


    H5Tclose(varlenstr);
    H5Tclose(fixedsizestr);
    H5Tclose(vlentype);
    H5Tclose(array_type);
    H5Tclose(varlen_str_type);
    H5Tclose(fixedsize_str_type);
    H5Tclose(arraytype);
    H5Sclose(space);
    H5Fclose(file);
}

BOOST_AUTO_TEST_CASE(lifecycle_tests)
{
    hsize_t dim[1]    = { 3 };
    hid_t   type_test = H5Tarray_create(H5T_NATIVE_INT, 1, dim);

    HDFType scltype;
    BOOST_TEST(not scltype.is_valid());
    scltype.open< std::array< int, 3 > >("arraytype", 3);
    BOOST_TEST(scltype.is_valid());
    BOOST_TEST(scltype.is_mutable() == true);
    BOOST_TEST(H5Tequal(scltype.get_C_id(), type_test) > 0);
    BOOST_TEST(scltype.type_category() == H5T_ARRAY);
    BOOST_TEST(scltype.get_refcount() == 1);

    // copy constructor
    HDFType scltype_cpy(scltype);
    BOOST_TEST(H5Tequal(type_test, scltype_cpy.get_C_id()) > 0);
    BOOST_TEST(scltype_cpy.is_mutable() == true);
    BOOST_TEST(scltype_cpy.is_valid());
    BOOST_TEST(scltype_cpy.type_category() == H5T_ARRAY);
    BOOST_TEST(scltype_cpy.get_refcount() == 2);
    BOOST_TEST(scltype.get_refcount() == 2);

    // copy assignment
    HDFType scltype_cpy2 = scltype;
    BOOST_TEST(H5Tequal(type_test, scltype_cpy2.get_C_id()) > 0);
    BOOST_TEST(scltype_cpy2.is_mutable() == true);
    BOOST_TEST(scltype_cpy2.is_valid());
    BOOST_TEST(scltype_cpy2.type_category() == H5T_ARRAY);
    BOOST_TEST(scltype_cpy2.get_refcount() == 3);
    BOOST_TEST(scltype_cpy.get_refcount() == 3);
    BOOST_TEST(scltype.get_refcount() == 3);

    // move constructor
    HDFType to_be_moved;
    to_be_moved.open< std::array<int, 3> >("arraytype_moved", 3);

    HDFType moveconstructed(std::move(to_be_moved));
    BOOST_TEST(H5Tequal(type_test, moveconstructed.get_C_id()) > 0);
    BOOST_TEST(moveconstructed.type_category() == H5T_ARRAY);
    BOOST_TEST(moveconstructed.is_mutable() == true);
    BOOST_TEST(moveconstructed.get_refcount() == 1);

    BOOST_TEST(to_be_moved.get_C_id() == -1);
    BOOST_TEST(to_be_moved.is_mutable() == false);
    BOOST_TEST(to_be_moved.type_category() == H5T_NO_CLASS);

    // move assignment operator
    HDFType moveassigned = std::move(moveconstructed);
    BOOST_TEST(H5Tequal(type_test, moveassigned.get_C_id()) > 0);
    BOOST_TEST(moveassigned.type_category() == H5T_ARRAY);
    BOOST_TEST(moveassigned.is_mutable() == true);
    BOOST_TEST(moveassigned.get_refcount() == 1);

    BOOST_TEST(moveconstructed.get_C_id() == -1);
    BOOST_TEST(moveconstructed.is_mutable() == false);
    BOOST_TEST(moveconstructed.type_category() == H5T_NO_CLASS);
    BOOST_TEST(moveconstructed.get_refcount() == -1);

    H5Tclose(type_test);
}
BOOST_AUTO_TEST_SUITE_END()
