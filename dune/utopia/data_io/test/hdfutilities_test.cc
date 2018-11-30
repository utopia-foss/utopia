/**
 * @brief This file implements tests for the type-check metafunctions
 *        implemented in hdfutilities.cc
 *
 * @file hdfutilities_test.cc
 */
#include "../hdfutilities.hh"
#include <cassert>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
using namespace Utopia::DataIO;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    // test remove_pointer metafunction

    // FIXME: for some reason gcc throws compiler errors of assert
    // having been given 2 arguments when putting the rhs of a =...
    // into the assert?
    constexpr bool a = std::is_same_v<remove_pointer_t<double*>, double>;
    assert(a == true);

    constexpr bool b = std::is_same_v<remove_pointer_t<double****>, double>;
    assert(b == true);

    constexpr bool c = std::is_same_v<remove_pointer_t<double>, double>;
    assert(c == true);

    // test is_string typetrait thing
    assert(is_string_v<const std::string*&> == true);
    assert(is_string_v<int> == false);
    assert(is_string_v<std::vector<int>> == false);
    assert(is_string_v<const char*> == true);
    assert(is_string_v<char*> == true);

    using maptype = std::map<int, double>;

    // test is_containter value types
    assert(is_container_v<std::vector<double>> == true);
    assert(is_container_v<maptype> == true);
    assert(is_container_v<int> == false);
    assert(is_container_v<std::string> == false); // of special importance

    // test against rvalue refs
    assert(is_container_v<std::vector<double>&&> == true);
    assert(is_container_v<maptype&&> == true);
    assert(is_container_v<int&&> == false);
    assert(is_container_v<std::string&&> == false); // of special importance

    // test against const refs
    assert(is_container_v<const std::vector<double>&> == true);
    assert(is_container_v<const maptype&> == true);
    assert(is_container_v<const int&> == false);
    assert(is_container_v<const std::string&> == false); // of special importance

    // test against pointers
    assert(is_container_v<std::vector<double>*> == true);
    assert(is_container_v<maptype*> == true);
    assert(is_container_v<int*> == false);
    assert(is_container_v<std::string*> == false); // of special importance

    // test against const pointers
    assert(is_container_v<const std::vector<double>*> == true);
    assert(is_container_v<const maptype*> == true);
    assert(is_container_v<const int*> == false);
    assert(is_container_v<const std::string*> == false); // of special importance

    // test against some more complicated (pathological) stuff
    assert(is_container_v<const std::vector<double>*&> == true);
    assert(is_container_v<std::vector<double>*&&> == true);

    assert(is_container_v<const std::string*&> == false);
    assert(is_container_v<std::string*&&> == false);

    constexpr bool x = is_array_like_v<Dune::FieldVector<double, 5>>;
    constexpr bool v = is_array_like_v<Dune::FieldVector<int, 5>>;
    assert(x == true);
    assert(v == true);

    constexpr std::size_t sarr = get_size_v<std::array<int, 4>>;
    assert(sarr == 4);

    constexpr std::size_t sdfv = get_size_v<Dune::FieldVector<int, 4>>;
    assert(sdfv == 4);

    constexpr bool y = is_array_like_v<std::tuple<int, double, char>>;
    constexpr bool z = is_array_like_v<std::list<float>>;

    assert(y == false);
    assert(z == false);

    return 0;
}