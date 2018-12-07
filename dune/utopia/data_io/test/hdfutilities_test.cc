/**
 * @brief This file implements tests for the type-check metafunctions
 *        implemented in hdfutilities.cc
 *
 * @file hdfutilities_test.cc
 */
#include <dune/utopia/data_io/hdfutilities.hh>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

using namespace Utopia::DataIO;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    // test remove_pointer metafunction
    constexpr bool a = std::is_same_v<remove_pointer_t<double*>, double>;
    static_assert(a == true, "remove_pointer failed");

    constexpr bool b = std::is_same_v<remove_pointer_t<double****>, double>;
    static_assert(b == true, "remove_pointer failed");

    constexpr bool c = std::is_same_v<remove_pointer_t<double>, double>;
    static_assert(c == true, "remove_pointer failed");

    // test is_string typetrait thing
    static_assert(is_string_v<const std::string*&> == true, "is_string failed");
    static_assert(is_string_v<int> == false, "is_string failed");
    static_assert(is_string_v<std::vector<int>> == false, "is_string failed");
    static_assert(is_string_v<const char*> == true, "is_string failed");
    static_assert(is_string_v<char*> == true, "is_string failed");

    using maptype = std::map<int, double>;

    // test is_containter value types
    static_assert(is_container_v<std::vector<double>> == true,
                  "is_container failed for std::vector<double>");
    static_assert(is_container_v<maptype> == true,
                  "is_container failed for std::map<int, double>");
    static_assert(is_container_v<int> == false, "is_container failed for int");
    static_assert(
        is_container_v<std::string> == false,
        "is_container failed for std::string"); // of special importance

    // test against rvalue refs
    static_assert(is_container_v<std::vector<double>&&> == true,
                  "is_container failed for std::vector<double>&&");
    static_assert(is_container_v<maptype&&> == true,
                  "is_container failed for maptype&&");
    static_assert(is_container_v<int&&> == false,
                  "is_container failed for int&&");
    static_assert(
        is_container_v<std::string&&> == false,
        "is_container failed for std::string&&"); // of special importance

    // test against const refs
    static_assert(is_container_v<const std::vector<double>&> == true,
                  "is_container failed for const std::vector<double>&");
    static_assert(is_container_v<const maptype&> == true,
                  "is_container failed for conststd::map<int, double>& ");
    static_assert(is_container_v<const int&> == false,
                  "is_container failed for const int&");
    static_assert(
        is_container_v<const std::string&> == false,
        "is_container failed for const std::string&"); // of special importance

    // test against pointers
    static_assert(is_container_v<std::vector<double>*> == true,
                  "is_container failed for std::vector<double>*");
    static_assert(is_container_v<maptype*> == true,
                  "is_container failed for std::map<int, double>*");
    static_assert(is_container_v<int*> == false,
                  "is_container failed for int*");
    static_assert(
        is_container_v<std::string*> == false,
        "is_container failed for std::string*"); // of special importance

    // test against const pointers
    static_assert(is_container_v<const std::vector<double>*> == true,
                  "is_container failed for const std::vector<double>*");
    static_assert(is_container_v<const maptype*> == true,
                  "is_container failed for const maptype*");
    static_assert(is_container_v<const int*> == false,
                  "is_container failed for const int*");
    static_assert(
        is_container_v<const std::string*> == false,
        "is_container failed for const std::string*"); // of special importance

    // test against some more complicated (pathological) stuff
    static_assert(is_container_v<const std::vector<double>*&> == true,
                  "is_container failed for const std::vector<double>*&");
    static_assert(is_container_v<std::vector<double>*&&> == true,
                  "is_container failed for std::vector<double>*&&");

    static_assert(is_container_v<const std::string*&> == false,
                  "is_container failed for const std::string*&");
    static_assert(is_container_v<std::string*&&> == false, "is_container failed for  std::string*&&");

    constexpr bool x = is_array_like_v<Dune::FieldVector<double, 5>>;
    constexpr bool v = is_array_like_v<Dune::FieldVector<int, 5>>;
    static_assert(x == true, "is_array_like failed");
    static_assert(v == true, "is_array_like failed");

    constexpr std::size_t sarr = get_size_v<std::array<int, 4>>;
    static_assert(sarr == 4, "get_size failed for arary of size 4");

    constexpr std::size_t sdfv = get_size_v<Dune::FieldVector<int, 4>>;
    static_assert(sdfv == 4, "get_size failed for Dune::FieldVecto of size 4");

    constexpr bool y = is_array_like_v<std::tuple<int, double, char>>;
    constexpr bool z = is_array_like_v<std::list<float>>;

    static_assert(y == false, "is_arraylike failed for tuple");
    static_assert(z == false, "is_arraylike failed for list");

    return 0;
}