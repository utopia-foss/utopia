/**
 * @brief This file implements tests for the type-check metafunctions
 *        implemented in hdfutilities.cc
 *
 * @file hdfutilities_test.cc
 */
#include "../hdfutilities.hh"
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace Utopia::DataIO;

int main()
{
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

    return 0;
}