
#include <H5Tpublic.h>
#define BOOST_TEST_MODULE typefactory_test

#include <list>
#include <string>
#include <vector>

#include <hdf5.h>

#include "utopia/core/logging.hh"
#include "utopia/data_io/hdfutilities.hh"
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdftypefactory.hh>

#include <boost/test/included/unit_test.hpp> // for unit tests

using namespace Utopia::DataIO;
using namespace Utopia::Utils;


struct Fix
{
    void setup () { Utopia::setup_loggers(); }
};

BOOST_AUTO_TEST_SUITE(Suite,
                      *boost::unit_test::fixture<Fix>())

BOOST_AUTO_TEST_CASE(constructor_and_equality_tests)
{
    HDFFile file("typefactory_testfile.h5", "w");

    auto str_dataset          = file.open_dataset("/stringdataset");
    auto vec_dataset          = file.open_dataset("/vectordataset");
    auto scl_dataset          = file.open_dataset("/scalardataset");
    auto arr_dataset          = file.open_dataset("/arraydataset");
    auto varlenvec_dataset    = file.open_dataset("/varlenvecdataset");
    auto fixedsizestr_dataset = file.open_dataset("/fixedsizestrdataset");

    scl_dataset->write(42);
    vec_dataset->write(std::vector< int >{ 1, 2, 3, 4, 5, 6, 7 });
    str_dataset->write(std::vector< std::string >{ "hello", "ya", "all" });
    arr_dataset->write(std::vector< std::array< double, 4 > >{
        { 1, 2, 3, 4 }, { -1, -2, -3, -4 } });
    varlenvec_dataset->write(std::vector< std::vector< double > >{
        { 3., 1., 2. }, { 1., 2., 3., 4., 6. } });

    fixedsizestr_dataset->write("hello");

    // create test types
    hid_t varlenstr = H5Tcopy(H5T_C_S1);
    H5Tset_size(varlenstr, H5T_VARIABLE);

    hid_t fixedsizestr = H5Tcopy(H5T_C_S1);
    H5Tset_size(fixedsizestr, 5);

    hid_t vlentype = H5Tvlen_create(H5T_NATIVE_DOUBLE);

    hsize_t dim[1]    = { 4 };
    hid_t   arraytype = H5Tarray_create(Detail::get_type< double >(), 1, dim);

    HDFTypeFactory< void > scl_type(*scl_dataset);
    BOOST_TEST(H5Tequal(scl_type.get_id(), H5T_NATIVE_INT) > 0);
    BOOST_TEST(scl_type.category() == H5T_INTEGER);
    BOOST_TEST(scl_type.is_mutable() == false);

    HDFTypeFactory< void > vec_type(*vec_dataset);
    BOOST_TEST(H5Tequal(vec_type.get_id(), H5T_NATIVE_INT) > 0);
    BOOST_TEST(vec_type.category() == H5T_INTEGER);
    BOOST_TEST(vec_type.is_mutable() == false);

    HDFTypeFactory< void > str_type(*str_dataset);
    BOOST_TEST(H5Tequal(str_type.get_id(), varlenstr) > 0);
    BOOST_TEST(str_type.category() == H5T_STRING);
    BOOST_TEST(str_type.is_mutable() == false);

    HDFTypeFactory< void > arr_type(*arr_dataset);
    BOOST_TEST(H5Tequal(arr_type.get_id(), arraytype) > 0);
    BOOST_TEST(arr_type.category() == H5T_ARRAY);
    BOOST_TEST(arr_type.is_mutable() == false);

    HDFTypeFactory< void > varlen_type(*varlenvec_dataset);
    BOOST_TEST(H5Tequal(varlen_type.get_id(), vlentype) > 0);
    BOOST_TEST(varlen_type.category() == H5T_VLEN);
    BOOST_TEST(varlen_type.is_mutable() == false);

    HDFTypeFactory< void > fixedsizestr_type(*fixedsizestr_dataset);
    BOOST_TEST(H5Tequal(fixedsizestr_type.get_id(), fixedsizestr) > 0);
    BOOST_TEST(fixedsizestr_type.category() == H5T_STRING);
    BOOST_TEST(fixedsizestr_type.is_mutable() == false);

    bool                  equal = false;
    HDFTypeFactory< int > scltype(0);

    equal = scltype == scl_type;
    BOOST_TEST(equal);

    equal = scltype == vec_type;
    BOOST_TEST(equal);

    HDFTypeFactory< std::string > strtype(0);
    equal = (strtype == str_type);
    BOOST_TEST(equal);

    HDFTypeFactory< std::string > fixedstrtype(5);
    equal = fixedstrtype == fixedsizestr_type;
    BOOST_TEST(equal);

    HDFTypeFactory< std::vector< double > > varlentype(0ul);
    equal = varlentype == varlen_type;
    BOOST_TEST(equal);

    HDFTypeFactory< std::array< double, 4 > > arrtype(4ul);
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
    H5Tclose(arraytype);
}

BOOST_AUTO_TEST_CASE(lifecycle_tests)
{

    // native, immutable types have no refcounts, hence they are always zero
    HDFTypeFactory< int > scltype(0);
    BOOST_TEST(H5Tequal(scltype.get_id(), H5T_NATIVE_INT) > 0);
    BOOST_TEST(scltype.is_mutable() == false);
    BOOST_TEST(scltype.is_valid());
    BOOST_TEST(scltype.category() == H5T_INTEGER);
    BOOST_TEST(H5Iget_ref(scltype.get_id()) == 0);

    // copy constructor
    HDFTypeFactory< int > scltype_cpy(scltype);
    BOOST_TEST(H5Tequal(scltype.get_id(), scltype_cpy.get_id()) > 0);
    BOOST_TEST(scltype_cpy.is_mutable() == true);
    BOOST_TEST(scltype_cpy.is_valid());
    BOOST_TEST(scltype_cpy.category() == H5T_INTEGER);
    BOOST_TEST(H5Iget_ref(scltype.get_id()) == 0);

    // copy assignment
    HDFTypeFactory< int > scltype_cpy2 = scltype;
    BOOST_TEST(H5Tequal(scltype.get_id(), scltype_cpy2.get_id()) > 0);
    BOOST_TEST(scltype_cpy2.is_mutable() == true);
    BOOST_TEST(scltype_cpy2.is_valid());
    BOOST_TEST(scltype_cpy2.category() == H5T_INTEGER);
    BOOST_TEST(H5Iget_ref(scltype.get_id()) == 0);

    // move constructor
    HDFTypeFactory< int > move_tmpl(0);

    HDFTypeFactory< int > moveconstructed(std::move(move_tmpl));
    BOOST_TEST(H5Tequal(scltype.get_id(), moveconstructed.get_id()) > 0);
    BOOST_TEST(moveconstructed.category() == H5T_INTEGER);
    BOOST_TEST(moveconstructed.is_mutable() == false);
    BOOST_TEST(H5Iget_ref(moveconstructed.get_id()) == 0);

    BOOST_TEST(move_tmpl.get_id() == -1);
    BOOST_TEST(move_tmpl.is_mutable() == false);
    BOOST_TEST(move_tmpl.category() == H5T_NO_CLASS);

    HDFTypeFactory< int > moveassigned = std::move(moveconstructed);
    BOOST_TEST(H5Tequal(scltype.get_id(), moveassigned.get_id()) > 0);
    BOOST_TEST(moveassigned.category() == H5T_INTEGER);
    BOOST_TEST(moveassigned.is_mutable() == false);
    BOOST_TEST(H5Iget_ref(moveassigned.get_id()) == 0);

    BOOST_TEST(moveconstructed.get_id() == -1);
    BOOST_TEST(moveconstructed.is_mutable() == false);
    BOOST_TEST(moveconstructed.category() == H5T_NO_CLASS);


    // test mutable type with active refcounting

    hid_t testtype = H5Tcopy(H5T_C_S1);
    H5Tset_size(testtype, 42);

    HDFTypeFactory<std::string> stringtype(42);
    BOOST_TEST(H5Tequal(stringtype.get_id(), testtype) > 0);
    BOOST_TEST(stringtype.is_mutable() == true);
    BOOST_TEST(stringtype.is_valid());
    BOOST_TEST(stringtype.category() == H5T_STRING);
    BOOST_TEST(H5Iget_ref(stringtype.get_id()) == 1);

    HDFTypeFactory<std::string> copied_stringtype(stringtype);
    BOOST_TEST(H5Tequal(copied_stringtype.get_id(), testtype) > 0);
    BOOST_TEST(copied_stringtype.is_mutable() == true);
    BOOST_TEST(copied_stringtype.is_valid());
    BOOST_TEST(copied_stringtype.category() == H5T_STRING);
    BOOST_TEST(H5Iget_ref(copied_stringtype.get_id()) == 2);
    BOOST_TEST(H5Iget_ref(stringtype.get_id()) == 2);


    HDFTypeFactory<std::string> copiedassigned_stringtype = stringtype;
    BOOST_TEST(H5Tequal(copiedassigned_stringtype.get_id(), testtype) > 0);
    BOOST_TEST(copiedassigned_stringtype.is_mutable() == true);
    BOOST_TEST(copiedassigned_stringtype.is_valid());
    BOOST_TEST(copiedassigned_stringtype.category() == H5T_STRING);
    BOOST_TEST(H5Iget_ref(copiedassigned_stringtype.get_id()) == 3);
    BOOST_TEST(H5Iget_ref(stringtype.get_id()) == 3);

    HDFTypeFactory<std::string> movedtype(std::move(copiedassigned_stringtype));
    BOOST_TEST(H5Tequal(movedtype.get_id(), testtype) > 0);
    BOOST_TEST(movedtype.is_mutable() == true);
    BOOST_TEST(movedtype.is_valid());
    BOOST_TEST(movedtype.category() == H5T_STRING);
    BOOST_TEST(H5Iget_ref(movedtype.get_id()) == 3);
    BOOST_TEST(H5Iget_ref(stringtype.get_id()) == 3);

    HDFTypeFactory<std::string> movedassigned_type(std::move(movedtype));
    BOOST_TEST(H5Tequal(movedassigned_type.get_id(), testtype) > 0);
    BOOST_TEST(movedassigned_type.is_mutable() == true);
    BOOST_TEST(movedassigned_type.is_valid());
    BOOST_TEST(movedassigned_type.category() == H5T_STRING);
    BOOST_TEST(H5Iget_ref(movedassigned_type.get_id()) == 3);
    BOOST_TEST(H5Iget_ref(stringtype.get_id()) == 3);


    H5Tclose(testtype);

}

BOOST_AUTO_TEST_SUITE_END()
