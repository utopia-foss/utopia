#define BOOST_TEST_MODULE utils test

#include <iostream>

#include <list>
#include <forward_list>
#include <vector>
#include <deque>

#include <string>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <type_traits>

#include <utopia/core/type_traits.hh>

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/graph/adjacency_list.hpp>

struct CustomIterable
{
    using iterator = int*;
};

// demonstrate custom addition of get_size overloads to add new
// array like or tuple_like types
namespace Utopia
{
namespace Utils
{

// the char*** seems not useful, just for demonstration here
template < arma::uword N, arma::uword M >
using special_matrix = arma::Mat< char*** >::fixed< N, M >;

template < arma::uword N, arma::uword M >
struct get_size< special_matrix< N, M > >
    : std::integral_constant< std::size_t, N * M >
{
};

template < arma::uword I, arma::uword J, arma::uword K >
struct get_size< arma::cube::fixed< I, J, K > >
    : std::integral_constant< std::size_t, I * J * K >
{
};
}
}

using iterables = boost::mpl::vector< std::vector< int >,
                                      std::list< int >,
                                      std::string,
                                      std::vector< std::array< int, 3 > >,
                                      std::array< int, 3 >,
                                      std::string_view,
                                      CustomIterable >;

using containers = boost::mpl::vector< std::vector< int >,
                                       std::list< int >,
                                       std::vector< std::array< int, 3 > >,
                                       CustomIterable,
                                       std::map< int, double > >;

using strings = boost::mpl::vector< std::basic_string< int >,
                                    std::string,
                                    const char*,
                                    char*,
                                    std::basic_string_view< int > >;

using associatives = boost::mpl::vector< std::map< int, double >,
                                         std::set< int >,
                                         std::multimap< int, std::size_t >,
                                         std::multiset< int > >;

using unordered_associatives =
    boost::mpl::vector< std::unordered_map< int, double >,
                        std::unordered_set< int >,
                        std::unordered_multimap< int, std::size_t >,
                        std::unordered_multiset< int > >;

using linear_containers = boost::mpl::vector< std::vector< std::vector< int > >,
                                              std::list< int >,
                                              std::deque< int >,
                                              std::array< int, 6 >,
                                              arma::Mat< double >,
                                              arma::Row< double >,
                                              arma::Col< int >::fixed< 7 > >;

using random_access_containers =
    boost::mpl::vector< std::vector< int >,
                        std::deque< int >,
                        std::array< double, 4 >,
                        arma::Mat< double >,
                        arma::Row< double >,
                        arma::Col< int >::fixed< 7 > >;

using array_like_types = boost::mpl::vector< std::array< int, 4 >,
                                             arma::vec::fixed< 3 >,
                                             arma::ivec::fixed< 4 >,
                                             arma::Col< int32_t >::fixed< 3 >,
                                             arma::Col< uint32_t >::fixed< 6 >,
                                             arma::rowvec::fixed< 2 >,
                                             arma::irowvec::fixed< 20 >,
                                             arma::Mat< double >::fixed< 3, 4 >,
                                             arma::cube::fixed< 1, 2, 3 > >;

using tuple_like_types =
    boost::mpl::vector< std::tuple< int, double, std::string >,
                        std::pair< int, std::size_t >,
                        std::array< int, 3 >,
                        arma::mat::fixed< 3, 4 >,
                        Utopia::Utils::special_matrix< 5, 6 >,
                        arma::cube::fixed< 1, 2, 3 > >;

using non_iterables = boost::mpl::vector< int, double, const char* >;
using non_containers =
    boost::mpl::vector< std::basic_string< int >, std::string, int**, double >;

using non_random_access_containers =
    boost::mpl::vector< std::list< int >, std::forward_list< int > >;

// for graph stuff

// .. Helper ..................................................................
/// The test vertex struct
struct Vertex
{
    int i;
};

// The vertex and edge container
using vertex_cont = boost::vecS;
using edge_cont   = boost::vecS;

// .. Actual Graphs .........................................................

using graphs =
    boost::mpl::vector< boost::adjacency_list< boost::vecS, // edge container
                                               boost::vecS, // vertex container
                                               boost::undirectedS,
                                               Vertex >,
                        boost::adjacency_list< boost::vecS, // edge container
                                               boost::vecS, // vertex container
                                               boost::bidirectionalS,
                                               Vertex >,
                        boost::adjacency_list< boost::listS, // edge container
                                               boost::listS, // vertex container
                                               boost::undirectedS,
                                               Vertex >,
                        boost::adjacency_list< boost::listS, // edge container
                                               boost::listS, // vertex container
                                               boost::bidirectionalS,
                                               Vertex > >;

[[maybe_unused]] auto lambda = [](double v, double x) -> double {
    return v + x;
};

struct Operator
{

    int
    operator()(int x)
    {
        return x * 2;
    }
};

using callables = boost::mpl::
    vector< std::function< void(int) >, decltype(lambda), Operator >;

BOOST_AUTO_TEST_CASE_TEMPLATE(iterable_test, T, iterables)
{
    BOOST_TEST(Utopia::Utils::is_iterable_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_iterable_test, T, non_iterables)
{
    BOOST_TEST(not Utopia::Utils::is_iterable_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(container_test, T, containers)
{
    BOOST_TEST(Utopia::Utils::is_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_container_test, T, non_containers)
{
    BOOST_TEST(not Utopia::Utils::is_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(string_test, T, strings)
{
    BOOST_TEST(Utopia::Utils::is_string_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_string_test, T, containers)
{
    BOOST_TEST(not Utopia::Utils::is_string_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(associative_test, T, associatives)
{
    BOOST_TEST(Utopia::Utils::is_associative_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_associative_test, T, unordered_associatives)
{
    BOOST_TEST(not Utopia::Utils::is_associative_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unordered_associative_test,
                              T,
                              unordered_associatives)
{
    BOOST_TEST(Utopia::Utils::is_unordered_associative_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_unordered_associative_test, T, associatives)
{
    BOOST_TEST(not Utopia::Utils::is_unordered_associative_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(linear_container_test, T, linear_containers)
{
    BOOST_TEST(Utopia::Utils::is_linear_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_linear_container_test, T, associatives)
{
    BOOST_TEST(not Utopia::Utils::is_linear_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(random_access_container_test,
                              T,
                              random_access_containers)
{
    BOOST_TEST(Utopia::Utils::is_random_access_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_random_access_container_test,
                              T,
                              non_random_access_containers)
{
    BOOST_TEST(not Utopia::Utils::is_random_access_container_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(tuple_like_test, T, tuple_like_types)
{
    BOOST_TEST(Utopia::Utils::has_static_size_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_tuple_like_test, T, containers)
{
    BOOST_TEST(not Utopia::Utils::has_static_size_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(array_like_test, T, array_like_types)
{
    BOOST_TEST(Utopia::Utils::is_array_like_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(non_array_like_test, T, containers)
{
    BOOST_TEST(not Utopia::Utils::is_array_like_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(has_vertex_descr, T, graphs)
{
    BOOST_TEST(Utopia::Utils::has_vertex_descriptor_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(not_has_vertex_descr, T, containers)
{
    BOOST_TEST(not Utopia::Utils::has_vertex_descriptor_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(has_edge_descr, T, graphs)
{
    BOOST_TEST(Utopia::Utils::has_edge_descriptor_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(not_has_edge_descr, T, containers)
{
    BOOST_TEST(not Utopia::Utils::has_edge_descriptor_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(is_graph_test, T, graphs)
{
    BOOST_TEST(Utopia::Utils::is_graph_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(not_is_graph_test, T, containers)
{
    BOOST_TEST(not Utopia::Utils::is_graph_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(is_callable_test, T, callables)
{
    BOOST_TEST(Utopia::Utils::is_callable_v< T >);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(not_is_callable_test, T, containers)
{
    BOOST_TEST(not Utopia::Utils::is_callable_v< T >);
}
