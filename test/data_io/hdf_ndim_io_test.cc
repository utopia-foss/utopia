#define BOOST_TEST_MODULE HDF5_WRITE_ND_IO_TEST

#include <exception>
#include <vector>
#include <array>
#include <list>

#include <boost/mpl/vector.hpp>
#include <boost/multi_array.hpp>
#include <boost/test/included/unit_test.hpp>

// #include <H5Spublic.h>

#include <utopia/core/logging.hh>
#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/hdffile.hh>
#include <utopia/data_io/hdfgroup.hh>

// README: partial reading is tested as part of the 'read' function and hence
// is omitted in this file.
//

using namespace Utopia::DataIO;
using namespace Utopia::Utils;

// helper functions for pretty-printing vectors and lists for usage with
// boost::test
namespace std
{

template < typename T >
std::ostream&
operator<<(std::ostream& out, std::vector< T > const& vec)
{
    out << "[";
    for (std::size_t i = 0; i < vec.size() - 1; ++i)
    {
        out << vec[i] << ",";
    }
    out << vec[vec.size() - 1] << "]";
    return out;
}

template < typename T >
std::ostream&
operator<<(std::ostream& out, std::list< T > const& l)
{
    out << "[";
    for (auto it = l.begin(); it != std::next(l.begin(), l.size() - 1); ++it)
    {
        out << *it << ",";
    }
    out << *(l.rbegin()) << "]";
    return out;
}

template < typename T, std::size_t d >
std::ostream&
operator<<(std::ostream& out, std::array< T, d > const& arr)
{
    out << "[";

    for (std::size_t i = 0; i < d - 1; ++i)
    {
        out << arr[i] << ",";
    }

    out << (d - 1 >= 0 ? std::to_string(arr[d - 1]) : std::string("")) << "]";
    return out;
}

}

/**
 * @brief Produce data for a given type to fill the buffers written to file in
 * this test
 *
 * @tparam T  Type to produce
 * @tparam Ts
 * @param ts
 * @return T
 */
template < typename T, typename... Ts >
T
make_data(Ts... ts)
{

    if constexpr (std::is_same_v< T, std::string >)
    {
        // return std::to_string(i) + std::to_string(j) + std::to_string(k);
        return (std::to_string(ts) + ...);
    }
    else if constexpr (std::is_same_v< T, std::array< int, sizeof...(Ts) > >)
    {
        return std::array< int, sizeof...(Ts) >{ { static_cast< int >(
            ts)... } };
    }
    else if constexpr (std::is_same_v< T, std::vector< int > >)
    {
        return std::vector< int >{ static_cast< int >(ts)... };
    }
    else if constexpr (std::is_same_v< T, std::list< int > >)
    {
        return std::list< int >{ static_cast< int >(ts)... };
    }
    else
    {
        return (ts + ... + 0);
    }
}

/**
 * @brief Make fill values for data that is not written explicitly
 * 
 * @tparam T 
 * @return T 
 */
template < typename T>
T
make_filler()
{

    if constexpr (std::is_same_v< T, std::string >)
    {
        // return std::to_string(i) + std::to_string(j) + std::to_string(k);
        return "\0";
    }
    else{
        return make_data<T>();
    }
}


/**
 * @brief Create a name for a given type T
 *
 * @tparam T
 * @return std::string
 */
template < typename T >
std::string
create_name_for_type()
{

    if constexpr (std::is_same_v< T, std::string >)
    {
        return "string";
    }
    else if constexpr (std::is_same_v< T, std::array< int, 3 > >)
    {
        return "array";
    }
    else if constexpr (std::is_same_v< T, std::vector< int > >)
    {
        return "vector";
    }
    else if constexpr (std::is_same_v< T, std::list< int > >)
    {
        return "list";
    }
    else
    {
        return "scalar";
    }
}

// used for comparing vectors natively without the boost:test_tools::per_element
// thingy...
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< double >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< int >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< std::string >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< hsize_t >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::list< int >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< std::vector< int > >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< std::list< int > >);


/**
 * @brief Struct which acts as a factory that produces the necessary data for
 * each test.
 *
 * @tparam T
 */
template < typename T >
struct Fixture
{
    using Type = T;

    using Cube   = boost::multi_array< T, 3 >;
    using Matrix = boost::multi_array< T, 2 >;
    using Vector = boost::multi_array< T, 1 >;

    Cube array = Cube(boost::extents[10][7][4]);

    Fixture() 
    {
        // get the utopia loggers which are needed for the test
        Utopia::setup_loggers();
        // initialize data
        for (int i = 0; i < 10; ++i)
        {
            for (int j = 0; j < 7; ++j)
            {
                for (int k = 0; k < 4; ++k)
                {
                    array[i][j][k] = make_data< T >(i, j, k);
                }
            }
        }
    }

    ~Fixture() = default;
};

// example for how to use the BOOST_FIXTURE_TEMPLATE_TEST_CASE_MACRO
// https://stackoverflow.com/questions/22065225/execute-one-test-case-several-times-with-different-fixture-each-time
using typelist = boost::mpl::vector< Fixture< double >
                                    //  ,Fixture< std::string >,
                                    //  Fixture< std::array< int, 3 > >,
                                    //  Fixture< std::vector< int > >,
                                    //  Fixture< std::list< int > > 
                                     >;

// write a single multi_array in a dataset which has enough space for it but not
// more, no extension of dataset or so
BOOST_FIXTURE_TEST_CASE_TEMPLATE(write_test_limited, T, typelist, T)
{
    HDFFile file("ndtest_limited_"+create_name_for_type<typename T::Type>()+".h5", "w");
    auto dataset = file.open_dataset("/"+create_name_for_type< typename T::Type >(), {10, 7, 4});
    dataset->write_nd(T::array);
    BOOST_TEST(dataset->get_capacity() ==
               (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
                (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 0, 0, 0 }));
}

BOOST_TEST_DECORATOR(*boost::unit_test::tolerance(2e-15));
BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_test_limited, T, typelist, T)
{
    HDFFile file("ndtest_limited_"+create_name_for_type<typename T::Type>()+".h5", "r");

    auto [shape, data] =
        file.open_dataset("/"+create_name_for_type< typename T::Type >())
            ->template read< std::vector< typename T::Type > >();

    BOOST_TEST(shape == (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(data == (std::vector< typename T::Type >(
                           T::array.data(),
                           T::array.data() + T::array.num_elements())));
}

// Test appending to a dataset
BOOST_FIXTURE_TEST_CASE_TEMPLATE(write_test_append, T, typelist, T)
{
    HDFFile file("ndtest_append_"+create_name_for_type<typename T::Type>()+".h5", "w");

    auto dataset = file.open_dataset("/"+create_name_for_type<typename T::Type>());
    // first write
    dataset->write_nd(T::array);

    BOOST_TEST(dataset->get_capacity() ==
               (std::vector< hsize_t >(3, H5S_UNLIMITED)));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(dataset->get_offset() ==
               (std::vector< hsize_t >{ 0, 0, 0 }));

    // then append two times with different data

    // create some data to append
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                T::array[i][j][k] =
                    make_data< typename T::Type >(2 + i, 2 + j, 2 + k);
            }
        }
    }

    dataset->write_nd(T::array);

    BOOST_TEST(dataset->get_capacity() ==
               (std::vector< hsize_t >(3, H5S_UNLIMITED)));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 20, 7, 4 }));
    BOOST_TEST(dataset->get_offset() ==
               (std::vector< hsize_t >{ 10, 0, 0 }));

    // append once more and check parameter correctness
    typename T::Cube appended(boost::extents[10][12][4]);

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                appended[i][j][k] =
                    make_data< typename T::Type >(3 + i, 3 + j, 3 + k);
            }
        }
    }

    dataset->write_nd(appended);

    BOOST_TEST(dataset->get_capacity() ==
               (std::vector< hsize_t >(3, H5S_UNLIMITED)));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 30, 12, 4 }));
    BOOST_TEST(dataset->get_offset() ==
               (std::vector< hsize_t >{ 20, 0, 0 }));
}

BOOST_TEST_DECORATOR(*boost::unit_test::tolerance(2e-15));
BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_test_append, T, typelist, T)
{
    HDFFile file("ndtest_append_"+create_name_for_type<typename T::Type>()+".h5", "r");

    // make expected data
    typename T::Cube expected(boost::extents[30][12][4]);

    // fill expected data
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(i, j, k);
            }
        }

        for (int j = 7; j < 12; ++j){            
            for (int k = 0; k < 4; ++k)
            {
            expected[i][j][k] = make_filler<typename T::Type>();
            }
        }
    }

    for (int i = 10; i < 20; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] =
                    make_data< typename T::Type >(2 + (i - 10), 2 + j, 2 + k);
            }
        }

        for(int j = 7; j < 12; ++j){
                        for (int k = 0; k < 4; ++k)
            {
            expected[i][j][k] = make_filler<typename T::Type>();
            }
        }
    }

    for (int i = 20; i < 30; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] =
                    make_data< typename T::Type >(3 + (i - 20), 3 + j, 3 + k);
            }
        }
    }

    auto [shape, data] =
        file.open_dataset("/"+create_name_for_type<typename T::Type>())
            ->template read< std::vector< typename T::Type > >();

    BOOST_TEST(shape == (std::vector< hsize_t >{ 30, 12, 4 }));
    BOOST_TEST(data == (std::vector< typename T::Type >(
                           expected.data(),
                           expected.data() + expected.num_elements())));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(write_test_append_subdim, T, typelist, T)
{
    HDFFile file("ndtest_append_subdim_" +
                     create_name_for_type< typename T::Type >() + ".h5",
                 "w");
    auto    dataset = file.open_dataset(
        "/" + create_name_for_type< typename T::Type >(), { 12, 12, 4 });

    dataset->write_nd(T::array);

    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 0, 0, 0 }));

    typename T::Matrix m(boost::extents[12][4]);

    for (int i = 0; i < 12; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m[i][j] = make_data< typename T::Type >(0, i, j);
        }
    }

    dataset->write_nd(m);
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 11, 12, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 10, 0, 0 }));

    dataset->write_nd(m);
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 12, 12, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 11, 0, 0 }));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_test_append_subdim, T, typelist, T)
{
    HDFFile file("ndtest_append_subdim_" +
                     create_name_for_type< typename T::Type >() + ".h5",
                 "r");
    auto    dataset = file.open_dataset(
        "/" + create_name_for_type< typename T::Type >());

    typename T::Cube expected(boost::extents[12][12][4]);

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(i, j, k);
            }
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 7; j < 12; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_filler<typename T::Type>();
            }
        }
    }
    

    for (int i = 10; i < 12; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(0, j, k);
            }
        }
    }

    auto [shape, data] =
        dataset->template read< std::vector< typename T::Type > >();

    BOOST_TEST(shape == (std::vector< hsize_t >{ 12, 12, 4 }));
    BOOST_TEST(data == (std::vector< typename T::Type >(
                           expected.data(),
                           expected.data() + expected.num_elements())));
}

// test usage of custom offset argument when data shall be written that varies
// in size in another than the first dimension
BOOST_FIXTURE_TEST_CASE_TEMPLATE(write_test_custom_offset, T, typelist, T)
{
    HDFFile file("ndtest_custom_offset_" +
                     create_name_for_type< typename T::Type >()+".h5",
                 "w");

    auto dataset = file.open_dataset(
        "/" + create_name_for_type< typename T::Type >(), { 18, 8, 4 });
    dataset->write_nd(T::array, { 0, 0, 0 });

    // create data used to extent the dataset

    typename T::Matrix added_matrix(boost::extents[4][4]);
    for (int j = 0; j < 4; ++j)
    {
        for (int k = 0; k < 4; ++k)
        {
            added_matrix[j][k] = make_data< typename T::Type >(0, j, k);
        }
    }

    BOOST_TEST(dataset->get_capacity() == (std::vector< hsize_t >{ 18, 8, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 10, 7, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 0, 0, 0 }));

    dataset->write_nd(added_matrix, { 10, 0, 0 });

    BOOST_TEST(dataset->get_capacity() == (std::vector< hsize_t >{ 18, 8, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 11, 7, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 10, 0, 0 }));

    dataset->write_nd(added_matrix, { 10, 4, 0 });

    BOOST_TEST(dataset->get_capacity() == (std::vector< hsize_t >{ 18, 8, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 11, 8, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 10, 4, 0 }));

    dataset->write_nd(added_matrix, { 11, 0, 0 });

    BOOST_TEST(dataset->get_capacity() == (std::vector< hsize_t >{ 18, 8, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 12, 8, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 11, 0, 0 }));

    dataset->write_nd(added_matrix, { 11, 4, 0 });
    BOOST_TEST(dataset->get_capacity() == (std::vector< hsize_t >{ 18, 8, 4 }));
    BOOST_TEST(dataset->get_current_extent() ==
               (std::vector< hsize_t >{ 12, 8, 4 }));
    BOOST_TEST(dataset->get_offset() == (std::vector< hsize_t >{ 11, 4, 0 }));
}


BOOST_TEST_DECORATOR(*boost::unit_test::tolerance(2e-15));
BOOST_FIXTURE_TEST_CASE_TEMPLATE(read_test_custom_offset, T, typelist, T)
{
    HDFFile file("ndtest_custom_offset_" +
                     create_name_for_type< typename T::Type >()+".h5",
                 "r");

    auto dataset = file.open_dataset(
        "/" + create_name_for_type< typename T::Type >());


    // make expected data
    typename T::Cube expected(boost::extents[12][8][4]);

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(i, j, k);
            }
        }
    }


    for (int i = 0; i < 10; ++i)
    {
        for (int j = 7; j < 8; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_filler<typename T::Type>();
            }
        }
    }
    

    // data we added to the original dataset
    for (std::size_t i = 10; i < 12; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            for (std::size_t k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(0, j, k);
            }
        }
    }

    for (std::size_t i = 10; i < 12; ++i)
    {
        for (std::size_t j = 4; j < 8; ++j)
        {
            for (std::size_t k = 0; k < 4; ++k)
            {
                expected[i][j][k] = make_data< typename T::Type >(0, j - 4, k);
            }
        }
    }

    // read data and check correctness

    auto [shape, data] =
        dataset
            ->template read< std::vector< typename T::Type > >();

    BOOST_TEST(shape == (std::vector< hsize_t >{ 12, 8, 4 }));
    BOOST_TEST(data == (std::vector< typename T::Type >(
                           expected.data(),
                           expected.data() + expected.num_elements())));
}

// check-predicates for thrown messages
bool
correct_message_overstep_capacity(const std::runtime_error& ex)
{
    BOOST_CHECK_EQUAL(
        ex.what(),
        std::string("Error in error1_scalar, capacity 10 at index 0 of 3 is "
                    "too small for new extent 20"));
    return true;
}

bool
correct_message_dimensions_wrong(const std::invalid_argument& ex)
{
    BOOST_CHECK_EQUAL(
        ex.what(),
        std::string("Error, the dimensionality of the dataset, "
                    "which is 3, must be >=  the dimensionality of"
                    " the data to be written, which is 4"));
    return true;
}

// this test is here to check that the most common exceptions are thrown
BOOST_AUTO_TEST_CASE(test_exceptions)
{
    HDFFile file("ndtest_exception_check_" + create_name_for_type< int >()+".h5",
                 "w");

    auto error_dataset1 = file.open_dataset(
        "/error1_" + create_name_for_type< int >(), { 10, 7, 4 });
    auto error_dataset2 =
        file.open_dataset("/error2_" + create_name_for_type< int >());

    Fixture< int > f;

    // make data used for checking dimension inconsistency
    boost::multi_array< int, 4 > array_4d(boost::extents[10][7][4][7]);

    // initialize array via loops
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 7; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                for (int l = 0; l < 7; ++l)
                {
                    array_4d[i][j][k][l] = make_data< int >(i, j, k, l);
                }
            }
        }
    }

    // create initial data
    error_dataset1->write_nd(f.array);
    error_dataset2->write_nd(f.array);

    // do something forbidden and check that the correct
    BOOST_CHECK_EXCEPTION(error_dataset1->write_nd(f.array),
                          std::runtime_error,
                          correct_message_overstep_capacity);

    BOOST_CHECK_EXCEPTION(error_dataset2->write_nd(array_4d),
                          std::invalid_argument,
                          correct_message_dimensions_wrong);
}
